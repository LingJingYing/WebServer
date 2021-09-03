#include "thread.h"
#include "log.h"

namespace ljy {

static thread_local Thread *st_thread = nullptr;
static thread_local std::string st_thread_name = "UNKNOW";

static ljy::Logger::ptr g_logger = LJY_LOG_NAME("system");

Thread* Thread::GetThis(){
    return st_thread;
}

const std::string& Thread::GetName(){
    return st_thread_name;
}


void Thread::SetName(const std::string& threadName){
    if(threadName.empty()){
        return;
    }

    if(st_thread){
        st_thread->m_name = threadName;
    }

    st_thread_name = threadName;
}

Thread::Thread(std::function<void()> fc, const std::string& threadName)
    :m_id(-1),
     m_thread(0),
     m_fc(fc),
     m_name(threadName){
    if(threadName.empty()){
        m_name = "UNKNOW";
    }

    int ret = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(ret != 0){
        LJY_LOG_ERROR(g_logger) << "pthread_create fail, ret=" << ret << " name=" << m_name;
        throw std::logic_error("pthread_create error!");
    }
    //LJY_LOG_ERROR(g_logger) << threadName << ":wait" << std::endl;
    m_sem.wait();
    //LJY_LOG_ERROR(g_logger) << threadName << ":waitend" << std::endl;
}

Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
    }
    //LJY_LOG_ERROR(g_logger) << "~Thread:" << this->m_name;
}

void Thread::join(){
    if(m_thread){
        int ret = pthread_join(m_thread, nullptr);
        if(ret != 0){
            LJY_LOG_ERROR(g_logger) << "pthread_join fail, ret=" << ret << " name=" << m_name;
            throw std::logic_error("pthread_join error!");
        }
        m_thread = 0;
    }
}

void* Thread::run(void *arg){
    Thread *thread = static_cast<Thread*>(arg);
    st_thread = thread;
    st_thread_name = thread->m_name;
    thread->m_id = ljy::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> fc;
    fc.swap(thread->m_fc);

    //LJY_LOG_INFO(g_logger)  << "this1:" <<thread->m_name << "-" << thread->m_id << std::endl;
    //LJY_LOG_INFO(g_logger)  << "thread_local1:" <<thread->m_name << "-" << thread->m_id << std::endl;

    thread->m_sem.notify();

    //LJY_LOG_INFO(g_logger) << "this2:" <<thread->m_name << "-" << thread->m_id << std::endl;
    //LJY_LOG_INFO(g_logger) << "thread_local2:" <<thread->m_name << "-" << thread->m_id << std::endl;
    fc();

    return 0;
}

}