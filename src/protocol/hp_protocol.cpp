#include "hp_protocol.hpp"
#include <algorithm>
#include <cstddef>

namespace {
std::uint64_t read_u64(const std::uint8_t *p) { std::uint64_t v = 0; for (int i = 0; i < 8; ++i) v = (v << 8) | p[i]; return v; }
void write_u64(std::uint8_t *p, std::uint64_t v) { for (int i = 7; i >= 0; --i) { p[i] = static_cast<std::uint8_t>(v); v >>= 8; } }
}

namespace hp {
bool parse_frame(const std::uint8_t *data, std::size_t size, Frame &frame, std::size_t &consumed) {
    consumed = 0;
    if (size < kHeaderSize) return false;
    const std::uint16_t magic = static_cast<std::uint16_t>((data[0] << 8) | data[1]);
    const std::uint32_t body_size = (static_cast<std::uint32_t>(data[4]) << 24) | (static_cast<std::uint32_t>(data[5]) << 16) |
        (static_cast<std::uint32_t>(data[6]) << 8) | data[7];
    if (magic != kMagic || data[2] != kVersion || body_size > kMaxBodySize) return false;
    if (size < kHeaderSize + body_size) return false;
    if (data[3] < 1 || data[3] > 3) return false;
    frame.type = static_cast<Type>(data[3]); frame.request_id = read_u64(data + 8);
    frame.body.assign(reinterpret_cast<const char *>(data + kHeaderSize), body_size);
    consumed = kHeaderSize + body_size;
    return true;
}

std::vector<std::uint8_t> encode_frame(Type type, std::uint64_t request_id, const std::string &body) {
    if (body.size() > kMaxBodySize) return {};
    std::vector<std::uint8_t> data(kHeaderSize + body.size());
    data[0] = static_cast<std::uint8_t>(kMagic >> 8); data[1] = static_cast<std::uint8_t>(kMagic);
    data[2] = kVersion; data[3] = static_cast<std::uint8_t>(type);
    const std::uint32_t length = static_cast<std::uint32_t>(body.size());
    data[4] = static_cast<std::uint8_t>(length >> 24); data[5] = static_cast<std::uint8_t>(length >> 16);
    data[6] = static_cast<std::uint8_t>(length >> 8); data[7] = static_cast<std::uint8_t>(length);
    write_u64(data.data() + 8, request_id);
    std::copy(body.begin(), body.end(), data.begin() + kHeaderSize);
    return data;
}
}
