#include "hp_protocol.hpp"
#include <cassert>
#include <cstdint>
#include <string>

int main() {
    const auto encoded = hp::encode_frame(hp::Type::Echo, 42, "abc");
    hp::Frame frame{}; size_t consumed = 0;
    assert(!hp::parse_frame(encoded.data(), 5, frame, consumed));
    assert(hp::parse_frame(encoded.data(), encoded.size(), frame, consumed));
    assert(consumed == encoded.size() && frame.request_id == 42 && frame.body == "abc");
    auto joined = encoded; joined.insert(joined.end(), encoded.begin(), encoded.end());
    assert(hp::parse_frame(joined.data(), joined.size(), frame, consumed) && consumed == encoded.size());
    joined[0] = 0; assert(!hp::parse_frame(joined.data(), joined.size(), frame, consumed));
    return 0;
}
