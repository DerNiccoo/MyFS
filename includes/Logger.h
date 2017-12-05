/*
 * Logger.h
 *
 *  Created on: Dec 4, 2017
 *      Author: FireEmerald
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdarg>

// Time based logging
#include <time.h>
#include <sys/time.h>

/**
 * Usage: LOGF("Testing: LOGF %d\n", argc);
 */
#define LOGF(fmt, ...) \
do { Logger::GetLogger()->LogF(fmt, __VA_ARGS__); } while (0)

#define LOG(text) \
do { Logger::GetLogger()->Log(text); } while (0)

#define LOGM() \
do { Logger::GetLogger()->LogM(__FILE__, __LINE__, __func__); } while (0)


/**
 * Provides methods to log.
 */
class Logger {
private:
    static Logger* _logger;

    Logger();
    virtual ~Logger();

    FILE *logFile;
    bool timeBasedLogging;
    void logTimestamp();

public:
    static Logger* getLogger();

    int setLogfile(char* logFilePath);
    void setTimeBasedLogging(bool enabled);
    void log(const char* text);
    void logM(const char* file, int line, const char* func);
    void logF(const char* format, ...);
};

#endif /* LOGGER_H_ */
