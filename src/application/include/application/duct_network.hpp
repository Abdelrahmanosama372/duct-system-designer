#pragma once

#include "geometry/block.hpp"
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace dsd::application {

using Metadata = std::map<std::string, std::string>;
enum class NetworkSystemType { Supply, Return, Exhaust };
enum class NetworkMaterial { GalvanizedSteel, StainlessSteel, Aluminum, CarbonSteel };
enum class NetworkItemType { Segment, AirTerminal, Damper };

struct SourcePath {
    std::string source_id; // CAD handle, not a generated DUCT ID
    std::wstring layer;
    std::vector<geometry::Point> vertices; // Only XY is used; processing returns Z = 0.
    bool closed = false;
    bool has_curves = false;
};

struct SourcePiece {
    std::string source_id;
    std::size_t vertex_index;
};

struct LineInfo {
    std::string id;
    geometry::Line raw_line;
    Metadata metadata;
    std::vector<SourcePiece> sources;
};

struct BlockInfo {
    std::string id;
    geometry::Block raw_block;
    Metadata metadata;
};

struct VisualizationRectangle {
    std::string id;
    std::string item_id;
    NetworkItemType type;
    std::array<geometry::Point, 4> corners;
    Metadata metadata;
};

struct NetworkIdsTree {
    std::string root_id;
    // Every item occurs once; an empty parent denotes the root.
    std::map<std::string, std::optional<std::string>> parents;
};

struct NetworkProcessingOptions {
    double connection_tolerance_mm = 5;
    double angle_threshold_degrees = 5;
    double visualization_width_mm = 100;
    double visualization_end_inset_mm = 50;
    double block_visualization_offset_mm = 50;
    double minimum_segment_length_mm = 100;
};

// The UI owns this draft. Processing services return values and never retain
// it.
struct DuctNetwork {
    std::string id;
    std::wstring name;
    NetworkSystemType system_type = NetworkSystemType::Supply;
    NetworkMaterial material = NetworkMaterial::GalvanizedSteel;
    std::string document_key;
    std::optional<SourcePath> root;
    std::vector<SourcePath> lines;
    std::optional<std::wstring> air_terminal_block_name;
    std::optional<std::wstring> damper_block_name;
    std::vector<LineInfo> segmented_lines_infos;
    std::vector<VisualizationRectangle> segmented_lines_vis_rectangles;
    std::vector<BlockInfo> air_terminal_blocks_infos;
    std::vector<VisualizationRectangle> air_terminals_blocks_vis_rectangles;
    std::vector<BlockInfo> damper_blocks_infos;
    std::vector<VisualizationRectangle> damper_blocks_vis_rectangles;
    NetworkIdsTree network_ids_tree;
    NetworkProcessingOptions options;
    short color_index = 11;
};

inline constexpr std::array<short, 12> NetworkColorPool{11,  31,  51,  71,  91,  111,
                                                        131, 151, 171, 191, 211, 231};

std::string network_item_id(const std::string& prefix, std::size_t number);

} // namespace dsd::application
