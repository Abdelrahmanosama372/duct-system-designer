#pragma once
#include "application/project_information.hpp"
#include "application/registered_network.hpp"
#include <functional>
#include <string_view>

namespace dsd::application::persistence {
inline constexpr std::uint32_t SchemaVersion = 1;
enum class OutputGroup { Geometry, Visualization, Annotation };
enum class EntityKind { Line, Polyline, Hatch, Text };
struct LayerDefinition {
    std::wstring name, linetype = L"Continuous";
    short color = 7;
    bool off = false, frozen = false, locked = false;
    std::uint32_t rgb = 0;
    bool true_color = false;
    int lineweight = -3;
    int transparency = 255;
    double pattern_length = 0;
    std::vector<double> dashes;
};
struct TextStyleDefinition {
    std::wstring name, font_file, big_font_file;
    double height = 0, width_factor = 1, oblique_angle = 0;
    bool vertical = false;
    int flags = 0;
};
struct EntityDefinition {
    std::string handle;
    OutputGroup group = OutputGroup::Geometry;
    EntityKind kind = EntityKind::Line;
    LayerDefinition layer;
    TextStyleDefinition style;
    std::wstring linetype = L"Continuous";
    short color = 256;
    std::uint32_t rgb = 0;
    bool true_color = false;
    int transparency = 255;
    bool transparency_by_layer = false, transparency_by_block = false;
    bool visible = true;
    std::vector<geometry::Point> points;
    std::vector<double> bulges;
    bool closed = false;
    std::wstring text;
    double text_height = 0, text_width = 0, rotation = 0;
    int attachment = 0;
    double linetype_scale = 1, pattern_length = 0;
    std::vector<double> dashes;
    std::string boundary_handle;
    std::wstring hatch_pattern = L"SOLID";
    double hatch_scale = 1, hatch_angle = 0;
    std::wstring tag_app;
    std::vector<std::wstring> tag_strings;
    std::optional<int> component_id;
};
struct NetworkRecord {
    RegisteredNetwork registered;
    std::string space_handle;
    VisualizationTextFields visualization_fields;
    bool visualization_visible = true, show_segments = true;
    std::vector<EntityDefinition> entities;
};
struct DrawingHeader {
    std::optional<ProjectInformation> project;
    int next_network_id = 1;
};
class NetworkRecordCodec {
  public:
    static std::string encode_network(const NetworkRecord &record);
    static NetworkRecord decode_network(std::string_view data);
    static std::string encode_header(const DrawingHeader &header);
    static DrawingHeader decode_header(std::string_view data);
};
struct RecoveryPlan {
    std::vector<std::string> missing_sources;
    std::vector<std::size_t> missing_entities;
    [[nodiscard]] bool consistent() const { return missing_sources.empty(); }
};
class NetworkRecoveryService {
  public:
    RecoveryPlan plan(const NetworkRecord &record,
                      const std::function<bool(const std::string &, bool)> &source_exists,
                      const std::function<bool(const EntityDefinition &)> &output_exists) const;
};
} // namespace dsd::application::persistence
