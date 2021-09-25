#ifndef __LJY_THREAD_H__
#define __LJY_THREAD_H__

#include <memory>
#include <string>
#include <functional>
#include <pthread.h>
#include <stdint.h>

#include "noncopyable.h"
#include "lock.h"

namespace ljy {

class Thread : Noncopyable {
public:
    typedef std::shared_ptr<Thread> ptr;

    Thread(std::function<void()> fc, const std::string& threadName);

    ~Thread();

    //获取本线程id
    pid_t getID() const {return m_id;}

    //获取本线程名称
    const std::string getName() const {return m_name;}

    void join();

    //获取当前线程指针
    static Thread* GetThis();

    //获取当前线程名称
    static const std::string& GetName();

    static void SetName(const std::string& threadName);

private:

    static void* run(void *arg);

private:
    //线程id
    pid_t m_id;
    //线程信息
    pthread_t m_thread;
    //线程执行函数
    std::function<void()> m_fc;
    //线程名称
    std::string m_name;
    //信号量
    Semaphore m_sem;

};




















}

#endif
