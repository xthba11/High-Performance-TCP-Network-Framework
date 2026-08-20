#include "hp_protocol.hpp"
#include "hp_server.h"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <unordered_map>

static hp_server_t *g_server = nullptr;
static std::unordered_map<int, std::vector<std::uint8_t>> g_input;
static void stop_handler(int) { if (g_server != nullptr) hp_server_stop(g_server); }

static void on_connect(hp_connection_t *connection, void *) { std::printf("connected fd=%d\n", hp_connection_fd(connection)); g_input[hp_connection_fd(connection)] = {}; }
static void on_close(hp_connection_t *connection, void *) { const int fd = hp_connection_fd(connection); std::printf("closed fd=%d\n", fd); g_input.erase(fd); }
static void on_message(hp_connection_t *connection, const void *data, size_t size, void *) {
    const int fd = hp_connection_fd(connection);
    auto &input = g_input[fd];
    const auto *bytes = static_cast<const std::uint8_t *>(data);
    input.insert(input.end(), bytes, bytes + size);
    size_t offset = 0;
    while (offset < input.size()) {
        hp::Frame frame{}; size_t consumed = 0;
        if (!hp::parse_frame(input.data() + offset, input.size() - offset, frame, consumed)) {
            if (input.size() - offset < hp::kHeaderSize) break;
            const char error[] = "invalid frame"; hp_connection_send(connection, error, sizeof(error) - 1); input.clear(); return;
        }
        const hp::Type response_type = frame.type == hp::Type::Ping ? hp::Type::Ping : hp::Type::Echo;
        const std::string body = frame.type == hp::Type::Ping ? "PONG" : frame.body;
        const auto response = hp::encode_frame(response_type, frame.request_id, body);
        hp_connection_send(connection, response.data(), response.size());
        offset += consumed;
    }
    if (offset > 0) input.erase(input.begin(), input.begin() + static_cast<std::ptrdiff_t>(offset));
}

int main(int argc, char **argv) {
    const uint16_t port = argc > 1 ? static_cast<uint16_t>(std::strtoul(argv[1], nullptr, 10)) : 9000;
    hp_server_config_t config{"0.0.0.0", port, 128, 2 * 1024 * 1024};
    g_server = hp_server_create(&config);
    if (g_server == nullptr) { std::fprintf(stderr, "hp_server_create failed\n"); return 1; }
    hp_server_set_callbacks(g_server, on_connect, on_message, on_close, nullptr);
    std::signal(SIGINT, stop_handler); std::signal(SIGTERM, stop_handler); std::signal(SIGPIPE, SIG_IGN);
    if (hp_server_start(g_server) != 0) { std::fprintf(stderr, "hp_server_start failed: %s\n", std::strerror(errno)); hp_server_destroy(g_server); return 1; }
    std::printf("listening on 0.0.0.0:%u\n", port); hp_server_run(g_server); hp_server_destroy(g_server); return 0;
}
