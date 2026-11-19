#include "application/network_persistence.hpp"
#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace dsd::application::persistence {
namespace {
constexpr std::size_t MaxBytes = 64 * 1024 * 1024, MaxItems = 1000000;
template <class T> struct Vector : std::false_type {};
template <class T, class A> struct Vector<std::vector<T, A>> : std::true_type {};
template <class T> struct Array : std::false_type {};
template <class T, std::size_t N> struct Array<std::array<T, N>> : std::true_type {};
template <class T> struct Optional : std::false_type {};
template <class T> struct Optional<std::optional<T>> : std::true_type {};
template <class T> struct Map : std::false_type {};
template <class K, class V, class C, class A> struct Map<std::map<K, V, C, A>> : std::true_type {};
template <class A, class T> void fields(A &, T &);
std::vector<std::uint32_t> codepoints(const std::wstring &text) {
    std::vector<std::uint32_t> result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        std::uint32_t c = static_cast<std::uint32_t>(text[i]);
        if constexpr (sizeof(wchar_t) == 2) {
            if (c >= 0xd800 && c <= 0xdbff) {
                if (++i == text.size())
                    throw std::invalid_argument("Incomplete Unicode surrogate");
                const auto low = static_cast<std::uint32_t>(text[i]);
                if (low < 0xdc00 || low > 0xdfff)
                    throw std::invalid_argument("Invalid Unicode surrogate");
                c = 0x10000 + ((c - 0xd800) << 10) + (low - 0xdc00);
            }
        }
        if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            throw std::invalid_argument("Invalid Unicode scalar");
        result.push_back(c);
    }
    return result;
}
std::wstring wide(const std::vector<std::uint32_t> &points) {
    std::wstring result;
    for (auto c : points) {
        if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            throw std::invalid_argument("Invalid Unicode scalar");
        if constexpr (sizeof(wchar_t) == 2) {
            if (c > 0xffff) {
                c -= 0x10000;
                result.push_back(static_cast<wchar_t>(0xd800 + (c >> 10)));
                result.push_back(static_cast<wchar_t>(0xdc00 + (c & 1023)));
                continue;
            }
        }
        result.push_back(static_cast<wchar_t>(c));
    }
    return result;
}
class Writer {
  public:
    static constexpr bool reading = false;
    std::string bytes;
    template <class... T> void operator()(T &...x) { (value(x), ...); }
    void scalar(std::uint64_t x) {
        for (int i = 0; i < 8; ++i)
            bytes.push_back(static_cast<char>((x >> (8 * i)) & 255));
    }
    template <class T> void value(T &x) {
        using U = std::remove_cv_t<T>;
        if constexpr (std::is_enum_v<U>)
            scalar(static_cast<std::uint64_t>(x));
        else if constexpr (std::is_integral_v<U>)
            scalar(static_cast<std::uint64_t>(x));
        else if constexpr (std::is_same_v<U, double>) {
            if (!std::isfinite(x))
                throw std::invalid_argument("Cannot persist non-finite numbers");
            scalar(std::bit_cast<std::uint64_t>(x));
        } else if constexpr (std::is_same_v<U, std::string>) {
            scalar(x.size());
            bytes.append(x);
        } else if constexpr (std::is_same_v<U, std::wstring>) {
            auto p = codepoints(x);
            value(p);
        } else if constexpr (Optional<U>::value) {
            bool has = x.has_value();
            value(has);
            if (has)
                value(*x);
        } else if constexpr (Vector<U>::value) {
            scalar(x.size());
            for (auto &e : x)
                value(e);
        } else if constexpr (Map<U>::value) {
            scalar(x.size());
            for (auto &[k, v] : x) {
                value(k);
                value(v);
            }
        } else if constexpr (Array<U>::value) {
            for (auto &e : x)
                value(e);
        } else
            fields(*this, x);
    }
};
template <class T> T empty_value() {
    if constexpr (std::is_same_v<T, geometry::Line> || std::is_same_v<T, geometry::Primitive2D>)
        return geometry::Line{{}, {}};
    else if constexpr (std::is_same_v<T, LineInfo>)
        return {"", {{}, {}}, {}, {}};
    else if constexpr (std::is_same_v<T, DuctInfo>)
        return {"", 0, {{}, {}}, 0, {}, {}, {}, {}};
    else
        return T{};
}
class Reader {
    std::string_view bytes_;
    std::size_t position_ = 0;

  public:
    static constexpr bool reading = true;
    explicit Reader(std::string_view bytes) : bytes_(bytes) {
        if (bytes.size() > MaxBytes)
            throw std::invalid_argument("Persistent record exceeds size limit");
    }
    template <class... T> void operator()(T &...x) { (value(x), ...); }
    std::uint64_t scalar() {
        if (bytes_.size() - position_ < 8)
            throw std::invalid_argument("Truncated persistent record");
        std::uint64_t x = 0;
        for (int i = 0; i < 8; ++i)
            x |= static_cast<std::uint64_t>(static_cast<unsigned char>(bytes_[position_++]))
                 << (8 * i);
        return x;
    }
    std::size_t count() {
        const auto n = scalar();
        if (n > MaxItems || n > (bytes_.size() - position_) / 8)
            throw std::invalid_argument("Invalid persistent collection length");
        if (n > (bytes_.size() - position_) / 8)
            throw std::invalid_argument("Truncated persistent collection");
        return static_cast<std::size_t>(n);
    }
    void finish() const {
        if (position_ != bytes_.size())
            throw std::invalid_argument("Unexpected trailing persistent data");
    }
    template <class T> void value(T &x) {
        if constexpr (std::is_enum_v<T>) {
            int v{};
            value(v);
            x = static_cast<T>(v);
        } else if constexpr (std::is_integral_v<T>) {
            const auto n = scalar();
            if constexpr (std::is_signed_v<T>) {
                const auto v = std::bit_cast<std::int64_t>(n);
                if (v < std::numeric_limits<T>::lowest() || v > std::numeric_limits<T>::max())
                    throw std::invalid_argument("Persistent integer overflow");
                x = static_cast<T>(v);
            } else {
                if (n > std::numeric_limits<T>::max())
                    throw std::invalid_argument("Persistent integer overflow");
                x = static_cast<T>(n);
            }
        } else if constexpr (std::is_same_v<T, double>) {
            x = std::bit_cast<double>(scalar());
            if (!std::isfinite(x))
                throw std::invalid_argument("Invalid persistent number");
        } else if constexpr (std::is_same_v<T, std::string>) {
            const auto n = scalar();
            if (n > MaxBytes || n > bytes_.size() - position_)
                throw std::invalid_argument("Invalid persistent string length");
            x = std::string(bytes_.substr(position_, static_cast<std::size_t>(n)));
            position_ += static_cast<std::size_t>(n);
        } else if constexpr (std::is_same_v<T, std::wstring>) {
            std::vector<std::uint32_t> p;
            value(p);
            x = wide(p);
        } else if constexpr (Optional<T>::value) {
            bool has{};
            value(has);
            if (has) {
                x.emplace();
                value(*x);
            } else
                x.reset();
        } else if constexpr (Vector<T>::value) {
            const auto n = count();
            x.clear();
            x.reserve((std::min)(n, std::size_t{4096}));
            for (std::size_t i = 0; i < n; ++i) {
                x.push_back(empty_value<typename T::value_type>());
                value(x.back());
            }
        } else if constexpr (Map<T>::value) {
            const auto n = count();
            x.clear();
            for (std::size_t i = 0; i < n; ++i) {
                typename T::key_type k{};
                typename T::mapped_type v{};
                value(k);
                value(v);
                if (!x.emplace(std::move(k), std::move(v)).second)
                    throw std::invalid_argument("Duplicate persistent map key");
            }
        } else if constexpr (Array<T>::value) {
            for (auto &e : x)
                value(e);
        } else
            fields(*this, x);
    }
};
using namespace ductsystem;
struct ComponentRecord {
    int kind = 0, id = 0;
    double flow = 0;
    PressureLevel pressure{};
    Material material{};
    std::optional<std::size_t> parent;
    std::optional<double> width, height, length, inlet_width, inlet_height, outlet_width,
        outlet_height;
    std::optional<int> reference, branch;
    double angle = 0;
    ElbowOrientation orientation{};
    TakeoffSide side{};
};
struct SystemRecord {
    std::string name;
    Material material{};
    PressureLevel pressure{};
    SystemType type{};
    double takeoff = 0, reducer = 0, radius = 0;
    std::vector<ComponentRecord> components;
};
SystemRecord capture(const DuctSystem &system) {
    const auto &c = system.config();
    SystemRecord out{system.system_name(),
                     c.material,
                     c.pressure_level,
                     c.system_type,
                     c.fittings.takeoff_length,
                     c.fittings.reducer_length,
                     c.fittings.elbow_inside_radius_ratio,
                     {}};
    std::map<int, std::size_t> indices;
    for (const auto *node : system.preorder_traversal()) {
        const auto &component = node->component();
        const auto &rect = dynamic_cast<const RectangularSheetMetalComponent &>(component);
        ComponentRecord r;
        r.id = component.id;
        r.flow = component.flow;
        r.pressure = component.pressure_level;
        r.material = component.material;
        r.width = rect.width;
        r.height = rect.height;
        if (auto parent = node->parent().lock())
            r.parent = indices.at(parent->component().id);
        if (const auto *duct = dynamic_cast<const RectangularDuct *>(&component)) {
            r.kind = 0;
            r.length = duct->length;
            r.reference = duct->ref_line_id;
        } else if (const auto *elbow = dynamic_cast<const RectangularElbow *>(&component)) {
            r.kind = 1;
            r.angle = elbow->angle;
            r.orientation = elbow->orientation;
        } else if (const auto *shoe = dynamic_cast<const RectangularShoeTakeoff *>(&component)) {
            r.kind = 2;
            r.length = shoe->takeoff_length;
            r.side = shoe->side;
            r.branch = shoe->branch_duct_id;
        } else if (const auto *reducer = dynamic_cast<const RectangularReducer *>(&component)) {
            r.kind = 3;
            r.length = reducer->length;
            r.inlet_width = reducer->inlet_width;
            r.inlet_height = reducer->inlet_height;
            r.outlet_width = reducer->outlet_width;
            r.outlet_height = reducer->outlet_height;
        } else if (dynamic_cast<const RectangularEndcap *>(&component))
            r.kind = 4;
        else
            throw std::invalid_argument("Unsupported persistent component");
        indices.emplace(r.id, out.components.size());
        out.components.push_back(r);
    }
    return out;
}
std::shared_ptr<const DuctSystem> restore(const SystemRecord &record) {
    std::vector<std::shared_ptr<SheetMetalNode>> nodes;
    std::set<int> ids;
    for (const auto &r : record.components) {
        if (!ids.insert(r.id).second ||
            (nodes.empty() ? r.parent.has_value() : !r.parent || *r.parent >= nodes.size()))
            throw std::invalid_argument("Invalid persistent component tree");
        std::unique_ptr<SheetMetalComponent> component;
        switch (r.kind) {
        case 0:
            component = std::make_unique<RectangularDuct>(r.id, r.flow, r.pressure, r.material,
                                                          r.width, r.height, r.length, r.reference);
            break;
        case 1:
            component = std::make_unique<RectangularElbow>(
                r.id, r.flow, r.pressure, r.material, r.angle, r.width, r.height, r.orientation);
            break;
        case 2:
            component = std::make_unique<RectangularShoeTakeoff>(r.id, r.flow, r.pressure,
                                                                 r.material, r.side, r.width,
                                                                 r.height, r.branch, r.length);
            break;
        case 3:
            component = std::make_unique<RectangularReducer>(
                r.id, r.flow, r.pressure, r.material, r.width, r.height, r.inlet_width,
                r.inlet_height, r.outlet_width, r.outlet_height, r.length);
            break;
        case 4:
            component = std::make_unique<RectangularEndcap>(r.id, r.flow, r.pressure, r.material,
                                                            r.width, r.height);
            break;
        default:
            throw std::invalid_argument("Unknown persistent component kind");
        }
        auto node = std::make_shared<SheetMetalNode>(std::move(component));
        if (r.parent)
            nodes[*r.parent]->add_child(node);
        nodes.push_back(node);
    }
    if (nodes.empty())
        throw std::invalid_argument("Empty persistent component tree");
    return std::make_shared<const DuctSystem>(
        record.name, DuctFittingTree{nodes[0]},
        DuctSystemConfig{record.material,
                         record.pressure,
                         {record.takeoff, record.reducer, record.radius},
                         record.type});
}
template <class A, class T> void fields(A &a, T &x) {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, geometry::Point>) {
        a(x.x, x.y);
        if constexpr (A::reading)
            x.z = 0;
    } else if constexpr (std::is_same_v<U, geometry::Line>)
        a(x.start, x.end, x.line_type, x.id);
    else if constexpr (std::is_same_v<U, geometry::Arc>)
        a(x.start, x.center, x.angle, x.line_type, x.id);
    else if constexpr (std::is_same_v<U, geometry::Primitive2D>) {
        bool arc = std::holds_alternative<geometry::Arc>(x);
        a(arc);
        if constexpr (A::reading) {
            if (arc)
                x = geometry::Arc{{}, {}, 0};
            else
                x = geometry::Line{{}, {}};
        }
        if (arc)
            a(std::get<geometry::Arc>(x));
        else
            a(std::get<geometry::Line>(x));
    } else if constexpr (std::is_same_v<U, geometry::GeometricRepresentation>)
        a(x.component_id, x.primitives, x.layout_skeleton);
    else if constexpr (std::is_same_v<U, SourcePath>)
        a(x.source_id); // Source geometry is never duplicated in NOD.
    else if constexpr (std::is_same_v<U, LineInfo>)
        a(x.id, x.raw_line, x.metadata, x.sources);
    else if constexpr (std::is_same_v<U, VisualizationRectangle>)
        a(x.id, x.item_id, x.type, x.corners, x.metadata);
    else if constexpr (std::is_same_v<U, BlockInfo>)
        a(x.id, x.raw_block.source_id, x.metadata);
    else if constexpr (std::is_same_v<U, NetworkIdsTree>)
        a(x.root_id, x.parents);
    else if constexpr (std::is_same_v<U, SourcePiece>)
        a(x.source_id, x.vertex_index);
    else if constexpr (std::is_same_v<U, NetworkProcessingOptions>)
        a(x.connection_tolerance_mm, x.angle_threshold_degrees, x.visualization_width_mm,
          x.visualization_end_inset_mm, x.block_visualization_offset_mm,
          x.visualization_text_height_mm, x.visualization_text_offset_mm,
          x.minimum_segment_length_mm);
    else if constexpr (std::is_same_v<U, NetworkSizingConfiguration>)
        a(x.method, x.minimum_mm, x.maximum_mm, x.increment_mm, x.aspect_ratio,
          x.head_loss_in_wg_per_100ft, x.velocity_fpm, x.roughness_mm, x.air_density_kg_m3,
          x.dynamic_viscosity_pa_s);
    else if constexpr (std::is_same_v<U, DuctNetwork>)
        a(x.id, x.name, x.system_type, x.material, x.sizing, x.root, x.lines,
          x.air_terminal_block_name, x.damper_block_name, x.segmented_lines_infos,
          x.segmented_lines_vis_rectangles, x.air_terminal_blocks_infos,
          x.air_terminals_blocks_vis_rectangles, x.damper_blocks_infos,
          x.damper_blocks_vis_rectangles, x.network_ids_tree, x.options, x.color_index);
    else if constexpr (std::is_same_v<U, DuctInfo>)
        a(x.id, x.component_id, x.line, x.flow, x.width, x.height, x.parent, x.children);
    else if constexpr (std::is_same_v<U, EquipmentInfo>)
        a(x.block, x.upstream_duct, x.at_duct_end);
    else if constexpr (std::is_same_v<U, NetworkTopology>)
        a(x.ducts, x.air_terminals, x.dampers);
    else if constexpr (std::is_same_v<U, AnnotationSettings>)
        a(x.flow, x.dimensions, x.bod, x.text_height_mm, x.offset_mm, x.placement, x.layer,
          x.text_style);
    else if constexpr (std::is_same_v<U, DuctAnnotation>)
        a(x.item_id, x.component_id, x.anchor, x.rotation, x.attachment, x.lines);
    else if constexpr (std::is_same_v<U, AnnotationGenerationResult>)
        a(x.network_id, x.source_revision, x.geometry_revision, x.settings, x.annotations);
    else if constexpr (std::is_same_v<U, ComponentRecord>)
        a(x.kind, x.id, x.flow, x.pressure, x.material, x.parent, x.width, x.height, x.length,
          x.inlet_width, x.inlet_height, x.outlet_width, x.outlet_height, x.reference, x.branch,
          x.angle, x.orientation, x.side);
    else if constexpr (std::is_same_v<U, SystemRecord>)
        a(x.name, x.material, x.pressure, x.type, x.takeoff, x.reducer, x.radius, x.components);
    else if constexpr (std::is_same_v<U, std::shared_ptr<const DuctSystem>>) {
        bool has = static_cast<bool>(x);
        a(has);
        if (!has) {
            if constexpr (A::reading)
                x.reset();
            return;
        }
        SystemRecord r;
        if constexpr (!A::reading)
            r = capture(*x);
        a(r);
        if constexpr (A::reading)
            x = restore(r);
    } else if constexpr (std::is_same_v<U, NetworkGenerationResult>)
        a(x.network_id, x.source_revision, x.system, x.topology, x.geometry, x.item_components,
          x.geometry_revision, x.annotations);
    else if constexpr (std::is_same_v<U, RegisteredNetwork>)
        a(x.network, x.generation, x.revision, x.geometry_revision, x.annotation_settings,
          x.geometry_settings);
    else if constexpr (std::is_same_v<U, GeometryOutputSettings>)
        a(x.centerlines, x.duct_layer, x.centerline_layer, x.takeoff_length_mm, x.reducer_length_mm,
          x.elbow_inside_radius_ratio);
    else if constexpr (std::is_same_v<U, LayerDefinition>)
        a(x.name, x.linetype, x.color, x.off, x.frozen, x.locked, x.rgb, x.true_color, x.lineweight,
          x.transparency, x.pattern_length, x.dashes);
    else if constexpr (std::is_same_v<U, TextStyleDefinition>)
        a(x.name, x.font_file, x.big_font_file, x.height, x.width_factor, x.oblique_angle,
          x.vertical, x.flags);
    else if constexpr (std::is_same_v<U, EntityDefinition>)
        a(x.handle, x.group, x.kind, x.layer, x.style, x.linetype, x.color, x.rgb, x.true_color,
          x.transparency, x.transparency_by_layer, x.transparency_by_block, x.visible, x.points,
          x.bulges, x.closed, x.text, x.text_height, x.text_width, x.rotation, x.attachment,
          x.linetype_scale, x.pattern_length, x.dashes, x.boundary_handle, x.hatch_pattern,
          x.hatch_scale, x.hatch_angle, x.tag_app, x.tag_strings, x.component_id);
    else if constexpr (std::is_same_v<U, NetworkRecord>)
        a(x.registered, x.space_handle, x.visualization_fields.flow,
          x.visualization_fields.dimensions, x.visualization_fields.bod, x.visualization_visible,
          x.show_segments, x.entities);
    else if constexpr (std::is_same_v<U, ProjectInformation>)
        a(x.project_name, x.project_number, x.building_name, x.project_address, x.client_name,
          x.organization_name, x.author, x.level_name, x.drawing_number, x.revision,
          x.additional_information);
    else if constexpr (std::is_same_v<U, DrawingHeader>)
        a(x.project, x.next_network_id);
    else
        static_assert(sizeof(U) == 0, "Missing persistence schema");
}
std::uint64_t checksum(std::string_view bytes) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char byte : bytes) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}
template <class T> std::string encode(T value) {
    Writer writer;
    std::string magic = "DSD-NOD";
    std::uint32_t version = SchemaVersion;
    writer(magic, version, value);
    writer.scalar(checksum(writer.bytes));
    if (writer.bytes.size() > MaxBytes)
        throw std::invalid_argument("Persistent record exceeds size limit");
    return writer.bytes;
}
template <class T> T decode(std::string_view data) {
    if (data.size() < 8 || data.size() > MaxBytes)
        throw std::invalid_argument("Invalid persistent record size");
    Reader trailer(data.substr(data.size() - 8));
    if (trailer.scalar() != checksum(data.substr(0, data.size() - 8)))
        throw std::invalid_argument("Persistent record checksum mismatch");
    Reader reader(data.substr(0, data.size() - 8));
    std::string magic;
    std::uint32_t version{};
    reader(magic, version);
    if (magic != "DSD-NOD" || version != SchemaVersion)
        throw std::invalid_argument("Unsupported network persistence format/version");
    T value{};
    reader(value);
    reader.finish();
    return value;
}
} // namespace
std::string NetworkRecordCodec::encode_network(const NetworkRecord &record) {
    return encode(record);
}
NetworkRecord NetworkRecordCodec::decode_network(std::string_view data) {
    auto record = decode<NetworkRecord>(data);
    if (record.registered.network.id.empty())
        throw std::invalid_argument("Persistent network ID is empty");
    const auto &network = record.registered.network;
    if (network.system_type != NetworkSystemType::Supply &&
        network.system_type != NetworkSystemType::Return &&
        network.system_type != NetworkSystemType::Exhaust)
        throw std::invalid_argument("Invalid saved system type");
    if (network.material != NetworkMaterial::GalvanizedSteel &&
        network.material != NetworkMaterial::StainlessSteel &&
        network.material != NetworkMaterial::Aluminum &&
        network.material != NetworkMaterial::CarbonSteel)
        throw std::invalid_argument("Invalid saved material");
    std::set<std::string> ids;
    for (const auto &segment : network.segmented_lines_infos)
        if (segment.id.empty() || !ids.insert(segment.id).second)
            throw std::invalid_argument("Invalid saved segment IDs");
    for (const auto &entity : record.entities) {
        const wchar_t *app = entity.group == OutputGroup::Geometry        ? L"DSD_GEOMETRY"
                             : entity.group == OutputGroup::Visualization ? L"DSD_NETWORK_VIS"
                             : entity.group == OutputGroup::Annotation    ? L"DSD_DUCT_ANNOTATIONS"
                                                                          : L"";
        if (entity.tag_app.empty() || entity.handle.empty() || entity.layer.name.empty() ||
            entity.tag_app != app || entity.tag_strings.empty() ||
            entity.tag_strings.front() != std::wstring(network.id.begin(), network.id.end()))
            throw std::invalid_argument("Invalid saved entity ownership");
        switch (entity.kind) {
        case EntityKind::Line:
            if (entity.points.size() != 2)
                throw std::invalid_argument("Invalid saved line");
            break;
        case EntityKind::Polyline:
            if (entity.points.size() < 2 || entity.points.size() != entity.bulges.size())
                throw std::invalid_argument("Invalid saved polyline");
            break;
        case EntityKind::Text:
            if (entity.points.size() != 1 || entity.style.name.empty() || entity.text_height <= 0 ||
                entity.text_width < 0 || entity.attachment < 1 || entity.attachment > 9)
                throw std::invalid_argument("Invalid saved text");
            break;
        case EntityKind::Hatch:
            if (entity.boundary_handle.empty() || entity.hatch_pattern.empty() ||
                entity.hatch_scale <= 0)
                throw std::invalid_argument("Invalid saved hatch");
            break;
        default:
            throw std::invalid_argument("Unknown saved entity kind");
        }
    }
    (void)record.registered.network.sizing.to_sizer_config();
    if (record.registered.geometry_settings)
        record.registered.geometry_settings->validate();
    if (record.registered.annotation_settings)
        record.registered.annotation_settings->validate();
    return record;
}
std::string NetworkRecordCodec::encode_header(const DrawingHeader &header) {
    return encode(header);
}
DrawingHeader NetworkRecordCodec::decode_header(std::string_view data) {
    auto header = decode<DrawingHeader>(data);
    if (header.next_network_id <= 0)
        throw std::invalid_argument("Invalid persistent network counter");
    return header;
}
} // namespace dsd::application::persistence
