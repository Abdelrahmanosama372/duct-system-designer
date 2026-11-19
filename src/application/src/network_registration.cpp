#include "application/application.hpp"
#include <algorithm>
#include <cwctype>
#include <stdexcept>

namespace dsd::application {
namespace {
std::wstring name_key(const std::wstring &name) {
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
NetworkAnalysisResult DsdApplication::analyze_network(const DuctNetwork &network) const {
    return network_analyzer_.analyze(network);
}
void DsdApplication::register_network(DuctNetwork network) {
    const auto key = name_key(network.name);
    if (network.id.empty() || key.empty())
        throw std::invalid_argument("Network ID and name are required");
    const auto analysis = analyze_network(network);
    (void)network.sizing.to_sizer_config();
    if (!analysis.ok())
        throw std::invalid_argument(analysis.issues.front().item_id + ": " +
                                    analysis.issues.front().message);
    const auto existing = registered_networks_.find(network.id);
    if (existing != registered_networks_.end() && existing->second.network.name != network.name)
        throw std::invalid_argument("The name of a registered network cannot be changed");
    for (const auto &[id, saved] : registered_networks_)
        if (id != network.id && name_key(saved.network.name) == key)
            throw std::invalid_argument("That network name is already registered");
    if (existing == registered_networks_.end()) {
        const auto id = network.id;
        registered_networks_.emplace(id, RegisteredNetwork{std::move(network), std::nullopt});
    } else {
        using std::swap;
        swap(existing->second.network, network);
        existing->second.generation.reset();
        existing->second.missing_source_handles.clear();
        ++existing->second.revision;
    }
}
const std::map<std::string, RegisteredNetwork> &
DsdApplication::registered_networks() const noexcept {
    return registered_networks_;
}
const DuctNetwork &DsdApplication::registered_network(const std::string &id) const {
    return registered_network_record(id).network;
}
const RegisteredNetwork &DsdApplication::registered_network_record(const std::string &id) const {
    const auto item = registered_networks_.find(id);
    if (item == registered_networks_.end())
        throw std::invalid_argument("Registered network not found");
    return item->second;
}
NetworkGenerationResult
DsdApplication::prepare_network_generation(const std::string &id,
                                           const ductsystem::FittingSizingConfig &fittings) const {
    const auto &record = registered_network_record(id);
    if (!record.consistent())
        throw std::invalid_argument("Network has missing source entities");
    auto result = network_geometry_generator_.generate(record.network, fittings);
    result.source_revision = record.revision;
    result.geometry_revision = record.geometry_revision + 1;
    if (record.annotation_settings)
        result.annotations = duct_annotation_factory_.create(result, *record.annotation_settings);
    return result;
}
void DsdApplication::commit_network_generation(NetworkGenerationResult result) {
    const auto &record = registered_network_record(result.network_id);
    if (result.source_revision != record.revision)
        throw std::invalid_argument("Network changed after geometry preparation; regenerate it");
    if (!result.system || result.geometry.empty())
        throw std::invalid_argument("Cannot publish an incomplete generation result");
    if (result.geometry_revision != record.geometry_revision + 1)
        throw std::invalid_argument("Geometry changed after preparation; regenerate it");
    registered_networks_.at(result.network_id).geometry_revision = result.geometry_revision;
    registered_networks_.at(result.network_id).generation = std::move(result);
}
void DsdApplication::clear_network_generation(const std::string &id) {
    (void)registered_network_record(id);
    registered_networks_.at(id).generation.reset();
    registered_networks_.at(id).annotation_settings.reset();
}
void DsdApplication::clear_networks() noexcept { registered_networks_.clear(); }
AnnotationGenerationResult
DsdApplication::prepare_network_annotations(const std::string &id,
                                            const AnnotationSettings &settings) const {
    if (!registered_network_record(id).consistent())
        throw std::invalid_argument("Network has missing source entities");
    const auto &generation = registered_network_record(id).generation;
    if (!generation)
        throw std::invalid_argument("Generate duct geometry before generating annotations");
    return duct_annotation_factory_.create(*generation, settings);
}
void DsdApplication::commit_network_annotations(AnnotationGenerationResult result) {
    const auto &record = registered_network_record(result.network_id);
    if (!record.generation || result.source_revision != record.revision ||
        result.geometry_revision != record.geometry_revision)
        throw std::invalid_argument(
            "Geometry changed after annotation preparation; regenerate annotations");
    result.settings.validate();
    registered_networks_.at(result.network_id).annotation_settings = result.settings;
    registered_networks_.at(result.network_id).generation->annotations = std::move(result);
}
void DsdApplication::clear_network_annotations(const std::string &id) {
    (void)registered_network_record(id);
    auto &record = registered_networks_.at(id);
    record.annotation_settings.reset();
    if (record.generation)
        record.generation->annotations.reset();
}
void GeometryOutputSettings::validate() const {
    (void)ductsystem::FittingSizingConfig{takeoff_length_mm, reducer_length_mm,
                                          elbow_inside_radius_ratio};
    if (duct_layer.empty() || (centerlines && centerline_layer.empty()))
        throw std::invalid_argument("Geometry output requires valid layer names");
}
void DsdApplication::set_geometry_output_settings(const std::string &id,
                                                  GeometryOutputSettings settings) {
    (void)registered_network_record(id);
    settings.validate();
    registered_networks_.at(id).geometry_settings = std::move(settings);
}
void DsdApplication::update_source_consistency(const std::string &id,
                                               std::vector<std::string> missing_handles) {
    (void)registered_network_record(id);
    registered_networks_.at(id).missing_source_handles = std::move(missing_handles);
}
void DsdApplication::restore_networks(std::vector<RegisteredNetwork> networks) {
    std::map<std::string, RegisteredNetwork> restored;
    for (auto &record : networks) {
        if (record.network.id.empty() ||
            !restored.emplace(record.network.id, std::move(record)).second)
            throw std::invalid_argument("Persistent network IDs must be nonempty and unique");
    }
    registered_networks_.swap(restored);
}
} // namespace dsd::application
