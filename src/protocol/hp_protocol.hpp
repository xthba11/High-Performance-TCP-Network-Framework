#ifndef HP_PROTOCOL_HPP
#define HP_PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace hp {
constexpr std::uint16_t kMagic = 0x4850;
constexpr std::uint8_t kVersion = 1;
constexpr std::size_t kHeaderSize = 16;
constexpr std::size_t kMaxBodySize = 1024 * 1024;
enum class Type : std::uint8_t { Ping = 1, Echo = 2, GetStats = 3 };
struct Frame { Type type; std::uint64_t request_id; std::string body; };
bool parse_frame(const std::uint8_t *data, std::size_t size, Frame &frame, std::size_t &consumed);
std::vector<std::uint8_t> encode_frame(Type type, std::uint64_t request_id, const std::string &body);
}

#endif
