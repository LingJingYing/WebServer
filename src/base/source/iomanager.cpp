#include <unistd.h>
#include <fcntl.h>
#include "iomanager.h"
#include "macro.h"
#include "log.h"

namespace ljy{

static ljy::Logger::ptr g_logger = LJY_LOG_NAME("system");

enum EpollCtlOp {
};

static std::ostream& operator<< (std::ostream& os, const EpollCtlOp& op) {
    switch((int)op) {
#define XX(ctl) \
        case ctl: \
            return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events) {
    if(!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            LJY_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {

    LJY_ASSERT(m_event & event);

    m_event = (Event)(m_event & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    //resetContext(ctx);
    return;
}

IOManager::IOManager(size_t threads, const std::string& name)
    :Scheduler(threads, name) {
    m_epfd = epoll_create(5000);
    LJY_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);//0：读端，1：写端
    LJY_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);//设置成非阻塞
    LJY_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    LJY_ASSERT(!rt);

    contextResize(32);

    start();
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    if(m_fdContexts.size() == size){
        return;
    }

    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->m_fd = i;
        }
    }
}

bool IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if(static_cast<int>(m_fdContexts.size()) > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }
    LJY_LOG_DEBUG(g_logger) << "fd=" << fd << " event=" << event;
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(LJY_UNLIKELY(fd_ctx->m_event & event)) {//事件已经存在
        LJY_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->m_event;
        LJY_ASSERT(!(fd_ctx->m_event & event));
    }

    int op = fd_ctx->m_event ? EPOLL_CTL_MOD : EPOLL_CTL_ADD; //修改或新增
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->m_event | event;//添加事件
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        LJY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->m_event;
        return false;
    }

    ++m_pendingEventCount;
    fd_ctx->m_event = (Event)(fd_ctx->m_event | event);//更新当前事件
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    LJY_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetCurrentScheduler();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        //传入当前协程，事件触发时执行此协程，此时协程调用hold让出调度，待时间触发后，继续从hold语句后面执行
        event_ctx.fiber = Fiber::GetCurrentFiber();
        LJY_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                      ,"state=" << event_ctx.fiber->getState());
    }
    return true;
}


bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if(static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(LJY_UNLIKELY(!(fd_ctx->m_event & event))) {//没有需要删除的事件
        return false;
    }

    Event new_events = (Event)(fd_ctx->m_event & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL; //修改或删除
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        LJY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->m_event = new_events;//更新当前事件
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if(static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(LJY_UNLIKELY(!(fd_ctx->m_event & event))) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->m_event & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        LJY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->m_event) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        LJY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->m_event & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->m_event & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    LJY_ASSERT(fd_ctx->m_event == 0);
    return true;
}


IOManager* IOManager::GetCurrentScheduler() {
    return dynamic_cast<IOManager*>(Scheduler::GetCurrentScheduler());
}

void IOManager::tickle() {
    if(!hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    LJY_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();

}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    LJY_LOG_DEBUG(g_logger) << "idle";
    const uint64_t MAX_EVNETS = 256;
    epoll_event* events = new epoll_event[MAX_EVNETS]();
    //需指定析构函数，否则无法析构数组
    //经过测试，需此idle函数执行结束，shared_events对象才会被析构，即从idle函数切去其他协程后，要切回idle协程才可以
    std::shared_ptr<epoll_event>  shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        if(LJY_UNLIKELY(stopping(next_timeout))) {
            LJY_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            //LJY_LOG_INFO(g_logger) << "next_timeout=" << next_timeout;
            //不会出现惊群现象，因为所有线程使用的是同一个epoll句柄，而哪个线程处理Fiber都无所谓，只要切换到对应协程执行即可
            //timeout：指定epoll的超时时间，单位是毫秒。当timeout为-1是，epoll_wait调用将永远阻塞，直到某个时间发生。
            //当timeout为0时，epoll_wait调用将立即返回。
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);//超时的精度是毫秒级的
            LJY_LOG_DEBUG(g_logger) << "rt=" << rt << ", next_timeout=" << next_timeout;
            if(rt < 0 && errno == EINTR) {//被信号中断
            } else {
                break;
            }
        } while(true);

        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }


        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];

            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->m_event;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }
            LJY_LOG_DEBUG(g_logger) << " fd_ctx=" << fd_ctx->m_fd << " events=" << event.events << ", real_events=" << real_events;
            if((fd_ctx->m_event & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->m_event & ~real_events);//剩余事件
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->m_fd, &event);//将剩余事件冲进加入到epoll监听
            if(rt2) {
                LJY_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << (EpollCtlOp)op << ", " << fd_ctx->m_fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }


            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::GetCurrentFiber();
        auto raw_ptr = cur.get();
        cur.reset(); //解除引用，防止此Fiber无法析构
        //让出调度，回到调度协程
        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}


}