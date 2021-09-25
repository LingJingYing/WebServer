#ifndef __LJY_IOMANAGER_H__
#define __LJY_IOMANAGER_H__
#include <sys/epoll.h>
#include "scheduler.h"
#include "timer.h"

namespace ljy {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event{
        NONE  = 0x0,
        READ  = 0x01,
        WRITE  = 0x04,
    };

private:

    class FdContext{
    public:
        typedef Mutex MutexType;

        struct EventContext{
            Scheduler *scheduler = nullptr;
            Fiber::ptr fiber;
            std::function<void()> cb;
        };

        EventContext& getContext(Event event);

        void resetContext(EventContext& ecx);

        void triggerEvent(Event event);

        EventContext read;
        EventContext write;
        int m_fd = 0;
        Event m_event = Event::NONE;
        MutexType mutex;
    };

public:

    IOManager(size_t threads = 1, const std::string& name = "");

    ~IOManager();

    bool addEvent(int fd, Event event, std::function<void()> fc = nullptr);

    bool delEvent(int fd, Event event);

    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    static IOManager* GetCurrentScheduler();
protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);

    bool stopping(uint64_t& timeout);
private:
    int m_epfd = 0;
    int m_tickleFds[2];
    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;

};

}


#endif