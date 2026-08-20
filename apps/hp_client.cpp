#include "hp_protocol.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdint>

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1"; const uint16_t port = argc > 2 ? static_cast<uint16_t>(std::strtoul(argv[2], nullptr, 10)) : 9000;
    const int fd = socket(AF_INET, SOCK_STREAM, 0); if (fd < 0) return 1;
    sockaddr_in address{}; address.sin_family = AF_INET; address.sin_port = htons(port); inet_pton(AF_INET, host, &address.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) return 1;
    const auto request = hp::encode_frame(hp::Type::Echo, 1, "hello from hp client");
    if (send(fd, request.data(), request.size(), 0) != static_cast<ssize_t>(request.size())) return 1;
    std::uint8_t buffer[4096]; const ssize_t count = recv(fd, buffer, sizeof(buffer), 0); close(fd);
    hp::Frame response{}; size_t consumed = 0;
    if (count <= 0 || !hp::parse_frame(buffer, static_cast<size_t>(count), response, consumed)) return 1;
    std::printf("response request_id=%llu body=%s\n", static_cast<unsigned long long>(response.request_id), response.body.c_str()); return 0;
}
