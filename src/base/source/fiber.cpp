#include <atomic>

#include "fiber.h"
#include "log.h"
#include "config.h"
#include "macro.h"
#include "scheduler.h"


namespace ljy {

static Logger::ptr g_logger = LJY_LOG_NAME("system");

static std::atomic<uint64_t> st_fiber_id {0};    //协程ID
static std::atomic<uint64_t> st_fiber_count {0}; //协程个数


static thread_local Fiber *s_fiber = nullptr;   //当前运行的协程
static thread_local Fiber::ptr st_threadFiber = nullptr; //当前线程的主协程

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

Fiber::Fiber()
    :m_id(0),
     m_stackSize(0),
     m_stack(nullptr){
    m_state = EXEC;
    SetCurrentFiber(this);

    if(getcontext(&m_ctx)){
        LJY_ASSERT2(false, "getcontext");
    }

    ++st_fiber_count; //协程计数

    LJY_LOG_INFO(g_logger) << "main Fiber ：" << this;
}
/*
Fiber::Fiber(std::function<void()> fc, uint32_t stackSize, bool use_caller)
    :m_id(++st_fiber_id){
    ++st_fiber_count;
    m_stackSize = (stackSize != 0) ? stackSize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stackSize);
    m_fc.swap(fc);
    if(getcontext(&m_ctx)){
        LJY_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;

    if(!use_caller){
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    }
    else{
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    LJY_LOG_DEBUG(g_logger) << "Fiber id = " << m_id;
}
*/
Fiber::Fiber(std::function<void()> fc, uint32_t stackSize)
    :m_id(++st_fiber_id){
    ++st_fiber_count;
    m_stackSize = (stackSize != 0) ? stackSize : g_fiber_stack_size->getValue();
    
    m_stack = StackAllocator::Alloc(m_stackSize);
    m_fc.swap(fc);
    if(getcontext(&m_ctx)){
        LJY_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    LJY_LOG_DEBUG(g_logger) << "Fiber id = " << m_id << ", m_stackSize = " << m_stackSize;
}
Fiber::~Fiber(){
    if(m_stack){
        LJY_ASSERT(m_state == TERM
                || m_state == EXCEPT
                || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stackSize);
    }
    else{
        LJY_ASSERT(!m_fc);
        LJY_ASSERT(m_state == EXEC);

        if(s_fiber == this) {//当前运行的协程是本协程
            SetCurrentFiber(nullptr);
        }
    }
    LJY_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << " total=" << st_fiber_count;
}

void Fiber::reset(std::function<void()> fc){
    LJY_ASSERT(m_stack != nullptr);
    LJY_ASSERT(TERM == m_state ||
               EXCEPT == m_state ||
               INIT == m_state);
    
    m_fc.swap(fc);
    if(getcontext(&m_ctx)){
        LJY_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}
/*
void Fiber::swapIn(){
    SetCurrentFiber(this);
    LJY_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        LJY_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapOut(){
    SetCurrentFiber(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        LJY_ASSERT2(false, "swapcontext");
    }
}
*/
void Fiber::swapIn(){
    SetCurrentFiber(this);
    m_state = EXEC;
    LJY_ASSERT2(this != st_threadFiber.get(), "error fiber");
    if(swapcontext(&st_threadFiber->m_ctx, &m_ctx)){
        LJY_LOG_ERROR(g_logger) << errno;
        LJY_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapOut(){
    SetCurrentFiber(st_threadFiber.get());
    LJY_ASSERT2(this != st_threadFiber.get(), "error fiber");
    if(swapcontext(&m_ctx, &st_threadFiber->m_ctx)){
        LJY_LOG_ERROR(g_logger) << errno;
        LJY_ASSERT2(false, "swapcontext");
    }
}

 void Fiber::SetCurrentFiber(Fiber *f){
    s_fiber = f;
 }

Fiber::ptr Fiber::GetCurrentFiber(){
     if(s_fiber != nullptr){
         return s_fiber->shared_from_this();
     }
     Fiber::ptr main_fiber(new Fiber);
     LJY_ASSERT(s_fiber == main_fiber.get());
     st_threadFiber = main_fiber;
     return s_fiber->shared_from_this();
 }

uint64_t Fiber::GetCurrentFiberID() {
    if(s_fiber) {
        return s_fiber->getID();
    }
    return 0;
}

 void Fiber::YieldToReady(){
     Fiber::ptr cur = GetCurrentFiber();
     LJY_ASSERT(cur->m_state == EXEC);
     cur->m_state = READY;
     cur->swapOut();
 }

 void Fiber::YieldToHold(){
    Fiber::ptr cur = GetCurrentFiber();
    LJY_ASSERT(cur->m_state == EXEC);
    //cur->m_state = HOLD;
    cur->swapOut();   
 }

 uint64_t Fiber::TotalFibers(){
     return st_fiber_count;
 }

 void Fiber::MainFunc(){
    Fiber::ptr cur = GetCurrentFiber();
    LJY_ASSERT(cur);
    try {
        cur->m_fc();
        cur->m_fc = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        LJY_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getID()
            << std::endl
            << ljy::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LJY_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getID()
            << std::endl
            << ljy::BacktraceToString();
    }

    auto raw_ptr = cur.get();//获取裸指针
    cur.reset();//释放引用。因为调度协程的上下文中还有对此协程的引用，所以这里不会释放对象
    raw_ptr->swapOut();//切换回主协程

    LJY_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getID()));
 }
/*
 void Fiber::CallerMainFunc(){
    Fiber::ptr cur = GetCurrentFiber();
    LJY_ASSERT(cur);
    try {
        cur->m_fc();
        cur->m_fc = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        LJY_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getID()
            << std::endl
            << ljy::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LJY_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getID()
            << std::endl
            << ljy::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();//释放引用，因为主线中调用的Scheduler::stop中还有对此协程的引用，所以这里不会释放对象
    raw_ptr->back();//主线程的调度协程需要切换到主线程的主协程去
    LJY_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getID()));
 }
*/
}