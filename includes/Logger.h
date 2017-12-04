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

    FILE *logFile;

    Logger();
    virtual ~Logger();

public:
    static Logger* GetLogger();

    int SetLogfile(char* logFilePath);
    void Log(const char* text);
    void LogM(const char* file, int line, const char* func);
    void LogF(const char* format, ...);
};

#endif /* LOGGER_H_ */
