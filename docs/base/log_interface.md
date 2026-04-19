# Base Logging Interface

## Scope

This document describes the logging API in:

- src/base/Log.h
- src/base/Log.cpp

It focuses on C++20 usage, cross-platform behavior (Windows and Linux), and test verification in tests/base.

## C++20 Highlights

The logging entry macros now rely on std::source_location to capture file and line without manual parameters.

- std::source_location::current() is passed through LogEvent::Create(...)
- caller file and line are captured automatically

## Core Macros

### Stream-style macros

- LOG_DEBUG(logger)
- LOG_INFO(logger)
- LOG_WARN(logger)
- LOG_ERROR(logger)
- LOG_FATAL(logger)

Generic form:

- LOG_LEVEL(logger, level)

Usage example:

```cpp
auto logger = LOG_NAME("app.main");
logger->setLevel(Gyanis::base::LogLevel::DEBUG);

LOG_INFO(logger) << "service started";
LOG_ERROR(logger) << "code=" << 500;
```

### Format-style macros

- LOG_FMT_DEBUG(logger, fmt, ...)
- LOG_FMT_INFO(logger, fmt, ...)
- LOG_FMT_WARN(logger, fmt, ...)
- LOG_FMT_ERROR(logger, fmt, ...)
- LOG_FMT_FATAL(logger, fmt, ...)

Generic form:

- LOG_FMT_LEVEL(logger, level, ...)

Usage example:

```cpp
LOG_FMT_DEBUG(logger, "task %s retry=%d", "sync", 3);
```

### Logger lookup helpers

- LOG_ROOT(): root logger
- LOG_NAME(name): named logger from LoggerManager

## Log Levels

Enum: Gyanis::base::LogLevel::Level

- UNKNOW
- DEBUG
- INFO
- WARN
- ERROR
- FATAL

Helpers:

- LogLevel::ToString(Level)
- LogLevel::FromString(std::string)

## Event and Logger APIs

## LogEvent

- constructor accepts file, line, thread id, timestamp
- static LogEvent::Create(logger, level, source_location)
- format(const char* fmt, ...)
- format(const char* fmt, va_list)
- getFile(), getLine(), getThreadId(), getTime(), getContent()

Behavior note:

- format(...) uses an internal dynamic buffer strategy
- va_list handling is safe for repeated vsnprintf calls

## LogEventWrap

- RAII helper
- writes the log in destructor via logger->log(...)

## Logger

- setLevel(...) / getLevel()
- addAppender(...) / delAppender(...) / clearAppenders()
- setFormatter(pattern string or formatter object)
- log(...) and convenience methods debug/info/warn/error/fatal

## Appenders

## StdoutLogAppender

- writes formatted output to std::cout

## FileLogAppender

- writes to file
- reopen() refreshes file stream (throttled in log path)

Path behavior:

- if the configured path has a directory part, directories are created as needed
- plain filename (for example "app.log") is supported without trying to create an empty directory

## Formatter and Pattern Tokens

Default pattern:

```text
%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n
```

Supported tokens:

- %d{...}: datetime (strftime format supported)
- %t: thread id
- %m: message
- %p: level
- %c: logger name
- %f: file name
- %l: line
- %n: newline
- %T: tab
- %%: literal percent sign

Invalid or broken patterns are marked as formatter errors by LogFormatter.

## Cross-Platform Notes

## Thread id and time source

- thread id: std::hash<std::thread::id>
- timestamp: std::chrono::system_clock in seconds

## Local time conversion

- Windows: localtime_s
- Linux/Unix: localtime_r

## Synchronization

- logger/appender manager locks use std::mutex and std::scoped_lock
- no platform-specific pthread header is required in this module

## Stress Tests (Catch2)

Location:

- tests/base/test_log_stress.cpp

Covered scenarios:

- formatted debug macro is callable
- file appender supports plain file name
- long formatted payload is preserved
- benchmark path for repeated stream logging

## Build and run tests

Recommended configuration style from this project:

```bash
cmake -S . -B debug-ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TEST=ON -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=G:/Codes/CLionProjects/Gyanis/conan_provider.cmake
cmake --build debug-ninja --target test_base_log_stress -j 4
ctest --test-dir debug-ninja -R test_base_log_stress --output-on-failure
```

Expected result:

- test_base_log_stress: 1/1 passed

## Quick Integration Example

```cpp
#include "base/Log.h"

int main() {
    auto logger = LOG_NAME("demo");
    logger->setLevel(Gyanis::base::LogLevel::DEBUG);

    logger->clearAppenders();
    logger->addAppender(std::make_shared<Gyanis::base::StdoutLogAppender>());
    logger->addAppender(std::make_shared<Gyanis::base::FileLogAppender>("logs/demo.log"));

    LOG_INFO(logger) << "hello from stream API";
    LOG_FMT_WARN(logger, "warn code=%d", 42);
    return 0;
}
```
