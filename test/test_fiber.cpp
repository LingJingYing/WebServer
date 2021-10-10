#include "fiber.h"
#include "log.h"

ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void run_in_fiber() {
    LJY_LOG_INFO(g_logger) << "run_in_fiber hold-1";
    ljy::Fiber::YieldToHold();
    LJY_LOG_INFO(g_logger) << "run_in_fiber hold-2";
    ljy::Fiber::YieldToHold();
    LJY_LOG_INFO(g_logger) << "run_in_fiber end";
}

void test_fiber() {
    LJY_LOG_INFO(g_logger) << "main fiber begin";
    {
        ljy::Fiber::GetCurrentFiber();
        LJY_LOG_INFO(g_logger) << "main fiber create new fiber";
        ljy::Fiber::ptr fiber(new ljy::Fiber(run_in_fiber));
        LJY_LOG_INFO(g_logger) << "main swapIn-1";
        fiber->swapIn();
        LJY_LOG_INFO(g_logger) << "main swapIn-2";
        fiber->swapIn();
        LJY_LOG_INFO(g_logger) << "main swapIn-3";
        fiber->swapIn();
//        LJY_LOG_INFO(g_logger) << "main swapIn-4";
//        fiber->swapIn();     //run_in_fiber 已经执行完，再次切换会报错
        LJY_LOG_INFO(g_logger) << "main swap end";
    }
    LJY_LOG_INFO(g_logger) << "main fiber end";
}

int main(int argc, char** argv) {
    ljy::Thread::SetName("main");

    LJY_LOG_INFO(g_logger) << "test begin";
    std::vector<ljy::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(ljy::Thread::ptr(
                    new ljy::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    LJY_LOG_INFO(g_logger) << "test middle";
    for(auto i : thrs) {
        i->join();
    }
    LJY_LOG_INFO(g_logger) << "test end";
    return 0;
}