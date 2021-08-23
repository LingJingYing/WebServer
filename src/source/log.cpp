#include <map>
#include <functional>
#include <iostream>

#include "log.h"
#include "util.h"

namespace ljy{




const char* LogLevel::ToString(LogLevel::Level level){
    switch (level)
    {
        LogLevel::Level::UNKNOW;
#define XX(name) \
    case LogLevel::Level::name: \
        return #name; \
        break;
    XX(UNKNOW)
    XX(DEBUG)
    XX(INFO)
    XX(WARN)
    XX(ERROR)
    XX(FATAL)
#undef XX
    default:
        return "UNKNOW";
        break;
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str){
#define XX(level) \
    if(#level == str){ \
        return LogLevel::Level::level; \
    }
    XX(UNKNOW)
    XX(DEBUG)
    XX(INFO)
    XX(WARN)
    XX(ERROR)
    XX(FATAL)
#undef XX
    return LogLevel::Level::UNKNOW;
}



LogEvent::LogEvent(LogLevel::Level level, const std::string& logName,
             const char *filename, const char *funcname, uint32_t line, 
             uint32_t elapse, uint32_t threadId, uint32_t fiberId, 
             uint64_t time, const std::string &threadName)
             :m_level(level),
              m_fileName(filename),
              m_funcName(funcname),
              m_line(line),
              m_elapse(elapse),
              m_threadId(threadId),
              m_fiberId(fiberId),
              m_time(time),
              m_threadName(threadName),
              m_logName(logName)
              {}



    //格式化写入日志内容
void LogEvent::format(const char *fmt, va_list al){
    char *buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if((buf != nullptr) && (len != -1)){
        m_ss << std::string(buf, len);
        free(buf);
    }
}


    //格式化写入日志内容
void LogEvent::format(const char *fmt, ...){
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}


class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << LogLevel::ToString(event->getLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getLogName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class FuncnameFormatItem : public LogFormatter::FormatItem {
public:
    FuncnameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFunc();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);//获取系统时间，精度为秒，线程安全
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);//格式化输出时间
        os << buf;
    }
private:
    std::string m_format;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << "\t";
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

LogFormatter::LogFormatter(const std::string& pattern)
:m_pattern(pattern ){
    init();
}

std::string LogFormatter::format(LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, LogEvent::ptr event) {
    for(auto& i : m_items) {
        i->format(ofs, event);
    }
    return ofs;
}

bool LogFormatter::isLegal(char c) const {
    bool isLegal = false;

    switch(c){
        case 'm':
        case 'p':
        case 'r':
        case 'c':
        case 't':
        case 'n':
        case 'd':
        case 'f':
        case 'l':
        case 'T':
        case 'F':
        case 'N':
        case 'u':
            isLegal = true;
            break;
        default:
            isLegal = false;
            break;
    }

    return isLegal;
}

void LogFormatter::init(){
    std::vector<std::tuple<std::string, std::string, int> > item;
    std::string normalStr;
    for(size_t i = 0; i < m_pattern.size(); i++){
        if(m_pattern[i] != '%'){
            normalStr.append(1, m_pattern[i]);
            continue;
        }

        if((i+1) >= m_pattern.size()){
            break;
        }

        //两个%挨在一起  %%
        if('%' == m_pattern[i+1]){
            normalStr.append(1, m_pattern[i+1]);
            continue;
        }

        if(!isLegal(m_pattern[i+1])){//%不打印，必须 %% 才打印 %
            continue;
        }
        else{
            std::string ftm;
            std::string str = m_pattern.substr(i+1, 1);
            bool hasError = false;
            size_t index = i + 1;
            if(((i+2) < m_pattern.size()) &&
               (m_pattern[i+2] == '{')){
                index = i + 2;
                while(index < m_pattern.size() && m_pattern[index] != '}'){
                    index ++;
                }
                if(index >= m_pattern.size()){
                    m_error = true;
                    str = "<error_format>";
                    ftm = m_pattern.substr(i+2, index - i - 3);
                }
                else{
                    str = m_pattern.substr(i+1, 1);
                    ftm = m_pattern.substr(i+3, index - i - 2);
                }
            }

            if(!normalStr.empty()) {
                item.push_back(std::make_tuple(normalStr, std::string(), 0));
                normalStr.clear();
            }

            item.push_back(std::make_tuple(str, ftm, 1));
            i = index;
        }
    }

    if(!normalStr.empty()) {
        item.push_back(std::make_tuple(normalStr, "", 0));
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(u, FuncnameFormatItem),          //u:函数名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
        XX(N, ThreadNameFormatItem),        //N:线程名称
#undef XX
    };

    for(auto& i : item){
        //std::cout << std::get<0>(i) << "\t" << std::get<2>(i) << std::endl;
        if(0 == std::get<2>(i)){
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else{
            auto it = s_format_items.find(std::get<0>(i));
            if(it != s_format_items.end()){
                m_items.push_back(it->second(std::get<1>(i)));
            }
            else{
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
        }
    }
}

LogAppender::LogAppender()
    :m_level(LogLevel::DEBUG),
     m_validFormatter(false)
     {}

void LogAppender::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if(m_formatter) {
        m_validFormatter = true;
    } else {
        m_validFormatter = false;
    }
}

void LogAppender::trySetFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    if(!m_validFormatter) {
        m_formatter = val;
        if(m_formatter) {
            m_validFormatter = true;
        } else {
            m_validFormatter = false;
        }
    }
}

LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void StdoutLogAppender::log(LogEvent::ptr event){
    if(event->getLevel() >= m_level) {
        MutexType::Lock lock(m_mutex);
        m_formatter->format(std::cout, event);
    }
}

FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename),
    m_lastTime(0){
    reopen();
}

void FileLogAppender::log(LogEvent::ptr event) {
    if(event->getLevel() >= m_level) {
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3)) {
            reopen();
            m_lastTime = now;
        }
        MutexType::Lock lock(m_mutex);
        //if(!(m_filestream << m_formatter->format(logger, level, event))) {
        if(!m_formatter->format(m_filestream, event)) {
            std::cout << "error" << std::endl;
        }
    }
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
    if(m_filestream.is_open()) {
        m_filestream.close();
    }
    return FSUtil::OpenForWrite(m_filestream, m_filename, std::ios::app);
}

Logger::Logger(const std::string& name)
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::log(LogEvent::ptr event) {
    if(event->getLevel() >= m_level) {
        MutexType::Lock lock(m_mutex);
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
                i->log(event);
            }
        } else if(m_root) {
            m_root->log(event);
        }
    }
}

void Logger::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;

    for(auto& i : m_appenders) {
        i->trySetFormatter(m_formatter);
    }
}

void Logger::setFormatter(const std::string& val) {
    std::cout << "---" << val << std::endl;
    ljy::LogFormatter::ptr new_val(new ljy::LogFormatter(val));
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl;
        return;
    }

    setFormatter(new_val);
}

void Logger::addAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    appender->trySetFormatter(m_formatter);
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin();
            it != m_appenders.end(); ++it) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}


LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr e)
    :m_event(e),
     m_logger(logger) {
}

LogEventWrap::~LogEventWrap() {
    m_logger->log(m_event);
}


std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}


LoggerManager::LoggerManager(){
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    m_loggers[m_root->getName()] = m_root;

    init();
}

void LoggerManager::init() {
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}







}