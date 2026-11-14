#include "application/application.hpp"
#include <algorithm>
#include <cwctype>
#include <stdexcept>

namespace dsd::application {
namespace {
std::wstring name_key(const std::wstring& name) {
    auto first =
        std::find_if_not(name.begin(), name.end(), [](wchar_t c) { return std::iswspace(c); });
    auto last = std::find_if_not(name.rbegin(), name.rend(), [](wchar_t c) {
                    return std::iswspace(c);
                }).base();
    if (first >= last)
        return {};
    std::wstring key(first, last);
    std::transform(key.begin(), key.end(), key.begin(), [](wchar_t c) { return std::towlower(c); });
    return key;
}
} // namespace
NetworkAnalysisResult DsdApplication::analyze_network(const DuctNetwork& network) const {
    return network_analyzer_.analyze(network);
}
void DsdApplication::register_network(DuctNetwork network) {
    const auto key = name_key(network.name);
    if (network.id.empty() || key.empty())
        throw std::invalid_argument("Network ID and name are required");
    const auto analysis = analyze_network(network);
    if (!analysis.ok())
        throw std::invalid_argument(analysis.issues.front().item_id + ": " +
                                    analysis.issues.front().message);
    const auto existing = registered_networks_.find(network.id);
    if (existing != registered_networks_.end() && existing->second.name != network.name)
        throw std::invalid_argument("The name of a registered network cannot be changed");
    for (const auto& [id, saved] : registered_networks_)
        if (id != network.id && name_key(saved.name) == key)
            throw std::invalid_argument("That network name is already registered");
    if (existing == registered_networks_.end()) {
        const auto id = network.id;
        registered_networks_.emplace(id, std::move(network));
    } else {
        using std::swap;
        swap(existing->second, network);
    }
}
const std::map<std::string, DuctNetwork>& DsdApplication::registered_networks() const noexcept {
    return registered_networks_;
}
const DuctNetwork& DsdApplication::registered_network(const std::string& id) const {
    const auto item = registered_networks_.find(id);
    if (item == registered_networks_.end())
        throw std::invalid_argument("Registered network not found");
    return item->second;
}
void DsdApplication::clear_networks() noexcept { registered_networks_.clear(); }
} // namespace dsd::application
