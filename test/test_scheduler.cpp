#include <unistd.h>
#include "scheduler.h"
#include "log.h"

static ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    LJY_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    ::sleep(1);
    if(--s_count >= 0) {
        ljy::Scheduler::GetCurrentScheduler()->schedule(&test_fiber, ljy::GetThreadId());
    }
}

int main(int argc, char** argv) {
    ljy::Thread::SetName("main");
    LJY_LOG_INFO(g_logger) << "main";
    ljy::Scheduler sc(3, "test");
    LJY_LOG_INFO(g_logger) << "start";
    sc.start();
    LJY_LOG_INFO(g_logger) << "sleep";
    ::sleep(2);
    LJY_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    LJY_LOG_INFO(g_logger) << "stop";
    LJY_LOG_INFO(g_logger) << sc.toString();
    sc.stop();
    LJY_LOG_INFO(g_logger) << "over";
    return 0;
}