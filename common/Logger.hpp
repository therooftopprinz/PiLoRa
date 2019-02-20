#ifndef SERVER_LOGGER_HPP_
#define SERVER_LOGGER_HPP_

#include <sstream>
#include <iostream>
#include <thread>
#include <list>
#include <chrono>
#include <ctime>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <unistd.h>

namespace logger
{

namespace color
{
    enum Code {
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_YELLOW   = 33,
        FG_BLUE     = 34,
        FG_DEFAULT  = 39,
        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49,
        DEFAULT  = 0
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
        // operator<<(std::ostream& os, const Modifier&) {
            return os << "\033[" << mod.code << "m";
            // return os;
        }
    };
}

enum ELogLevel {DEBUG, INFO, WARNING, ERROR};

struct LogEntry
{
    LogEntry():
        level(INFO)
    {}

    std::chrono::time_point<
        std::chrono::high_resolution_clock> time;
    std::string name;
    uint64_t threadId;
    ELogLevel level;
    std::ostringstream os;
};

class Logger
{
public:
    Logger(std::string name):
        name(name)
    {}

    std::string getName()
    {
        return name;
    }
private:
    std::string name;
};

class LoggerStream
{
public:
    LoggerStream(Logger& logger, ELogLevel logLevel):
        logger(logger),
        logLevel(logLevel)
    {

    }

    ~LoggerStream()
    {
        #ifndef __NOLOGSPLEASE__
        entry.name = logger.getName();
        entry.level = logLevel;
        entry.threadId = std::hash<std::thread::id>()(std::this_thread::get_id());
        entry.time =  std::chrono::high_resolution_clock::now();

        color::Modifier red(color::FG_RED);
        color::Modifier yellow(color::FG_YELLOW);
        color::Modifier green(color::FG_GREEN);
        color::Modifier def(color::DEFAULT);

        auto us = std::chrono::duration_cast<std::chrono::microseconds>
            (entry.time.time_since_epoch());

        std::ostringstream os;
        os << def;
        os << std::dec << us.count()%(86164000000ul) << "us ";
        
        os << color::Modifier(static_cast<color::Code>((entry.threadId&0x0F)+30));
        os << "t" << std::dec << (entry.threadId&0xFFF);
        os << def;
        os << " ";

        if (entry.level == DEBUG)
        {
            os << "DBG ";
            os << entry.name << " ";
            os << green;
        }
        else if (entry.level == INFO)
        {
            os << "INF ";
            os << entry.name << " ";
            os << green;
        }
        else if (entry.level == WARNING)
        {
            os << "WRN ";
            os << entry.name << " ";
            os << yellow;
        }
        else if (entry.level == ERROR)
        {
            os << "ERR ";
            os << entry.name << " ";
            os << red;
        }

        os << entry.os.str() << "\n";
        std::string out = os.str();
        ::write(2, out.data(), out.size());
        #endif
    }

    template<typename T>
    #ifdef __NOLOGSPLEASE__
    LoggerStream& operator << (const T&)
    #else
    LoggerStream& operator << (const T& message)
    #endif
    {
        #ifndef __NOLOGSPLEASE__
        entry.os << message;
        #endif
        return *this;
    }

private:
    Logger& logger;
    ELogLevel logLevel;
    LogEntry entry;
};

inline LoggerStream operator << (Logger& logger, ELogLevel logLevel)
{
    return LoggerStream(logger, logLevel);
}

} // namespace logger

#endif // SERVER_LOGGER_HPP_