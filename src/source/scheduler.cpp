#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace ljy {

static ljy::Logger::ptr g_logger = LJY_LOG_NAME("system");

static thread_local Scheduler* st_scheduler = nullptr;//当前协程调度器
static thread_local Fiber* st_scheduler_fiber = nullptr;//当前协程调度器的主协程

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name),
     m_stopping(true),
     m_autoStop(false),
     m_rootThread(-1){
    LJY_ASSERT(threads > 0);

    if(use_caller){
        //创建主协程
        ljy::Fiber::GetCurrentFiber();
        --threads;

        LJY_ASSERT(GetCurrentScheduler() == nullptr);
        SetCurrentScheduler(this);

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));//用于调度协程的协程
        ljy::Thread::SetName(m_name);

        st_scheduler_fiber = m_rootFiber.get();
        m_rootThread = ljy::GetThreadId();
        m_threadIDs.push_back(m_rootThread);
    }

    m_threadCount = threads;
}

Scheduler::~Scheduler(){
    LJY_ASSERT(m_stopping);
    if(GetCurrentScheduler() == this){
        SetCurrentScheduler(nullptr);
    }
}

Scheduler* Scheduler::GetCurrentScheduler() {
    return st_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return st_scheduler_fiber;
}

void Scheduler::start(){
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }

    m_stopping = false;
    LJY_ASSERT(m_threadPoll.empty());

    m_threadPoll.resize(m_threadCount);

    for(size_t i = 0; i < m_threadCount; i++){
        m_threadPoll[i].reset(new Thread(std::bind(&Scheduler::run, this),
                              m_name + "_" + std::to_string(i)));
        m_threadIDs.push_back(m_threadPoll[i]->getID());
    }
    lock.unlock();
}

void Scheduler::stop(){
    m_autoStop = true;
    if(m_rootFiber != nullptr &&
       0 == m_threadCount &&
       (Fiber::INIT == m_rootFiber->getState() ||
        Fiber::EXEC == m_rootFiber->getState())){
        LJY_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping()){
            return;
        }
    }

    if(m_rootThread != -1) {
        LJY_ASSERT(GetCurrentScheduler() == this);
    } else {
        LJY_ASSERT(GetCurrentScheduler() != this);
    }

    m_stopping = true;
    //唤醒所有线程，通知其要退出
    for(size_t i = 0; i < m_threadCount; i++){
        tickle();
    }

    //主线程也用于执行协程
    if(m_rootThread != -1){
        tickle();
    }

    //主线程的协程
    if(m_rootFiber) {

        if(!stopping()) {
            m_rootFiber->call();//主线程开启协程调度
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threadPoll);
    }

    for(auto& i : thrs) {
        i->join();
    } 

}

void Scheduler::switchTo(int thread){
    
}

std::ostream& Scheduler::dump(std::ostream& os){
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIDs.size(); ++i) {
        os << m_threadIDs[i];
        if(i != m_threadIDs.size()) {
            os << ", ";
        }
    }
    return os;
}

void Scheduler::tickle(){
    LJY_LOG_INFO(g_logger) << "tickle";
}

void Scheduler::run(){
    LJY_LOG_DEBUG(g_logger) << m_name << "run";
    SetCurrentScheduler(this);//设置当前协程调度器
    if(ljy::GetThreadId() != m_rootThread){
        st_scheduler_fiber = ljy::Fiber::GetCurrentFiber().get();//给其他非主线程，创建主协程
    }

    ljy::Fiber::ptr idleFiber(new Fiber(std::bind(&Scheduler::idle, this)));//idle协程
    Fiber::ptr cb_fiber;

    FiberAndThread ft;//
    while(true){
        ft.reset();
        bool needTicke = false;
        bool isActive = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fiberList.begin();
            while(it != m_fiberList.end()){
                if(it->thread != -1 && it->thread != ljy::GetThreadId()){//指定的执行线程不是当前线程
                    it++;
                    continue;
                }

                LJY_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == ljy::Fiber::EXEC){//协程已经在运行中
                    ++it;
                    continue;
                }

                ft = *it;
                m_fiberList.erase(it++);
                ++m_activeThreadCount; 
                isActive = true;
                break;
            }
            needTicke |= (it != m_fiberList.end());//判断是否需要唤醒
        }

        if(needTicke){
            tickle();
        }

        if(ft.fiber && 
           (ft.fiber->getState() != ljy::Fiber::TERM &&
            ft.fiber->getState() != ljy::Fiber::EXCEPT)){

            ft.fiber->swapIn(); //执行协程
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY){
                schedule(ft.fiber);//仍需继续执行
            }
            
            ft.reset();
        }
        else if(ft.cb){
            if(cb_fiber){
                cb_fiber->reset(ft.cb);
            }
            else{
                cb_fiber.reset(new Fiber(ft.cb));
            }

            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;

            if(cb_fiber->getState() == Fiber::READY){
                schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if(cb_fiber->getState() == Fiber::EXCEPT ||
                    cb_fiber->getState() == Fiber::TERM){
                        cb_fiber->reset(nullptr);
            }
            else{
                cb_fiber.reset();
            }
        }
        else{
            if(isActive){
                --m_activeThreadCount;
                continue;
            }

            if(idleFiber->getState() == Fiber::TERM){
                LJY_LOG_INFO(g_logger) << "Idle Fiber Finish";
                break;
            }

            ++m_idleThreadCount;
            idleFiber->swapIn();
            --m_idleThreadCount;
        }
    }
}

bool Scheduler::stopping(){
   MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping //已经启动协程，已经调用Scheduler::stop
        && m_fiberList.empty() && m_activeThreadCount == 0; //当前没有需要执行的协程，活跃线程数为0
}

void Scheduler::idle(){
    LJY_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        ljy::Fiber::YieldToHold();
    }
}

void Scheduler::SetCurrentScheduler(Scheduler* scheduler){
    st_scheduler = scheduler;
}


}