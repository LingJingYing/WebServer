#include "hook.h"
#include <dlfcn.h>

#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "macro.h"
#include "fd_manager.h"

ljy::Logger::ptr g_logger = LJY_LOG_NAME("system");
namespace ljy {

static ljy::ConfigVar<int>::ptr g_tcp_connect_timeout =
    ljy::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;  //是否启用hook

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                LJY_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;//为了进入main函数之前初始化

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {//T&&并不一定表示右值引用，它可能是个左值引用又可能是个右值引用。
    if(!ljy::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);//std::forward就能够保存參数的左值或右值特性。//完美转发
    }

    ljy::Fd::ptr ctx = ljy::FdMgr::GetInstance()->get(fd);
    if(!ctx) {//不存在
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNonblock()) {//用户自己将socket设置成非阻塞
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);//获取超时时间
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    //LJY_LOG_ERROR(g_logger) << hook_fun_name << " n=" << n;
    while(n == -1 && errno == EINTR) {//系统调用被中断
        n = fun(fd, std::forward<Args>(args)...);
    }
    if(n == -1 && errno == EAGAIN) {//当前没有数据
        ljy::IOManager* iom = ljy::IOManager::GetCurrentScheduler();
        ljy::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);
        LJY_LOG_DEBUG(g_logger) << hook_fun_name << " fd=" << fd << " n=" << n << " errno=" << errno;
        if(to != (uint64_t)-1) {//-1表示用户没有设置过超时时间
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {//添加定时器
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (ljy::IOManager::Event)(event));
            }, winfo);
        }
        
        bool rt = iom->addEvent(fd, (ljy::IOManager::Event)(event));//添加事件
        if(LJY_UNLIKELY(!rt)) {
            LJY_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            ljy::Fiber::YieldToHold();//让出调度
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }
    
    return n;
}


extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!ljy::t_hook_enable) {
        return sleep_f(seconds);
    }

    ljy::Fiber::ptr fiber = ljy::Fiber::GetCurrentFiber();
    ljy::IOManager* iom = ljy::IOManager::GetCurrentScheduler();
    iom->addTimer(seconds * 1000, std::bind((void(ljy::Scheduler::*)//函数成员指针
            (ljy::Fiber::ptr, int thread))&ljy::IOManager::schedule//模板函数，bind的时候要声明参数具体类型
            ,iom, fiber, -1));//iom为this指针，成员函数的隐藏参数
    ljy::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!ljy::t_hook_enable) {
        return usleep_f(usec);
    }
    ljy::Fiber::ptr fiber = ljy::Fiber::GetCurrentFiber();
    ljy::IOManager* iom = ljy::IOManager::GetCurrentScheduler();
    iom->addTimer(usec / 1000, std::bind((void(ljy::Scheduler::*)
            (ljy::Fiber::ptr, int thread))&ljy::IOManager::schedule
            ,iom, fiber, -1));
    ljy::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!ljy::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    ljy::Fiber::ptr fiber = ljy::Fiber::GetCurrentFiber();
    ljy::IOManager* iom = ljy::IOManager::GetCurrentScheduler();
    iom->addTimer(timeout_ms, std::bind((void(ljy::Scheduler::*)
            (ljy::Fiber::ptr, int thread))&ljy::IOManager::schedule
            ,iom, fiber, -1));
    ljy::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if(!ljy::t_hook_enable) {
        LJY_LOG_DEBUG(g_logger) << "socket no hook";
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    LJY_LOG_DEBUG(g_logger) << "socket fd=" << fd;
    if(fd == -1) {
        return fd;
    }
    ljy::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!ljy::t_hook_enable) {
        LJY_LOG_DEBUG(g_logger) << "connect_with_timeout no hook";
        return connect_f(fd, addr, addrlen);
    }
    ljy::Fd::ptr ctx = ljy::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()) {//只处理socket
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->getUserNonblock()) {//用户自己设置成非阻塞，则不需要hook
        return connect_f(fd, addr, addrlen);
    }
    LJY_LOG_DEBUG(g_logger) << "connect_f" << ", timeout_ms=" << timeout_ms;
    //LJY_ASSERT2(false, "connect_with_timeout");
    int n = connect_f(fd, addr, addrlen);
    if(n == 0) {
        LJY_LOG_DEBUG(g_logger) << "1: n=" << n << ", errno=" << errno;
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {//EINPROGRESS，代表连接还在进行中
        LJY_LOG_DEBUG(g_logger) << "2: n=" << n << ", errno=" << errno;
        return n;
    }
    LJY_LOG_DEBUG(g_logger) << "3: n=" << n << ", errno=" << errno;
    ljy::IOManager* iom = ljy::IOManager::GetCurrentScheduler();
    ljy::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1) {//-1表示用户没有通过配置文件设置超时时间
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, ljy::IOManager::WRITE);//此处取消，会把fd从epoll中删除，同时强制触发下面addEvent添加的事件，使得当前协程加入调度，
                                                              //从而可以从 ljy::Fiber::YieldToHold(); 语句后继续执行
        }, winfo);
    }

    bool rt = iom->addEvent(fd, ljy::IOManager::WRITE);//当connect成功，socket句柄立刻就能写，所以采用write事件，可以立马触发
    if(LJY_UNLIKELY(!rt)) {
        if(timer) {
            timer->cancel();
        }
        LJY_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    } else {
        ljy::Fiber::YieldToHold();
        if(timer) {//删除定时器
            timer->cancel();
        }
        //定时器 timer 先触发，则 cancelled 会被赋值
        //WRITE先触发，则切换回当前协程后，此时 cancelled 没有被赋值
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {//查看socket上是否有错误
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, ljy::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", ljy::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        ljy::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", ljy::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", ljy::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", ljy::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", ljy::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", ljy::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", ljy::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", ljy::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", ljy::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", ljy::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", ljy::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!ljy::t_hook_enable) {
        LJY_LOG_DEBUG(g_logger) << "close no hook, fd=" << fd;
        return close_f(fd);
    }

    ljy::Fd::ptr ctx = ljy::FdMgr::GetInstance()->get(fd);
    LJY_LOG_DEBUG(g_logger) << "close fd=" << fd << ", ctx" << ctx;
    if(ctx) {
        auto iom = ljy::IOManager::GetCurrentScheduler();
        if(iom) {
            iom->cancelAll(fd);
        }
        ljy::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                ljy::Fd::ptr ctx = ljy::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);//设置用户阻塞模式
                if(ctx->getSysNonblock()) {//取系统阻塞模式
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                ljy::Fd::ptr ctx = ljy::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {//取用户阻塞模式
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {//设置阻塞模式
        bool user_nonblock = !!*(int*)arg;//arg是0或非0的值，!!用于将int转换成bool
        ljy::Fd::ptr ctx = ljy::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);//设置用户阻塞模式
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!ljy::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {//接收和发送的超时时间
            ljy::Fd::ptr ctx = ljy::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);//设置超时时间
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
