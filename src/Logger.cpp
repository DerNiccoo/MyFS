/*
 * Logger.cpp
 *
 *  Created on: Dec 4, 2017
 *      Author: FireEmerald
 */

#include "Logger.h"

Logger* Logger::_logger = NULL;

Logger* Logger::getLogger() {
    if(_logger == NULL) {
        _logger = new Logger();
    }
    return _logger;
}

Logger::Logger() {
    this->timeBasedLogging = false; // Disabled by default.
    this->logFile = stdout;         // If none log file path was set.
}
Logger::~Logger() {
}

/**
 * Sets the log file path for the logger.
 * If set, any further output will be written to the file, instead of stdout.
 *
 * @param logFilePath The path of the logfile.
 * @return 0 on success and -1 on error.
 */
int Logger::setLogfile(char* logFilePath) {

    // Open log file
    this->logFile = fopen(logFilePath, "a"); // Create new log file or append
    if(this->logFile == NULL) {
        this->logFile = stdout;
        fprintf(stderr, "ERROR: Cannot open log file %s\n", logFilePath);
        return -1;
    }

    // Turn off log file buffering
    setvbuf(this->logFile, NULL, _IOLBF, 0);
    logF("―――――――――――――――――――――――――――――――――――――\nStarting logging to log file...\n");
    LOGM();
    return 0;
}

/**
 * Defines if the log should contain a leading time stamp.
 * Default: Disabled
 *
 * @param enabled If true, a time stamp will be added, false disables the time stamp.
 */
void Logger::setTimeBasedLogging(bool enabled) {
    this->timeBasedLogging = enabled;
}

/**
 * Creates an prefix log entry with a current time stamp without a ending new line.
 *
 * See https://stackoverflow.com/a/37658735/4300087
 */
void Logger::logTimestamp() {
    struct timeval timeNow;
    struct tm* localTime;
    char buffer[30], usecBuffer[6];
    
    gettimeofday(&timeNow, NULL);
    localTime = localtime(&timeNow.tv_sec);
    
    strftime(buffer, 30, "%Y.%m.%d %H:%M:%S", localTime);
    strcat(buffer, ".");
    
    sprintf(usecBuffer, "%d", (int) timeNow.tv_usec);
    strcat(buffer, usecBuffer);

    fprintf(this->logFile, "%s | ", buffer);
}

/**
 * Creates an simple log entry of plain text.
 * The log entry ends always with an new line.
 *
 * @param text The text to log.
 */
void Logger::log(const char* text) {
    if (this->timeBasedLogging)
        logTimestamp();

    fprintf(this->logFile, "%s\n", text);
}

/**
 * Creates an log entry of an method call position.
 * The log entry ends always with an new line.
 *
 * @param file The file name.
 * @param line The line number.
 * @param func The name of the calling function.
 */
void Logger::logM(const char* file, int line, const char* func) {
    if (this->timeBasedLogging)
        logTimestamp();

    fprintf(this->logFile, "%s:%d:%s()\n", file, line, func);
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
void Logger::logF(const char* format, ...) {
    if (this->timeBasedLogging)
        logTimestamp();

    std::va_list argptr;
    va_start(argptr, format);
    vfprintf(this->logFile, format, argptr);
    va_end(argptr);
}
