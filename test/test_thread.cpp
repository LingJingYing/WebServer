#include <vector>
#include "thread.h"
#include "log.h"

ljy::Logger::ptr g_logger = LJY_LOG_ROOT();

void func1(){
    LJY_LOG_INFO(g_logger) << "name: " << ljy::Thread::GetName()
                             << " this.name: " << ljy::Thread::GetThis()->getName()
                             << " id: " << ljy::GetThreadId()
                             << " this.id: " << ljy::Thread::GetThis()->getID();
}

void func2() {
    while(true) {
        LJY_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

int main(){
    LJY_LOG_INFO(g_logger) << "thread test begin!";
    std::vector<ljy::Thread::ptr> vec;

    for(size_t i = 0; i < 10; i++){
        ljy::Thread::ptr threadPtr(new ljy::Thread(&func1, "name_"+std::to_string(i)));
        vec.push_back(threadPtr);
    }

    for(auto& v : vec){
        v->join();
        //LJY_LOG_INFO(g_logger) << "join:" << i;

    }

    LJY_LOG_INFO(g_logger) << "thread test end!";

    return 0;
}