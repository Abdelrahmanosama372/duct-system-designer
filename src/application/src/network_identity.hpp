#pragma once
#include <string>
#include <vector>

namespace dsd::application::detail {
// New identities never reuse an identity still present in the previous result.
template <class Info> std::size_t next_item_number(const std::vector<Info>& previous) {
    std::size_t next = 1;
    for (const auto& item : previous) {
        const auto suffix = item.id.substr(item.id.rfind('-') + 1);
        const auto value = static_cast<std::size_t>(std::stoull(suffix));
        if (value >= next)
            next = value + 1;
    }
    return next;
}
} // namespace dsd::application::detail
