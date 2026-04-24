# 日志模块接口文档

## 1. 概述

日志模块位于 `src/base/Log.h` 与 `src/base/Log.cpp`，提供以下能力：

- 按日志级别输出（DEBUG/INFO/WARN/ERROR/FATAL）。
- 支持流式日志与格式化日志两种调用方式。
- 支持可扩展的格式化模板（时间、线程 ID、级别、文件行号等）。
- 支持控制台与文件输出。
- 文件输出采用异步队列 + 后台线程批量刷盘，降低业务线程 I/O 阻塞。

## 2. 快速使用

### 2.1 流式日志

```cpp
auto logger = LOG_NAME("system");
LOG_INFO(logger) << "service started, port=" << 8080;
```

### 2.2 格式化日志

```cpp
auto logger = LOG_NAME("system");
LOG_FMT_INFO(logger, "service started, port=%d", 8080);
LOG_FMT_WARN(logger, "no extra args");
```

### 2.3 根日志器

```cpp
LOG_ERROR(LOG_ROOT()) << "fatal path=" << path;
```

## 3. 核心类型

### 3.1 `LogLevel`

- `ToString(Level)`：日志级别转字符串。
- `FromString(std::string_view)`：字符串转日志级别（大小写不敏感）。

### 3.2 `LogEvent`

日志事件对象，包含：

- 文件名与行号
- 线程 ID
- 时间戳（秒）
- 日志内容缓存流
- 所属 logger 与日志级别

### 3.3 `LogFormatter`

用于将 `LogEvent` 按模板格式化为文本。默认模板：

```text
%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f{32}:%l{4}%T%m%n
```

### 3.4 `LogAppender`

日志输出目标抽象基类，当前实现：

- `StdoutLogAppender`：输出到标准输出。
- `FileLogAppender`：输出到文件（异步）。

`StdoutLogAppender` 支持按日志级别着色：

- 仅日志级别标记着色（如 `[INFO]`、`[DEBUG]`），其余字段保持原样。

- `DEBUG`：青色
- `INFO`：绿色
- `WARN`：黄色
- `ERROR`：红色
- `FATAL`：高亮红色

默认开启，可通过 appender 配置字段 `color: true/false` 控制。

### 3.5 `Logger`

日志器，维护：

- 日志级别
- 格式器
- appender 列表

### 3.6 `LoggerManager`

日志器管理单例，负责：

- 获取 root logger
- 按名称获取/创建 logger
- 导出全部 logger 配置

## 4. 格式化占位符

支持以下占位符：

- `%d{...}`：时间，花括号中可指定 `strftime` 格式。
- `%t`：线程 ID。
- `%p`：日志级别。
- `%c`：logger 名称。
- `%f`：文件名。
- `%l`：行号。
- `%l{N}`：固定宽度行号（右对齐，`N` 为宽度），用于避免 `1/10/100` 位数变化导致后续列错位。
- `%m`：日志正文。
- `%n`：换行。
- `%T`：制表符。
- `%%`：字面量 `%`。

## 5. 异步文件日志线程模型

`FileLogAppender` 采用“生产者-消费者”模型：

- 生产者（业务线程）：将日志任务入队。
- 消费者（后台线程）：按批次从队列取任务并写入文件。

### 5.1 关键参数

可通过 `setAsyncConfig(max_queue_size, flush_batch_size, flush_interval)` 配置：

- `max_queue_size`：队列容量上限。
- `flush_batch_size`：单次批量写入任务数。
- `flush_interval`：队列空闲时的最大等待间隔。

### 5.2 行为语义

- 队列满时，生产者线程阻塞等待可用空间（默认不丢日志）。
- 后台线程析构前会 drain 队列并 flush 文件。
- 文件句柄按周期重开，兼容日志轮转场景。

## 6. 日志模块架构

### 6.1 分层与职责

- 入口层：日志宏（`LOG_DEBUG/INFO/WARN/ERROR/FATAL`、`LOG_FMT_*`）负责采集调用点并创建日志事件。
- 事件层：`LogEvent` 承载文件、行号、线程、时间和正文；`LogEventWrap` 在析构时触发提交。
- 路由层：`Logger` 按级别过滤并分发到多个 appender；`LoggerManager` 统一管理 root 与命名 logger。
- 格式化层：`LogFormatter` 将事件映射为文本，`FormatItem` 负责各占位符输出。
- 输出层：`StdoutLogAppender` 负责控制台输出（支持仅等级标记着色）；`FileLogAppender` 负责异步文件输出。
- 配置层：`Config` + `LogDefine` + 监听器共同完成配置加载、差异比对与热更新。

### 6.2 组件关系图

```
业务代码 (LOG_* / LOG_FMT_* 宏)
         │
         ▼
   LogEventWrap (构造 LogEvent，析构时提交)
         │
         ├──► LogEvent (文件名、行号、线程ID、时间戳、日志内容流)
         │
         ▼
      Logger (名称、级别、Formatter、Appender 列表)
         │
         ├── 级别过滤 ──► 不满足级别则丢弃
         │
         └── 分发至 Appender 列表
                   │
         ┌─────────┴──────────┐
         ▼                    ▼
  StdoutLogAppender      FileLogAppender
         │                    │
         │                    ├─► 异步队列 (生产者-消费者)
         │                    │         │
         │                    │         ▼
         │                    │   后台工作线程
         │                    │         │
         │                    │         ├─ 批量取出日志任务
         │                    │         ├─ 调用 Formatter 格式化
         │                    │         └─ 写入文件 & 周期性 flush
         │                    │                   │
         │                    │                   ▼
         │                    │              日志文件 (轮转兼容)
         │                    │
         └──────────┬─────────┘
                    │
            LogFormatter (模板解析: %d, %t, %p, %c, %f, %l, %m, %n, %T)
                    │
                    └─► 输出文本 (控制台仅等级标记着色)

════════════════════════════════════════════════════════════════════

配置与热更新链路 (独立于日志写入流程)

   logs.yml / Config 变更
            │
            ▼
    LogIniter 监听器 (对比新旧 std::set<LogDefine>)
            │
            ├─► 新增/变更 logger ──► 更新级别、Formatter、Appender
            ├─► 删除 logger      ──► 降级并清空 Appender
            └─► Appender 属性更新 ──► 颜色开关 / 文件路径 / 异步参数

════════════════════════════════════════════════════════════════════

核心管理组件

   LoggerManager (单例)
            │
            ├─► 持有 root logger
            ├─► 名称 → Logger 映射表
            └─► 导出全部 logger 配置
```


```
┌──────────────────────────────────────────────────────────────────┐
│                        业务线程                                   │
│  LOG_INFO(logger) << "msg";   /   LOG_FMT_INFO(logger, "fmt", ..)│
└───────────────────────────────┬──────────────────────────────────┘
                                │ 构造临时对象
                                ▼
┌──────────────────────────────────────────────────────────────────┐
│ LogEventWrap (RAII)                                              │
│  ├─ LogEvent (文件/行号/线程ID/时间戳/内容流)                      │
│  └─ 析构时调用 logger.log(level, event)                           │
└───────────────────────────────┬──────────────────────────────────┘
                                │
                                ▼
┌──────────────────────────────────────────────────────────────────┐
│ Logger (名称: "system" 等)                                        │
│  ├─ 级别检查 (低于当前级别则忽略)                                   │
│  ├─ LogFormatter (将 event 按模板转为字符串)                       │
│  └─ Appender 列表 (遍历调用 log())                                │
└───────────────────────────────┬──────────────────────────────────┘
                │                               │
                ▼                               ▼
┌───────────────────────────┐     ┌─────────────────────────────────┐
│ StdoutLogAppender         │     │ FileLogAppender                 │
│  ├─ 全局互斥锁 (防交错)    │     │  ├─ 异步队列 (容量上限)           │
│  ├─ 仅级别标记着色         │     │  ├─ 入队 (队列满时阻塞)           │
│  └─ 输出至 stdout         │     │  └─ 后台线程                     │
└───────────────────────────┘     │       ├─ 批量取出 (flush_batch)  │
                                  │       ├─ 格式化                  │
                                  │       ├─ 写入文件 & flush        │
                                  │       └─ 周期性重开文件句柄       │
                                  └───────────────┬─────────────────┘
                                                  ▼
                                          ┌───────────────┐
                                          │   日志文件     │
                                          └───────────────┘
```

### 6.4 配置热更新链路

1. 配置系统加载 `logs` 配置项并反序列化为 `std::set<LogDefine>`。
2. 监听器接收 old/new 配置集合并做差异比较。
3. 对新增或变更 logger：更新 logger 级别、formatter 与 appender 列表。
4. 对控制台 appender：应用 `color` 开关；对文件 appender：应用路径与格式器。
5. 对已删除 logger：降级并清空其 appender，避免继续输出。

### 6.5 并发与可靠性要点

- `Logger` 在写日志时缩小锁持有范围，降低多线程竞争。
- 控制台输出使用全局互斥保证单行原子性，避免多线程交叉打印。
- 文件输出采用队列 + 后台线程，业务线程不直接做磁盘 I/O。
- 队列满时阻塞等待，默认保证日志不丢失。
- 析构阶段会 drain 队列并 flush，确保退出前尽量完成落盘。
