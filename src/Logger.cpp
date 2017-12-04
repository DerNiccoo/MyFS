/*
 * Logger.cpp
 *
 *  Created on: Dec 4, 2017
 *      Author: FireEmerald
 */

#include "Logger.h"

Logger* Logger::_logger = NULL;

Logger* Logger::GetLogger() {
    if(_logger == NULL) {
        _logger = new Logger();
    }
    return _logger;
}

Logger::Logger() {
    this->logFile = stderr;
}
Logger::~Logger() {
}

/**
 * Sets the log file path for the logger.
 * If set, any further output will be written to the file, instead of be displayed.
 *
 * @param logFilePath The path of the logfile.
 * @return 0 on success and -1 on error.
 */
int Logger::SetLogfile(char* logFilePath) {

    // Open log file
    this->logFile = fopen(logFilePath, "w");
    if(this->logFile == NULL) {
        fprintf(stderr, "ERROR: Cannot open log file %s\n", logFilePath);
        return -1;
    }

    // Turn off log file buffering
    setvbuf(this->logFile, NULL, _IOLBF, 0);

    Log("Starting logging...\n");
    LOGM();
    return 0;
}

/**
 * Creates an simple log entry of plain text.
 * The log entry ends always with an new line.
 *
 * @param text The text to log.
 */
void Logger::Log(const char* text) {
    if (this->logFile == stderr) {
        std::cout << text << std::endl;
    } else {
        fprintf(this->logFile, "%s\n", text);
    }
}

/**
 * Creates an log entry of an method call position.
 * The log entry ends always with an new line.
 *
 * @param file The file name.
 * @param line The line number.
 * @param func The name of the calling function.
 */
void Logger::LogM(const char* file, int line, const char* func) {
    if (this->logFile == stderr) {
        std::cout << file << ":" << line << ":" << func << "()" << std::endl;
    } else {
        fprintf(this->logFile, "%s:%d:%s()\n", file, line, func);
    }
}

/**
 * Creates an log entry of an formatted string with variable arguments.
 * The log entry doesn't end with an new line by default!
 * Use \n to add a new line in your provided format.
 *
 * See https://stackoverflow.com/a/1056442/4300087
 *
 * @param format A composite format string.
 * @param ... Zero or more objects to format.
 */
void Logger::LogF(const char* format, ...) {
    std::va_list argptr;
    va_start(argptr, format);
    vfprintf(this->logFile, format, argptr);
    va_end(argptr);
}
