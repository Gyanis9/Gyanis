# 日志模块接口文档

## 1. 概述

日志模块位于 `src/base/Log.h` 与 `src/base/Log.cpp`，提供以下能力：

- 按日志级别输出（DEBUG/INFO/WARN/ERROR/FATAL）。
- 支持流式日志与格式化日志两种调用方式。
- 支持可扩展的格式化模板（时间、线程 ID、级别、文件行号等）。
- 支持控制台与文件输出。
- 文件输出采用异步队列 + 后台线程批量刷盘，降低业务线程 I/O 阻塞。

## 2. 兼容性说明

本次重构保持原有宏和调用方式兼容：

- `LOG_DEBUG/INFO/WARN/ERROR/FATAL`
- `LOG_FMT_DEBUG/INFO/WARN/ERROR/FATAL`
- `LOG_ROOT()`
- `LOG_NAME(name)`

同时内部升级到 C++20：

- 使用 `std::source_location` 采集文件与行号。
- 使用 `std::string_view` 优化部分字符串参数路径。
- 使用 `__VA_OPT__` 兼容格式化宏的“有参/无参”场景。

## 3. 快速使用

### 3.1 流式日志

```cpp
auto logger = LOG_NAME("system");
LOG_INFO(logger) << "service started, port=" << 8080;
```

### 3.2 格式化日志

```cpp
auto logger = LOG_NAME("system");
LOG_FMT_INFO(logger, "service started, port=%d", 8080);
LOG_FMT_WARN(logger, "no extra args");
```

### 3.3 根日志器

```cpp
LOG_ERROR(LOG_ROOT()) << "fatal path=" << path;
```

## 4. 核心类型

### 4.1 `LogLevel`

- `ToString(Level)`：日志级别转字符串。
- `FromString(std::string_view)`：字符串转日志级别（大小写不敏感）。

### 4.2 `LogEvent`

日志事件对象，包含：

- 文件名与行号
- 线程 ID
- 时间戳（秒）
- 日志内容缓存流
- 所属 logger 与日志级别

### 4.3 `LogFormatter`

用于将 `LogEvent` 按模板格式化为文本。默认模板：

```text
%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n
```

### 4.4 `LogAppender`

日志输出目标抽象基类，当前实现：

- `StdoutLogAppender`：输出到标准输出。
- `FileLogAppender`：输出到文件（异步）。

### 4.5 `Logger`

日志器，维护：

- 日志级别
- 格式器
- appender 列表

### 4.6 `LoggerManager`

日志器管理单例，负责：

- 获取 root logger
- 按名称获取/创建 logger
- 导出全部 logger 配置

## 5. 格式化占位符

支持以下占位符：

- `%d{...}`：时间，花括号中可指定 `strftime` 格式。
- `%t`：线程 ID。
- `%p`：日志级别。
- `%c`：logger 名称。
- `%f`：文件名。
- `%l`：行号。
- `%m`：日志正文。
- `%n`：换行。
- `%T`：制表符。
- `%%`：字面量 `%`。

## 6. 异步文件日志线程模型

`FileLogAppender` 采用“生产者-消费者”模型：

- 生产者（业务线程）：将日志任务入队。
- 消费者（后台线程）：按批次从队列取任务并写入文件。

### 6.1 关键参数

可通过 `setAsyncConfig(max_queue_size, flush_batch_size, flush_interval)` 配置：

- `max_queue_size`：队列容量上限。
- `flush_batch_size`：单次批量写入任务数。
- `flush_interval`：队列空闲时的最大等待间隔。

### 6.2 行为语义

- 队列满时，生产者线程阻塞等待可用空间（默认不丢日志）。
- 后台线程析构前会 drain 队列并 flush 文件。
- 文件句柄按周期重开，兼容日志轮转场景。

## 7. 性能优化点（本次重构）

- 缩小 `Logger::log` 的锁持有范围，避免在锁内执行 I/O。
- 修复 `va_list` 二次消费未定义行为，保证格式化稳定。
- 文件日志改为异步写入，显著降低调用线程阻塞时间。
- 时间格式化采用线程本地缓存，降低重复 `localtime_r` 开销。
- 避免 `std::endl` 的隐式 flush（改为 `\n`）。

## 8. 注意事项

- `FileLogAppender` 异步线程生命周期与 appender 对象绑定。
- 在进程退出前应确保 logger/appender 正常析构，以完成最终 flush。
- 如果日志文件路径无父目录，模块会直接在当前路径创建/追加文件。

## 9. 最小验证建议

- 构建：`cmake --preset debug`、`cmake --build --preset debug`。
- 测试：`ctest --preset debug --output-on-failure`。
- 重点检查：
  - 格式化宏是否支持无可变参数。
  - 大量日志写入时是否无丢失。
  - 程序退出前文件是否完整落盘。
