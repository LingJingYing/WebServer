#ifndef __LJY_FIBER_H__
#define __LJY_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

namespace ljy {

class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State{
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };

private:
    Fiber();

public:
    //Fiber(std::function<void()> fc, uint32_t stackSize = 0, bool use_caller = false);
    Fiber(std::function<void()> fc, uint32_t stackSize = 0);

    ~Fiber();

    void reset(std::function<void()> fc);

    void swapIn();

    void swapOut();

    void call();

    void back();

    uint64_t getID(){return m_id;}

    State getState(){return m_state;}

    static void SetCurrentFiber(Fiber *f);

    static Fiber::ptr GetCurrentFiber();

    static uint64_t GetCurrentFiberID();

    static void YieldToReady();

    static void YieldToHold();

    static uint64_t TotalFibers();

    static void MainFunc();

    static void CallerMainFunc();

private:
    //协程id
    uint64_t m_id;
    //协程运行栈大小
    uint32_t m_stackSize;
    //协程状态
    State m_state;
    //协程上下文
    ucontext_t m_ctx;
    //协程运行栈指针
    void *m_stack;
    //协程运行函数
    std::function<void()> m_fc;


};


}


#endif