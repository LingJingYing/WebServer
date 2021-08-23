#include <iostream>
#include "log.h" 


void test_log(){
    std::cout << "log test begin!" << std::endl;
    ljy::Logger::ptr logger(new ljy::Logger);
    logger->addAppender(ljy::LogAppender::ptr(new ljy::StdoutLogAppender()));

    ljy::FileLogAppender::ptr file_appender(new ljy::FileLogAppender("./log.txt"));
    std::cout << "set new format!" << std::endl;
    file_appender->setFormatter(ljy::LogFormatter::ptr(new ljy::LogFormatter("%d%T%u[%l]:%m%n")));
    file_appender->setLevel(ljy::LogLevel::Level::ERROR);  

    logger->addAppender(file_appender);

    LJY_LOG_DEBUG(logger) << "test DEBUG";
    LJY_LOG_ERROR(logger) << "test ERROR";

    LJY_LOG_FMT_DEBUG(logger, "%s,%d", "test FMT DEBUG", 1);
    LJY_LOG_FMT_ERROR(logger, "%s\t%d", "test FMT ERROR", 2);

    auto log = LJY_LOG_NAME("test");

    LJY_LOG_DEBUG(log) << "test log";

    std::cout << "log test end!" << std::endl;
}

int main()
{
    test_log();
    return 0;    
}