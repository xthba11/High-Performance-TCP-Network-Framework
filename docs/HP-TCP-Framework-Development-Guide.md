# High-Performance TCP Network Framework
# Codex 辅助完整开发实施文档

> 中文项目名：高性能 TCP 网络服务框架  
> 英文项目名：High-Performance TCP Network Framework  
> 目标平台：Linux x86_64（Ubuntu 20.04+ / CentOS 7+）  
> 主要语言：C++17、Shell  
> 构建方式：CMake + GCC 9+  
> 建议周期：单人 6～8 周  
> 文档定位：把项目拆成可由 Codex 辅助完成、可逐步验收、可形成 Git 证据链的工程任务，最终交付一个简历可写、面试可讲、压测数据真实的高并发网络库 + 应用示例。

---

## 1. 文档目的

本项目不是"让 Codex 一次生成全部代码"，而是由你控制架构、任务边界和验收标准，Codex 负责局部设计、编码、测试、脚本、文档和问题分析。

完整开发闭环固定为：

```text
明确当前小目标
    ↓
Codex 阅读仓库与相关文档
    ↓
Codex 先输出设计方案，不修改代码
    ↓
人工审查模块边界、接口、线程模型与资源释放
    ↓
Codex 按确认后的设计进行最小范围实现
    ↓
查看 git diff
    ↓
本机编译与单元测试
    ↓
压测与性能剖析（wrk / ab / netperf）
    ↓
异常路径测试（连接暴增、半关闭、超时、OOM）
    ↓
记录测试证据和 worklog
    ↓
提交 Git
```

一个步骤只有同时具备"代码、测试、日志、验收记录和 Git 提交"，才算完成。

---

## 2. 项目最终完成定义

系统必须形成如下完整闭环：

```text
ServerSocket bind/listen
    ↓
Acceptor 接收新连接
    ↓
EventLoop（Reactor 核心）通过 epoll 等待事件
    ↓
Channel 分发可读/可写/错误事件
    ↓
TcpConnection 处理连接生命周期
    ↓
Buffer 进行非阻塞读写（应用层缓冲区）
    ↓
HTTP Request/Response 解析 或 Redis 协议解析
    ↓
业务处理（HTTP 路由 / KV 命令执行）
    ↓
定时器（时间轮）处理连接超时与心跳
    ↓
EventLoopThreadPool 将连接分配到多 IO 线程
    ↓
线程池（TaskThreadPool）处理计算密集型任务
    ↓
优雅关闭（已连接 fd 处理完毕后再退出）
```

项目只有满足以下条件才算真正完成：

- 框架运行在 Linux x86_64 实机，而不只是编译通过。
- 使用非阻塞 IO + epoll ET/LT 模式，不存在阻塞式 read/write。
- Buffer 能正确处理 TCP 粘包、半包、水位线回调。
- 定时器精度 ≤10ms，支持海量定时任务（时间轮 O(1) 插入/删除）。
- 多线程模型下无数据竞争、无死锁，valgrind/ASan 无报错。
- 实现简易 HTTP 服务器（支持静态文件、路由）或 Redis-like KV 存储。
- 压测工具（wrk/ab）能打出 QPS、P99 延迟等量化数据。
- 所有简历指标均来自实测，而不是估算。

---

## 3. 固定技术方案

### 3.1 核心框架

| 层次 | 固定方案 |
|---|---|
| 操作系统 | Linux x86_64，内核 ≥ 3.10 |
| 编译器 | GCC 9+ / Clang 12+ |
| C++ 标准 | C++17 |
| 构建 | CMake 3.16+ |
| 事件驱动 | Reactor 模式，epoll（支持 ET 边沿触发） |
| IO 模型 | 非阻塞 socket + 应用层 Buffer |
| 定时器 | 时间轮（Timing Wheel）或最小堆（二选一，推荐时间轮） |
| 线程模型 | 1 个 Main EventLoop + N 个 IO EventLoop（One Loop Per Thread） |
| 线程池 | 固定大小任务队列 + 条件变量 / eventfd 唤醒 |
| 日志 | 异步日志（参考 muduo AsyncLogging） |
| 协议 | HTTP/1.1 或 Redis RESP 协议（二选一，推荐 HTTP 先做） |

### 3.2 压测与调试

| 项目 | 方案 |
|---|---|
| 压测工具 | wrk2（HTTP）/ redis-benchmark（KV）/ 自写 TCP 客户端 |
| 性能剖析 | perf、valgrind --tool=callgrind、火焰图 |
| 内存检查 | ASan（AddressSanitizer）、valgrind --tool=memcheck |
| 线程检查 | TSan（ThreadSanitizer） |
| 网络工具 | tcpdump、ss、netstat、strace |
| 监控 | /proc 读取 CPU/内存，自定义 Metrics 接口 |

---

## 4. 推荐仓库结构

```text
hp-tcp-framework/
├── AGENTS.md
├── README.md
├── CHANGELOG.md
├── CMakeLists.txt
├── cmake/
│   ├── options.cmake
│   └── sanitizers.cmake
├── src/
│   ├── CMakeLists.txt
│   ├── base/
│   │   ├── timestamp.h / timestamp.cc
│   │   ├── noncopyable.h
│   │   ├── logging.h / logging.cc
│   │   ├── async_logging.h / async_logging.cc
│   │   ├── thread.h / thread.cc
│   │   ├── mutex.h
│   │   ├── condition.h
│   │   ├── thread_pool.h / thread_pool.cc
│   │   └── singleton.h
│   ├── net/
│   │   ├── socket.h / socket.cc
│   │   ├── inet_address.h / inet_address.cc
│   │   ├── channel.h / channel.cc
│   │   ├── event_loop.h / event_loop.cc
│   │   ├── epoll_poller.h / epoll_poller.cc
│   │   ├── poller.h
│   │   ├── acceptor.h / acceptor.cc
│   │   ├── tcp_server.h / tcp_server.cc
│   │   ├── tcp_connection.h / tcp_connection.cc
│   │   ├── buffer.h / buffer.cc
│   │   ├── timer.h / timer.cc
│   │   ├── timer_queue.h / timer_queue.cc
│   │   ├── event_loop_thread.h / event_loop_thread.cc
│   │   └── event_loop_thread_pool.h / event_loop_thread_pool.cc
│   ├── http/
│   │   ├── http_request.h / http_request.cc
│   │   ├── http_response.h / http_response.cc
│   │   ├── http_context.h / http_context.cc
│   │   ├── http_server.h / http_server.cc
│   │   └── http_callback.h
│   ├── kvstore/
│   │   ├── redis_parser.h / redis_parser.cc
│   │   ├── kv_store.h / kv_store.cc
│   │   └── kv_server.h / kv_server.cc
│   └── examples/
│       ├── echo_server.cc
│       ├── http_echo_server.cc
│       ├── http_static_server.cc
│       └── kv_demo.cc
├── tests/
│   ├── unit/
│   │   ├── test_buffer.cc
│   │   ├── test_channel.cc
│   │   ├── test_event_loop.cc
│   │   ├── test_timer.cc
│   │   ├── test_thread_pool.cc
│   │   ├── test_http_parser.cc
│   │   └── test_kv_store.cc
│   ├── integration/
│   │   ├── test_tcp_server.cc
│   │   ├── test_http_server.cc
│   │   └── test_kv_server.cc
│   └── performance/
│       ├── bench_echo.cc
│       ├── bench_http.cc
│       └── bench_kv.cc
├── tools/
│   ├── tcp_client_test.cc
│   ├── http_load_test.sh
│   ├── kv_load_test.sh
│   └── flamegraph.sh
├── protocol/
│   ├── http_protocol.md
│   ├── redis_protocol.md
│   └── test_vectors/
├── config/
│   ├── server.json
│   ├── logging.json
│   └── benchmark.json
├── scripts/
│   ├── build.sh
│   ├── build_debug.sh
│   ├── build_release.sh
│   ├── run_tests.sh
│   ├── run_benchmarks.sh
│   ├── collect_metrics.sh
│   ├── flamegraph.sh
│   └── fault_injection.sh
├── docs/
│   ├── architecture/
│   ├── design/
│   ├── api/
│   ├── tests/
│   ├── reports/
│   ├── logs/
│   ├── worklog.md
│   ├── bug_review.md
│   └── interview_notes.md
└── releases/
```

### 4.1 主程序规划

第一版只保留一个正式示例可执行程序：

```text
http_server          # 或 kv_server，取决于你选择的应用层
```

它负责：
- 启动 TcpServer 和 EventLoopThreadPool
- 注册 HTTP 请求回调（或 KV 命令处理）
- 启动定时器（连接超时检测）
- 设置优雅退出信号处理
- 启动异步日志

每个模块另外提供独立测试程序，先单独跑通后再接入正式程序。

---

## 5. 你与 Codex 的职责边界

### 5.1 你必须负责

- 确认项目范围和模块边界。
- 确认 Reactor 模型、线程模型和 Buffer 设计方案。
- 审查 Codex 给出的设计是否符合 muduo/陈硕风格。
- 审查每次 `git diff`，禁止盲目接受大范围修改。
- 执行编译、压测、valgrind/ASan 检查和性能剖析。
- 判断性能数据是否真实、测试方法是否合理。
- 保存压测日志、火焰图、QPS 数据。
- 能独立讲清楚核心代码和关键问题。

### 5.2 适合交给 Codex 的工作

- 阅读现有仓库并梳理调用关系。
- 生成模块设计文档和接口草案。
- 实现职责清晰的 C++ 小任务（Socket 封装、Buffer 方法等）。
- 补充 CMake、单元测试、压测脚本和文档。
- 根据编译日志进行最小修复。
- 根据 perf/valgrind 日志定位热点和内存问题。
- 对当前 diff 做代码审查和测试覆盖分析。

### 5.3 不允许直接交给 Codex 决定的内容

- 在没有理解 Reactor 模型前让 Codex 选择事件驱动方案。
- 未确认接口前一次性重构整个 net 目录。
- 根据理论值编造 QPS、P99 延迟、内存占用数据。
- 未经审查自动执行可能删除数据或覆盖系统文件的命令。
- 把所有逻辑堆进 `main.cc` 或一个"万能 TcpServer"类。
- 使用阻塞 IO 却声称"高性能"。

---

## 6. Codex 的仓库级规则：AGENTS.md

在仓库根目录创建 `AGENTS.md`，建议内容如下：

```markdown
# Project Instructions

## Project
This repository implements a high-performance TCP network framework
based on Reactor pattern, epoll, non-blocking IO, and application-level
buffering. The target is Linux x86_64 with C++17 and CMake.

## Core Rules
1. Read the relevant design document before modifying code.
2. Work on only the requested module.
3. Do not make unrelated refactors.
4. Before editing, list the files you expect to change.
5. After editing, list every changed file and explain why.
6. C++ code uses C++17 and CMake.
7. No raw pointers for ownership; use std::unique_ptr / std::shared_ptr.
8. No bare new/delete; use RAII for all resources (fd, mutex, epoll).
9. All sockets must be non-blocking.
10. epoll must use EPOLLET (edge-triggered) where applicable.
11. Buffer reads must handle EAGAIN / EWOULDBLOCK correctly.
12. Each thread must support stop, wake-up and join.
13. Each resource must have a deterministic release path.
14. TCP connections must handle half-close (EPOLLRDHUP).
15. Timer callbacks must not block the event loop.
16. Logging must not allocate memory in signal handlers.
17. main.cpp only assembles modules and controls lifecycle.
18. Every failure path must produce a useful log message and error code.
19. Never fabricate benchmark results or performance numbers.
20. All test commands must be reproducible.

## Required Validation
For every code task, provide:
- build command;
- unit or minimal test command;
- expected output;
- one abnormal-path test;
- rollback instructions when system files are changed.

## Code Review Rules
Focus on:
- incorrect ownership or lifetime (fd, Buffer, TcpConnection);
- data races and deadlocks (cross-thread shared state);
- blocking operations in EventLoop (sleep, large memcpy, sync IO);
- unbounded queues or missing backpressure;
- missing EPOLLRDHUP / EPOLLHUP handling;
- Buffer overflow or incorrect watermark logic;
- timer drift or missed expirations;
- thread pool task starvation;
- missing protocol length validation;
- silent packet loss or connection drops;
- fd leak (not closed on error paths);
- memory copies in hot paths (zero-copy where possible).
```

还可在子目录放置更具体的 `AGENTS.md`：

- `src/base/AGENTS.md`：基础工具类、线程、RAII 规则；
- `src/net/AGENTS.md`：网络模块、epoll、Reactor 规则；
- `src/http/AGENTS.md`：HTTP 协议解析和路由规则；
- `src/kvstore/AGENTS.md`：KV 存储引擎和 RESP 协议规则。

---

## 7. 每个模块的统一开发闭环

### 7.1 模块设计 Prompt

```text
请先阅读：
- AGENTS.md
- docs/architecture/system_architecture.md
- 与当前模块相关的现有代码和设计文档

当前任务：为【模块名称】编写设计文档，先不要修改代码。

设计文档必须包含：
1. 模块目标；
2. 模块职责；
3. 明确不属于该模块的职责；
4. 输入与输出；
5. 核心数据结构；
6. 对外接口；
7. 对象所有权与数据生命周期；
8. 线程模型和停止顺序；
9. 正常流程；
10. 错误处理和错误码；
11. 资源释放；
12. 日志字段；
13. 配置项；
14. 最小测试；
15. 异常测试；
16. 性能风险；
17. 与其他模块的依赖；
18. 需要新增或修改的文件；
19. 当前仍无法确认的问题。

只输出设计，不写代码，不假设未提供的系统参数。
```

### 7.2 代码实现 Prompt

```text
设计文档已经人工确认。
请严格按照该设计实现【模块名称】。

要求：
1. 只修改当前模块所必需的文件；
2. 修改前先列出计划修改的文件；
3. 不增加设计外的新功能；
4. 不做无关重构；
5. 保持 C++17 和现有 CMake 风格；
6. 所有线程提供 stop、唤醒和 join；
7. 所有资源具有明确释放路径（RAII）；
8. 所有失败路径输出模块名、错误码和关键参数；
9. 增加最小单元测试或独立测试程序；
10. 完成后执行可在当前环境执行的构建和测试；
11. 最后输出：修改文件、设计对应关系、构建命令、测试命令、预期结果和已知限制。
```

### 7.3 编译错误修复 Prompt

```text
下面是完整编译命令和完整错误日志。
请先定位第一个根因，不要根据后续级联错误大范围修改。

要求：
1. 说明错误属于头文件、类型、链接、ABI 还是 CMake 问题；
2. 检查 C++ 标准是否一致；
3. 给出最小修复方案；
4. 只修改与根因直接相关的文件；
5. 不通过删功能、注释代码或关闭整个模块绕过错误；
6. 修改后重新执行原构建命令。

构建命令：
<粘贴命令>

完整日志：
<粘贴日志>
```

### 7.4 压测与性能分析 Prompt

```text
这是压测命令和完整结果日志。
请按"现象 -> 证据 -> 可能原因 -> 下一条验证命令"的方式分析。

环境：
- Linux x86_64
- GCC 9+
- C++17
- epoll ET mode

请重点判断：
1. QPS 是否达到预期，瓶颈在 CPU、内存还是网络；
2. P99 延迟是否异常，是否存在长尾；
3. Buffer 拷贝次数是否过多；
4. epoll_wait 返回频率是否合理；
5. 线程池任务是否堆积；
6. 定时器是否出现大量到期任务堆积；
7. 是否存在 fd 泄漏或连接未释放；
8. 内存分配是否频繁（malloc/free 热点）。

压测命令：
<粘贴命令>

结果日志：
<粘贴完整日志>
```

### 7.5 Diff 审查 Prompt

```text
请只审查当前 git diff，不修改代码。

按严重程度输出：
- Blocker：会导致崩溃、数据损坏、安全问题或主流程错误；
- Major：线程、资源、协议、性能隐患；
- Minor：可维护性、日志和测试问题。

重点检查：
1. 数据竞争、死锁和线程退出；
2. fd 生命周期（是否提前 close 或 double close）；
3. Buffer 的 prepend/append/retrieve 正确性；
4. epoll EPOLLET 下是否循环 read/write 到 EAGAIN；
5. 半关闭（EPOLLRDHUP）处理；
6. 定时器回调中是否执行了阻塞操作；
7. TCP 粘包/半包处理是否正确；
8. 线程池任务队列是否有界、满时策略；
9. 错误日志是否足够定位；
10. 测试是否覆盖正常和异常路径。

每个问题必须给出文件、位置、原因和最小修改建议。
```

---

## 8. 模块完成标准 Definition of Done

每一个步骤完成时都要检查：

```text
[ ] 已有设计文档
[ ] 设计已经人工审查
[ ] 修改范围与设计一致
[ ] 编译通过（Debug + Release）
[ ] 单元测试通过
[ ] 至少一个异常路径通过
[ ] ASan/valgrind 无报错
[ ] 无明显泄漏、死锁和无法退出的问题
[ ] 关键日志可用于定位问题
[ ] 配置未散落硬编码
[ ] 测试输入可复现
[ ] 测试结果已保存
[ ] worklog 已更新
[ ] README/设计/接口文档已按需更新
[ ] git status 干净并已提交
```

禁止把"Codex 已生成代码"视为完成。

---

## 9. 全项目里程碑

| 里程碑 | 核心结果 | 建议时间 |
|---|---|---:|
| M0 环境基线 | 工具链、CMake、GCC、ASan、压测工具验证 | 第 1 周 |
| M1 Reactor 核心 | EventLoop + epoll + Channel + Acceptor | 第 2～3 周 |
| M2 非阻塞 IO + Buffer | TcpConnection + Buffer + 粘包处理 | 第 4 周 |
| M3 定时器 + 线程池 | 时间轮 + EventLoopThreadPool + TaskThreadPool | 第 5 周 |
| M4 应用层协议 | HTTP 服务器 或 Redis-like KV 存储 | 第 6～7 周 |
| M5 压测与工程化 | 压测报告、火焰图、简历材料、面试准备 | 第 8 周 |

---

# 10. 详细分步开发计划

## 第 0 步：冻结项目范围并建立 Codex 上下文

### 目标

让 Codex 首先理解项目边界，不直接写代码。

### 任务

- 建立根目录 `AGENTS.md`。
- 创建 `docs/architecture/system_architecture.md`。
- 创建 `docs/project_scope.md`，明确本期和二期功能。
- 创建 `docs/worklog.md` 和 `docs/bug_review.md`。
- 固定正式技术栈和命名。
- 创建 `docs/decisions/`，记录架构决策。

### Codex Prompt

```text
请阅读当前仓库和项目计划，先不要写业务代码。
为"High-Performance TCP Network Framework"建立项目上下文文档。

请创建或完善：
1. AGENTS.md；
2. docs/project_scope.md；
3. docs/architecture/system_architecture.md；
4. docs/worklog.md；
5. docs/bug_review.md；
6. docs/decisions/README.md。

必须明确：
- 本期必须完成的闭环（Reactor + epoll + Buffer + 定时器 + 线程池 + HTTP/KV）；
- 暂不实现的功能（SSL/TLS、HTTP/2、集群、持久化 AOF）；
- Reactor 模型的线程分工；
- 数据可以丢旧帧，但 TCP 数据不可静默丢失；
- 所有性能数据必须来自实测。

只建立文档和空目录，不实现功能。
```

### 验收标准

- Codex 能准确描述完整数据流。
- 文档明确区分本期、二期和不做范围。
- 后续新会话只阅读 `AGENTS.md` 和架构文档即可恢复项目上下文。
- Git 产生首个基线提交：`chore: initialize project context and architecture`。

---

## 第 1 步：Linux 环境与工具链基线审计

### 目标

确认开发机真实能力，避免后续由 Codex 根据假设生成无法运行的代码。

### 人工执行命令

```bash
uname -a
cat /etc/os-release
gcc --version
cmake --version
lscpu
free -h
ulimit -n
ulimit -a
# epoll 支持检查
grep -w epoll /proc/kallsyms 2>/dev/null | head
# 压测工具
which wrk || echo "wrk not found"
which ab || echo "ab not found"
which redis-benchmark || echo "redis-benchmark not found"
# 性能工具
which perf || echo "perf not found"
which valgrind || echo "valgrind not found"
```

### Codex Prompt

```text
请根据我提供的环境命令输出生成 docs/reports/environment_baseline.md。

文档必须包含：
1. 系统、内核和架构；
2. GCC 和 CMake 版本；
3. CPU 核心数和内存；
4. ulimit 限制（特别是 open files）；
5. 已安装的压测和性能工具；
6. 已确认项、风险项、缺失项；
7. 每个缺失项对应的下一条验证命令。

禁止根据文件名猜测功能可用，必须区分"发现文件"和"已运行验证"。
```

### 验收标准

- 明确使用 GCC 9+ 和 CMake 3.16+。
- 明确 ulimit -n 是否满足高并发测试需求（建议 ≥ 65535）。
- 明确压测工具是否已安装。

---

## 第 2 步：固定构建系统和工程骨架

### 目标

形成"一条命令构建、一条命令测试"的可重复链路。

### Codex Prompt

```text
请为当前工程设计并实现 CMake 构建基础设施。

新增：
- cmake/options.cmake
- cmake/sanitizers.cmake
- scripts/build.sh
- scripts/build_debug.sh
- scripts/build_release.sh
- scripts/run_tests.sh
- docs/build_and_test.md

要求：
1. 支持 Debug/Release 构建类型；
2. Release 开启 -O2 -DNDEBUG；
3. Debug 开启 -g -O0；
4. 可选开启 ASan、TSan、UBSan；
5. 支持 BUILD_TESTS、BUILD_EXAMPLES 选项；
6. 只创建基础 hello 程序用于验证，不实现业务模块；
7. 脚本使用 set -euo pipefail。
```

### 验收命令

```bash
bash scripts/build_debug.sh
bash scripts/build_release.sh
./build-debug/bin/hello
./build-release/bin/hello
```

### 验收标准

- Debug 和 Release 都能构建成功。
- 产物可正常运行。
- 重新拉取仓库后，仅执行脚本即可重复构建。

---

## 第 3 步：基础工具类（Timestamp、NonCopyable、Logging、Thread）

### 目标

先建立所有模块共同依赖的工程底座。

### 任务

- `Timestamp`：封装 `std::chrono` 或 `gettimeofday`，提供格式化输出。
- `NonCopyable`：禁用拷贝构造和赋值。
- `Logging`：线程安全日志，支持不同级别。
- `Thread`：RAII 线程封装。
- `Mutex` / `Condition`：封装 pthread 原语。

### Codex Prompt

```text
请先设计并实现"基础工具类模块"。

要求：
1. 所有类继承 NonCopyable；
2. Timestamp 使用 monotonic clock；
3. Logging 线程安全，支持 DEBUG/INFO/WARN/ERROR/FATAL；
4. Thread 封装 pthread_create/join，支持 name 和 start/join；
5. Mutex 和 Condition 封装 pthread_mutex_t / pthread_cond_t；
6. 增加单元测试：正常、异常、边界；
7. 不接入网络模块。
```

### 验收标准

- 日志能正确输出时间和级别。
- Thread 能正常启动和 join。
- Mutex/Condition 能正确同步。

---

## 第 4 步：Socket 封装和 InetAddress

### 目标

将底层 socket API 封装为 RAII 风格，避免 fd 泄漏。

### 核心接口

```cpp
class Socket : NonCopyable {
public:
    explicit Socket(int sockfd);
    ~Socket();
    int fd() const;
    void bind(const InetAddress& addr);
    void listen();
    int accept(InetAddress* peeraddr);
    void setNonBlocking();
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setTcpNoDelay(bool on);
    void setKeepAlive(bool on);
    // ...
};
```

### Codex Prompt

```text
请设计并实现 Socket 和 InetAddress 模块。

要求：
1. Socket 持有 fd，析构时 close；
2. 所有 set* 方法在失败时输出日志；
3. InetAddress 支持 IPv4，封装 sockaddr_in；
4. 提供 createNonblocking() 静态方法；
5. 增加单元测试：bind/listen/accept 本地回环；
6. 测试端口复用和非阻塞设置。
```

### 验收标准

- Socket 析构时 fd 被正确关闭。
- setNonBlocking 后 fcntl 验证 O_NONBLOCK 已设置。
- 单元测试通过。

---

## 第 5 步：Channel 和 Poller 抽象

### 目标

建立事件分发的基础抽象。

### 核心设计

```cpp
class Channel {
public:
    void setReadCallback(ReadCallback cb);
    void setWriteCallback(WriteCallback cb);
    void setCloseCallback(CloseCallback cb);
    void setErrorCallback(ErrorCallback cb);
    void enableReading();
    void disableWriting();
    void update();  // 调用 Poller 更新
    void handleEvent(Timestamp receiveTime);
};

class Poller {
public:
    virtual ~Poller() = default;
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
};
```

### Codex Prompt

```text
请设计并实现 Channel 和 Poller 抽象基类。

要求：
1. Channel 不直接操作 epoll，通过 Poller 接口更新；
2. Channel::handleEvent 根据 revents 调用对应回调；
3. Poller 提供 create() 工厂方法，后续可扩展 poll/select；
4. 增加 Channel 单元测试（模拟事件触发）；
5. 不依赖具体 epoll 实现。
```

### 验收标准

- Channel 能正确分发读写事件。
- Poller 接口清晰，可扩展。

---

## 第 6 步：EPollPoller 实现

### 目标

实现基于 epoll 的 Poller 具体类。

### Codex Prompt

```text
请基于 Poller 接口实现 EPollPoller。

要求：
1. 使用 epoll_create1(EPOLL_CLOEXEC)；
2. 支持 EPOLLET 边沿触发；
3. 支持 EPOLLRDHUP 半关闭检测；
4. 每次 poll 返回后正确填充 activeChannels；
5. updateChannel 和 removeChannel 正确调用 epoll_ctl；
6. 处理 EPOLL_CTL_ADD/DEL/MOD 的错误处理；
7. 增加测试：注册/修改/删除事件；
8. 测试 epoll_wait 超时行为。
```

### 验收标准

- epoll 能正确检测 socket 可读/可写。
- EPOLLRDHUP 能被正确触发。
- 单元测试通过。

---

## 第 7 步：EventLoop 核心循环

### 目标

实现 Reactor 模式的核心——事件循环。

### 核心设计

```cpp
class EventLoop {
public:
    void loop();
    void quit();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    void wakeup();  // 跨线程唤醒
private:
    void handleWakeup();
    int createEventfd();
};
```

### Codex Prompt

```text
请设计并实现 EventLoop。

要求：
1. 每个 EventLoop 绑定一个线程（threadId_ 校验）；
2. loop() 中调用 poller_->poll()，然后处理 activeChannels；
3. 支持 runInLoop（当前线程直接执行）和 queueInLoop（跨线程排队）；
4. 使用 eventfd 实现跨线程唤醒；
5. 处理 pendingFunctors_ 时先 swap 到局部变量，减少锁竞争；
6. 增加 wakeup 测试：跨线程调用 runInLoop；
7. 测试 quit 后 loop 退出。
```

### 验收标准

- EventLoop 能正确运行和退出。
- 跨线程 runInLoop 能被正确执行。
- eventfd 唤醒正常。

---

## 第 8 步：Acceptor 和 TcpServer 骨架

### 目标

实现服务器端的连接接收和分发。

### Codex Prompt

```text
请设计并实现 Acceptor 和 TcpServer 骨架。

要求：
1. Acceptor 封装 listen socket，接受新连接后回调 newConnectionCallback；
2. TcpServer 持有 Acceptor 和 EventLoop，管理所有 TcpConnection；
3. TcpServer 提供 setConnectionCallback 和 setMessageCallback；
4. 新连接分配到 EventLoopThreadPool（先单线程，后扩展）；
5. 增加 echo server 示例验证端到端连通性；
6. 测试客户端连接后能收到 echo。
```

### 验收标准

- 客户端能连接服务器。
- 发送数据后能收到 echo。
- 连接关闭后资源正确释放。

### 里程碑

完成本步骤即达到 **M1 Reactor 核心**。

---

## 第 9 步：Buffer 和 TcpConnection

### 目标

实现应用层缓冲区，正确处理非阻塞读写和 TCP 粘包。

### 核心设计

```cpp
class Buffer {
public:
    size_t readableBytes() const;
    size_t writableBytes() const;
    void append(const void* data, size_t len);
    void retrieve(size_t len);
    void retrieveAll();
    ssize_t readFd(int fd, int* savedErrno);   // 非阻塞读
    ssize_t writeFd(int fd, int* savedErrno);  // 非阻塞写
    // 水位线回调
    void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t mark);
};
```

### Codex Prompt

```text
请设计并实现 Buffer 和 TcpConnection。

要求：
1. Buffer 内部使用 std::vector<char>，支持自动扩容；
2. readFd 使用 readv 分散读，减少一次 memcpy；
3. writeFd 在 EAGAIN 时停止写，等待 EPOLLOUT；
4. TcpConnection 处理 onMessage 回调，将数据放入 Buffer；
5. 支持高水位线回调（防止发送过快）；
6. 正确处理 EPOLLRDHUP（半关闭）；
7. 增加 Buffer 单元测试：append/retrieve/readFd/writeFd；
8. 测试 TCP 粘包和半包场景。
```

### 验收标准

- Buffer 能正确处理大量数据读写。
- 半关闭连接能被正确检测。
- 单元测试通过。

### 里程碑

完成本步骤即达到 **M2 非阻塞 IO + Buffer**。

---

## 第 10 步：定时器（时间轮）

### 目标

实现 O(1) 插入/删除的定时器，用于连接超时和心跳。

### 核心设计

```cpp
class TimerWheel {
public:
    using TimerCallback = std::function<void()>;
    TimerWheel(EventLoop* loop, int tickMs = 10, int wheelSize = 600);
    TimerId addTimer(TimerCallback cb, int timeoutMs, int intervalMs = 0);
    void cancel(TimerId timerId);
private:
    void onTick();  // 每次 tick 推进指针
    std::vector<std::vector<TimerEntry>> wheel_;
    int index_;
};
```

### Codex Prompt

```text
请设计并实现 TimerWheel 定时器。

要求：
1. 基于 EventLoop 的 timerfd 驱动 tick；
2. 时间轮 O(1) 插入和删除；
3. 支持一次性定时器和重复定时器；
4. 定时器回调中不得执行阻塞操作；
5. 增加测试：定时触发、重复触发、取消；
6. 测试精度：误差 ≤ 10ms。
```

### 验收标准

- 定时器能准确触发。
- 取消定时器后不再触发。
- 精度满足要求。

---

## 第 11 步：线程池（EventLoopThreadPool + TaskThreadPool）

### 目标

实现 IO 线程池和任务线程池。

### Codex Prompt

```text
请设计并实现 EventLoopThreadPool 和 TaskThreadPool。

要求：
1. EventLoopThreadPool：创建 N 个 IO 线程，每个线程一个 EventLoop；
2. TcpServer 新连接按 round-robin 分配到 IO 线程；
3. TaskThreadPool：固定大小线程池，执行计算密集型任务；
4. 任务队列有界，满时策略可配置（阻塞/丢弃/拒绝）；
5. 增加测试：多线程并发提交任务；
6. 测试线程池停止时所有任务完成或清理。
```

### 验收标准

- IO 线程池能均匀分配连接。
- 任务线程池能正确执行任务。
- 停止时所有线程能 join。

### 里程碑

完成本步骤即达到 **M3 定时器 + 线程池**。

---

## 第 12 步：HTTP 服务器或 Redis-like KV

### 目标

基于框架实现完整应用，证明框架可用性。

### 方案 A：HTTP 服务器

```text
支持：
- HTTP/1.1 请求解析（Request Line + Headers + Body）
- 路由注册（GET / POST）
- 静态文件服务
- 简单的 JSON API
```

### 方案 B：Redis-like KV

```text
支持：
- RESP 协议解析
- GET / SET / DEL / EXISTS / INCR / DECR
- 内存存储（std::unordered_map）
- 简单过期机制
```

### Codex Prompt（以 HTTP 为例）

```text
请基于当前框架实现简易 HTTP 服务器。

要求：
1. 实现 HttpRequest、HttpResponse、HttpContext 解析器；
2. 支持 GET/POST 方法；
3. 支持 Content-Length 和 Transfer-Encoding: chunked；
4. 路由支持静态文件（指定 root 目录）；
5. 支持自定义路由回调；
6. 增加测试：curl 发送请求验证响应；
7. 测试大请求体、非法请求、keep-alive。
```

### 验收标准

- curl 能正常访问 HTTP 服务器。
- 静态文件能正确返回。
- 压测能打出 QPS 数据。

### 里程碑

完成本步骤即达到 **M4 应用层协议**。

---

## 第 13 步：压测、性能优化和简历数据

### 目标

获得可用于简历和面试的真实量化结果。

### 压测方案

```bash
# HTTP 压测
wrk -t4 -c1000 -d30s http://localhost:8080/
wrk -t4 -c1000 -d30s -s post.lua http://localhost:8080/api

# TCP echo 压测
# 使用自写客户端或 redis-benchmark（KV 模式）
```

### Codex Prompt

```text
请为当前系统增加性能埋点并生成压测报告。

要求：
1. 记录每个请求的处理时延（P50/P95/P99）；
2. 记录 QPS、吞吐量（MB/s）；
3. 记录 CPU 使用率、内存占用；
4. 使用 perf 生成火焰图；
5. 对比优化前后的数据；
6. 输出 docs/reports/performance_report.md。

禁止编造数据，所有数字必须来自实测。
```

### 基础验收目标

| 指标 | 基础目标 | 优化目标 |
|---|---:|---:|
| QPS（echo） | ≥ 50,000 | ≥ 100,000 |
| QPS（HTTP 静态） | ≥ 10,000 | ≥ 30,000 |
| P99 延迟 | ≤ 5ms | ≤ 2ms |
| 长连接数 | 10,000+ | 50,000+ |
| 内存/连接 | ≤ 4KB | ≤ 2KB |

### 验收标准

- 每个数字都有测试环境、命令、样本数和原始数据。
- 报告区分平均值与 P95/P99。
- 优化前后使用同一输入和同一配置。

### 里程碑

完成本步骤即达到 **M5 压测与工程化**。

---

## 第 14 步：发布、部署、演示和简历材料

### 目标

形成可交付、可复现、可面试讲解的完整项目。

### Codex Prompt

```text
请基于当前已经通过测试的仓库完成发布整理，不增加新功能。

生成或完善：
1. README.md；
2. docs/architecture/system_architecture.md；
3. docs/build_and_test.md；
4. docs/api/ 目录（如有）；
5. docs/tests/test_cases.md；
6. docs/reports/performance_report.md；
7. docs/interview_notes.md；
8. CHANGELOG.md；
9. scripts/package_release.sh；
10. releases/manifest.json。

要求：
- 所有命令必须与实际程序参数一致；
- 版本、依赖写入 manifest；
- 简历指标只引用性能报告中的真实值；
- 提供完整回滚和卸载方法。
```

### 最终验收清单

```text
[ ] 全新目录可按 README 构建
[ ] 所有单元测试通过
[ ] HTTP 服务器或 KV 服务器可正常启动
[ ] 压测数据真实可复现
[ ] 火焰图已生成
[ ] 简历描述中的每个数字均可追溯
```

---

## 11. 每日实际使用 Codex 的方式

每天只推进一个可在当天闭环的小目标。

### 上午

```text
1. 阅读上一日 worklog 和未解决问题。
2. git status 确保工作区状态明确。
3. 明确今天唯一主目标和不做范围。
4. 让 Codex 只做仓库阅读与设计。
5. 人工审查设计。
```

### 下午

```text
6. 让 Codex 按设计实现。
7. 查看 git diff --stat 和 git diff。
8. 构建并执行单元测试。
9. 运行压测或集成测试。
10. 返回完整日志给 Codex 做最小修复。
11. 进行异常路径测试。
```

### 晚上

```text
12. 更新 docs/worklog.md。
13. 保存命令、日志、截图和测试数据。
14. 让 Codex 对当前 diff 做一次只读审查。
15. 修复 Blocker/Major 问题。
16. Git 提交。
17. 写下一步入口条件。
```

---

## 12. Worklog 模板

```markdown
## YYYY-MM-DD：模块名称

### 今日目标

### 相关设计文档

### 修改文件

### 实际执行命令

### 正常路径结果

### 异常路径结果

### 压测结果

### 性能数据

### 遇到的问题

### 根因与修复

### 未解决问题

### Git commit

### 下一步入口条件
```

---

## 13. Git 提交和分支建议

### 分支

```text
main                  可演示、已验收版本
feature/reactor       事件循环和 epoll
feature/buffer        Buffer 和非阻塞 IO
feature/timer         定时器
feature/thread-pool   线程池
feature/http          HTTP 服务器
feature/kv            KV 存储
test/performance      压测和性能优化
```

单人开发也建议每个大模块使用短生命周期分支，避免 Codex 大范围改动后无法回退。

### 提交示例

```text
chore: initialize project context and build system
feat(base): add Timestamp, NonCopyable, Logging
feat(net): add Socket RAII wrapper
feat(reactor): add Channel, Poller, EPollPoller
feat(reactor): add EventLoop with eventfd wakeup
feat(server): add Acceptor and TcpServer skeleton
feat(io): add Buffer with readv/writev
feat(connection): add TcpConnection lifecycle
feat(timer): add TimerWheel with timerfd
feat(thread): add EventLoopThreadPool and TaskThreadPool
feat(http): add HTTP request/response parser
feat(http): add static file serving and routing
test(perf): add wrk benchmark and flamegraph
docs: add performance report and interview notes
```

### 每次提交前

```bash
git status
git diff --check
git diff --stat
cmake --build build-debug --parallel
ctest --test-dir build-debug --output-on-failure
cmake --build build-release --parallel
./build-release/bin/echo_server &
wrk -t4 -c1000 -d10s http://localhost:8080/
```

---

## 14. Codex 使用中的常见错误

### 错误 1：一次要求完成整个项目

结果通常是目录很多、代码看似完整，但依赖、线程、接口全部未经验证。

正确做法：一次完成一个可单独测试的模块。

### 错误 2：没有先让 Codex 阅读仓库

Codex 可能重复实现已有类、破坏接口或使用错误路径。

正确做法：先要求列出相关文件、数据流和拟修改文件。

### 错误 3：只粘贴最后几行编译错误

后续错误常是第一个错误引发的级联结果。

正确做法：提供完整命令和从第一个 error 开始的完整日志。

### 错误 4：在本地编译成功就认为可用

编译成功 ≠ 运行时正确，更 ≠ 高性能。

正确做法：必须运行测试、压测、valgrind/ASan 全链路验证。

### 错误 5：让 Codex 猜测性能数据

Codex 无法替代真实环境测量。

正确做法：让 Codex 生成埋点和分析脚本，你负责实测。

### 错误 6：为了"看起来快"使用无界队列

IO 或任务一旦变慢，内存会持续增长，延迟也越来越大。

正确做法：IO 队列有界，业务事件可靠落盘。

### 错误 7：让 EventLoop 做阻塞操作

结果是事件分发卡顿和"无响应"。

正确做法：EventLoop 只做事件分发，耗时工作在独立线程。

### 错误 8：直接接受 Codex 的 QPS 结论

Codex 可能基于理论值估算，而非实测。

正确做法：你必须亲自跑 wrk 并记录数据。

---

## 15. 关键风险与专门处理

| 风险 | 处理策略 |
|---|---|
| epoll LT 模式导致惊群 | 使用 EPOLLET + 循环 read/write 到 EAGAIN |
| Buffer 内存拷贝过多 | 使用 readv 分散读、zero-copy 优化 |
| 定时器精度不足 | timerfd + 时间轮，tick 间隔 10ms |
| 多线程 fd 竞争 | One Loop Per Thread，fd 绑定到固定线程 |
| 连接泄漏 | RAII 管理 TcpConnection，析构时 close |
| 半关闭未处理 | 监听 EPOLLRDHUP，正确调用 shutdown |
| 线程池任务堆积 | 有界队列 + 拒绝策略 + 监控 |
| 压测数据不真实 | 多次测量取中位数，排除 warm-up 影响 |
| Codex 大范围误改 | 小分支、小任务、先设计、审查 diff |

---

## 16. 建议的 8 周安排

| 周 | 主要步骤 | 周末验收物 |
|---:|---|---|
| 1 | 0～3 | 环境基线、构建系统、基础工具类 |
| 2 | 4～6 | Socket、Channel、EPollPoller |
| 3 | 7～8 | EventLoop、Acceptor、TcpServer 骨架 |
| 4 | 9 | Buffer、TcpConnection、echo 验证 |
| 5 | 10～11 | 定时器、线程池 |
| 6 | 12 | HTTP 服务器或 KV 存储 |
| 7 | 13 | 压测、性能优化、火焰图 |
| 8 | 14 | 发布、文档、简历材料 |

遇到 epoll、线程竞争或性能瓶颈问题时，应使用预留缓冲周，不要删减测试和文档来"赶进度"。

---

## 17. 最终演示脚本

```text
1. 展示项目架构图（Reactor + 多线程）。
2. 展示 EventLoop、epoll、Buffer 的核心代码。
3. 启动 HTTP 服务器，curl 访问返回结果。
4. 展示 wrk 压测命令和实时 QPS 输出。
5. 展示 P50/P95/P99 延迟数据。
6. 展示火焰图，指出热点函数。
7. 展示 ASan/valgrind 无内存泄漏。
8. 展示 10,000 长连接下的内存占用。
9. 展示定时器精度测试。
10. 展示简历上的量化指标及其原始数据。
```

---

## 18. 项目完成后的简历数据采集表

| 指标 | 测试值 | 测试方法 | 原始证据 |
|---|---:|---|---|
| QPS（echo） | 待实测 | wrk -t4 -c1000 -d30s | 文件路径 |
| QPS（HTTP 静态） | 待实测 | wrk -t4 -c1000 -d30s | 文件路径 |
| P99 延迟 | 待实测 | wrk 输出 | 文件路径 |
| 长连接数 | 待实测 | 压测客户端 | 文件路径 |
| 内存/连接 | 待实测 | RSS / 连接数 | 文件路径 |
| 定时器精度 | 待实测 | 误差统计 | 文件路径 |
| 编译时间 | 待实测 | time cmake --build | 文件路径 |
| ASan 结果 | 待实测 | asan.log | 文件路径 |

只有填写了该表并能打开原始证据后，才能把数字写进简历。

---

## 19. 第一周立即执行清单

```text
[ ] 创建仓库和 AGENTS.md
[ ] 固定项目范围与系统架构
[ ] 执行环境审计命令
[ ] 保存环境原始日志
[ ] 固定 GCC 9+ 和 CMake 3.16+
[ ] 建立 build-debug 和 build-release
[ ] 运行 hello 程序
[ ] 验证 ulimit -n ≥ 65535
[ ] 安装 wrk / ab / redis-benchmark
[ ] 安装 perf / valgrind
[ ] 输出 environment_baseline.md
[ ] 完成第一个可重复构建脚本
```

第一周没有完成这些基线验证前，不让 Codex 开始批量生成业务模块。

---

## 20. 最终原则

```text
Codex 负责加速阅读、设计、编码、测试和整理；
你负责架构、审查、验证和真实数据。

先小模块，后集成；
先设计，后编码；
先独立验证，后接主程序；
先正常路径，后异常路径；
先保留证据，后写简历指标。
```

一个真正适合校招和面试展示的项目，不是代码文件数量多，而是你能展示：

- 为什么选择 Reactor 而不是 Proactor；
- epoll ET 和 LT 的区别和选择依据；
- Buffer 如何正确处理 TCP 粘包和半包；
- 时间轮如何实现 O(1) 定时器；
- 如何避免多线程下的数据竞争；
- 如何定位性能瓶颈（火焰图解读）；
- 如何保证连接不泄漏、内存不增长；
- 如何通过压测数据证明系统的高性能。
