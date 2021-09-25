#include <sys/stat.h>

#include "fd_manager.h"
#include "hook.h"
#include "log.h"

namespace ljy{

static ljy::Logger::ptr g_logger = LJY_LOG_NAME("system");

Fd::Fd(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1) {
    init();
}

Fd::~Fd() {
}

bool Fd::init(){
    if(m_isInit){
        return true;
    }
    //LJY_LOG_DEBUG(g_logger) << "m_fd=" << m_fd << ", m_isInit=" << m_isInit;
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)){
        m_isInit = false;
        m_isSocket = false;
    }
    else{
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    //LJY_LOG_DEBUG(g_logger) << "m_fd=" << m_fd << ", m_isSocket=" << m_isInit;
    if(m_isSocket){
        int flag = fcntl_f(m_fd, F_GETFL, 0);
        //LJY_LOG_DEBUG(g_logger) << "1: m_fd=" << m_fd << ", flag=" << flag << ", O_NONBLOCK=" << O_NONBLOCK << ", flag & O_NONBLOCK=" << (flag & O_NONBLOCK);
        if(!(flag & O_NONBLOCK)){
            fcntl_f(m_fd, F_SETFL, flag | O_NONBLOCK);
            //LJY_LOG_DEBUG(g_logger) << "2: m_fd=" << m_fd << ", flag=" << flag;
        }
        m_sysNonblock = true;
    }
    else{
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void Fd::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

uint64_t Fd::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

Fd::ptr FdManager::get(int fd, bool auto_create) {
    if(fd == -1) {
        return nullptr;
    }
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        if(auto_create == false) {
            return nullptr;
        }
    } else {
        if(m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();
    //需要自动创建
    RWMutexType::WriteLock lock2(m_mutex);
    Fd::ptr ctx(new Fd(fd));
    if(fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;
    LJY_LOG_DEBUG(g_logger) << "fd=" << fd << ", m_sysNonblock=" << ctx->getSysNonblock();
    return ctx;
}

void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset();//释放 FdCtx 对象引用，对象自动析构
}

}