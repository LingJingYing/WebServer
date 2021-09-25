#ifndef _LJY_LOG_H_
#define _LJY_LOG_H_

#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <stdarg.h>
#include <vector>
#include <list>
#include <map>
#include "singleton.h"
#include "lock.h"
#include "util.h"
#include "thread.h"
#include "fiber.h"

#define LJY_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        ljy::LogEventWrap(logger, ljy::LogEvent::ptr(new ljy::LogEvent(level, logger->getName(),\
                        __FILE__, __FUNCTION__, __LINE__, 0, ljy::GetThreadId(),\
                ljy::Fiber::GetCurrentFiberID(), time(0), ljy::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define LJY_LOG_DEBUG(logger) LJY_LOG_LEVEL(logger, ljy::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define LJY_LOG_INFO(logger) LJY_LOG_LEVEL(logger, ljy::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define LJY_LOG_WARN(logger) LJY_LOG_LEVEL(logger, ljy::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define LJY_LOG_ERROR(logger) LJY_LOG_LEVEL(logger, ljy::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define LJY_LOG_FATAL(logger) LJY_LOG_LEVEL(logger, ljy::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入到logger
 */
#define LJY_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        ljy::LogEventWrap(logger, ljy::LogEvent::ptr(new ljy::LogEvent(level, logger->getName(),\
                        __FILE__, __FUNCTION__, __LINE__, 0, ljy::GetThreadId(),\
                ljy::Fiber::GetCurrentFiberID(), time(0), ljy::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define LJY_LOG_FMT_DEBUG(logger, fmt, ...) LJY_LOG_FMT_LEVEL(logger, ljy::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define LJY_LOG_FMT_INFO(logger, fmt, ...)  LJY_LOG_FMT_LEVEL(logger, ljy::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define LJY_LOG_FMT_WARN(logger, fmt, ...)  LJY_LOG_FMT_LEVEL(logger, ljy::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define LJY_LOG_FMT_ERROR(logger, fmt, ...) LJY_LOG_FMT_LEVEL(logger, ljy::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define LJY_LOG_FMT_FATAL(logger, fmt, ...) LJY_LOG_FMT_LEVEL(logger, ljy::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 */
#define LJY_LOG_ROOT() ljy::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 */
#define LJY_LOG_NAME(name) ljy::LoggerMgr::GetInstance()->getLogger(name)


namespace ljy{

class LogLevel{
public:
    //告警级别
    enum Level{
        UNKNOW = 0,
        DEBUG  = 1,
        INFO   = 2,
        WARN   = 3,
        ERROR  = 4,
        FATAL  = 5
    };

    static const char* ToString(LogLevel::Level level);

    static LogLevel::Level FromString(const std::string& str);
};


/*
LogEvent 使用 Log 输出日志
Log 通过 LogAppender 去输出日志
LogAppender 根据 LogFormatter 打印日志格式
LoggerManager 管理所有 Log
*/
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;

    LogEvent(LogLevel::Level level, const std::string& logName,
             const char *filename, const char *funcname, uint32_t line, 
             uint32_t elapse, uint32_t threadId, uint32_t fiberId, 
             uint64_t time, const std::string &threadName = "None");

    //返回文件名
    const char* getFile() const {return m_fileName;}
    //返回文件名
    const char* getFunc() const {return m_funcName;}
    //返回行号
    uint32_t getLine() const {return m_line;}
    //返回程序启动开始到现在的毫秒数
    uint32_t getElapse() const {return m_elapse;}
    //返回线程ID
    uint32_t getThreadId() const {return m_threadId;}
    //返回协程ID
    uint32_t getFiberId() const {return m_fiberId;}
    //返回时间戳
    uint64_t getTime() const {return m_time;}
    //返回线程名称
    const std::string& getThreadName() const {return m_threadName;}
    //返回日志内容
    const std::string getContent() const {return m_ss.str();}
    //返回日志内容流
    std::stringstream&  getSS() {return m_ss;}
    //返回日志等级
    LogLevel::Level getLevel() const {return m_level;}
    //返回日志名称
    const std::string getLogName() const {return m_logName;}
    //格式化写入日志内容
    void format(const char *fmt, va_list al);
    //格式化写入日志内容
    void format(const char *fmt, ...);

private:
    //文件名
    const char *m_fileName;
    //函数名
    const char *m_funcName;
    //行号
    uint32_t m_line;
    //程序启动开始到现在的毫秒数
    uint32_t m_elapse;
    //线程ID
    uint32_t m_threadId;
    //协程ID
    uint32_t m_fiberId;
    //时间戳
    uint64_t m_time;
    //线程名称
    std::string m_threadName;
    //日志内容流
    std::stringstream m_ss;
    //日志等级
    LogLevel::Level m_level;
    //日志名称
    std::string m_logName;

};

class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details 
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %u 函数名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
    LogFormatter(const std::string& pattern);

    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        /**
         * @brief 析构函数
         */
        virtual ~FormatItem() {}
        /**
         * @brief 格式化日志到流
         * @param[in, out] os 日志输出流
         * @param[in] logger 日志器
         * @param[in] level 日志等级
         * @param[in] event 日志事件
         */
        virtual void format(std::ostream& os, LogEvent::ptr event) = 0;
    };

    void init();

    bool isError() const { return m_error;}

    const std::string getPattern() const { return m_pattern;}

    std::string format(LogEvent::ptr event);

    std::ostream& format(std::ostream& ofs, LogEvent::ptr event);
private:
    bool isLegal(char c) const;


private:
    //日志格式模板
    std::string m_pattern;
    //日志格式解析后格式
    std::vector<FormatItem::ptr> m_items;
    //模板是否有误
    bool m_error;

};

class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    LogAppender();

    virtual ~LogAppender() {}

    virtual void log(LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);

    void trySetFormatter(LogFormatter::ptr val);

    LogFormatter::ptr getFormatter();

    LogLevel::Level getLevel() const { return m_level;}

    void setLevel(LogLevel::Level val) { m_level = val;}

protected:
    //日志级别
    LogLevel::Level m_level;  
    //是否有自己的日志格式器
    bool m_ownFormatter;
    //锁
    MutexType m_mutex;
    //日志格式器
    LogFormatter::ptr m_formatter;
};

class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(LogEvent::ptr event) override;
    std::string toYamlString() override;
};

/**
 * @brief 输出到文件的Appender
 */
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(LogEvent::ptr event) override;
    std::string toYamlString() override;

    /**
     * @brief 重新打开日志文件
     * @return 成功返回true
     */
    bool reopen();
private:
    /// 文件路径
    std::string m_filename;
    /// 文件流
    std::ofstream m_filestream;
    /// 上次重新打开时间
    uint64_t m_lastTime;
};

class Logger{
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    Logger(const std::string &name = "root");

    //输出日志
    void log(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);

    void delAppender(LogAppender::ptr appender);

    void clearAppenders();

    const std::string& getName() const { return m_name;}

    LogLevel::Level getLevel() const {return m_level;}

    void setLevel(LogLevel::Level val) { m_level = val;}

    void setFormatter(LogFormatter::ptr val);

    void setFormatter(const std::string& val);

    LogFormatter::ptr getFormatter();

    std::string toYamlString();

private:
    //日志名称
    std::string m_name;
    //日志级别
    LogLevel::Level m_level;
    //锁
    MutexType m_mutex;
    //日志目标集合
    std::list<LogAppender::ptr> m_appenders;
    /// 日志默认格式器
    LogFormatter::ptr m_formatter;
    //默认目标
    Logger::ptr m_root;

};

class LogEventWrap {
public:

    LogEventWrap(Logger::ptr logger, LogEvent::ptr e);

    ~LogEventWrap();

    LogEvent::ptr getEvent() const { return m_event;}

    std::stringstream& getSS();

    Logger::ptr getLogger() const {return m_logger;}

private:
    LogEvent::ptr m_event;

    Logger::ptr m_logger;
};

class LoggerManager {
public:
    typedef Spinlock MutexType;

    LoggerManager();

    Logger::ptr getLogger(const std::string& name);

    void init();

    Logger::ptr getRoot() const { return m_root;}

    std::string toYamlString();
private:

    MutexType m_mutex;
    /// 日志器容器
    std::map<std::string, Logger::ptr> m_loggers;
    /// 主日志器
    Logger::ptr m_root;
};

/// 日志器管理类单例模式
typedef ljy::Singleton<LoggerManager> LoggerMgr;
}


#endif