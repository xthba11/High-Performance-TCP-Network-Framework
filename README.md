# High-Performance TCP Network Framework

一个面向 Linux x86_64 的高性能 TCP 服务器练习项目：C11/POSIX 网络核心 + C++17 应用层。

当前版本已实现单线程 Reactor MVP：

- 非阻塞 TCP socket、`epoll` LT 事件循环
- 连接建立、读写、半关闭、异常关闭和 fd 回收
- 动态 Buffer，支持 `readv`/非阻塞写
- 自定义长度前缀二进制协议
- C++ Echo Server/Client
- 协议单元测试和 ASan 构建选项

## Linux 构建

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
ctest --test-dir build-debug --output-on-failure
```

```bash
./build-debug/hp_server 9000
./build-debug/hp_client 127.0.0.1 9000
```

当前 Windows 工作区仅保存源码，不能替代 Linux 构建验证。详细说明见 [docs/build_and_test.md](docs/build_and_test.md)。

## 路线

下一步按顺序实现多线程 EventLoop、`eventfd` 唤醒、timerfd 超时、背压、指标和性能测试；暂不把 TLS、HTTP/2、分布式能力混入 MVP。
