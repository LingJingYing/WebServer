#ifndef __LJY_SCHEDULER_H__
#define __LJY_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <atomic>
#include "lock.h"
#include "thread.h"
#include "fiber.h"

namespace ljy {

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    virtual ~Scheduler();

    const std::string& getName() {return m_name;}

    static Scheduler* GetCurrentScheduler();

    static Fiber* GetMainFiber();

    void start();

    void stop();

    void switchTo(int thread = -1);

    std::ostream& dump(std::ostream& os);

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if(need_tickle) {
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }

private:
    /**
     * @brief 协程调度启动(无锁)
     */
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fiberList.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fiberList.push_back(ft);
        }
        return need_tickle;
    }
protected:
    /**
     * @brief 通知协程调度器有任务了
     */

    virtual void tickle();
    /**
     * @brief 协程调度函数
     */
    void run();

    /**
     * @brief 返回是否可以停止
     */
    virtual bool stopping();

    /**
     * @brief 协程无任务可调度时执行idle协程
     */
    virtual void idle();

    /**
     * @brief 设置当前的协程调度器
     */
    static void SetCurrentScheduler(Scheduler* scheduler);

    /**
     * @brief 是否有空闲线程
     */
    bool hasIdleThreads() { return m_idleThreadCount > 0;}

private:
    /**
     * @brief 协程/函数/线程组
     */
    struct FiberAndThread {
        /// 协程
        Fiber::ptr fiber;
        /// 协程执行函数
        std::function<void()> cb;
        /// 线程id
        int thread;

        /**
         * @brief 构造函数
         * @param[in] f 协程
         * @param[in] thr 线程id
         */
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(Fiber::ptr* f, int thr)
            :thread(thr) {
            fiber.swap(*f);
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数
         * @param[in] thr 线程id
         */
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数指针
         * @param[in] thr 线程id
         * @post *f = nullptr
         */
        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
            cb.swap(*f);
        }

        /**
         * @brief 无参构造函数
         */
        FiberAndThread()
            :thread(-1) {
        }

        /**
         * @brief 重置数据
         */
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    //互斥锁
    MutexType m_mutex;
    //协程调度器名称
    std::string m_name;
    //线程池
    std::vector<Thread::ptr> m_threadPoll;
    //等待执行的协程队列
    std::list<FiberAndThread> m_fiberList;
    //use_caller 为 true 时启用，调度协程
    Fiber::ptr m_rootFiber;
protected:
    //线程id数组
    std::vector<int> m_threadIDs;
    //线程数量
    size_t m_threadCount;
    //工作线程数量
    std::atomic<size_t> m_activeThreadCount;
    //空闲线程数量
    std::atomic<size_t> m_idleThreadCount;
    /// 是否正在停止
    bool m_stopping;
    /// 是否自动停止
    bool m_autoStop;
    /// 主线程id(use_caller)
    int m_rootThread;
};


}


#endif