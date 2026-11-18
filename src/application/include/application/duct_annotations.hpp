#pragma once
#include "geometry/primitives.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace dsd::application {
enum class AnnotationPlacement { Above, Below, Centered };
enum class AnnotationAttachment { BottomCenter, TopCenter, MiddleCenter };
struct AnnotationSettings {
    bool flow = true, dimensions = true, bod = true;
    double text_height_mm = 150;
    double offset_mm = 100;
    AnnotationPlacement placement = AnnotationPlacement::Above;
    std::wstring layer, text_style;
    void validate() const;
};
struct DuctAnnotation {
    std::string item_id;
    int component_id;
    geometry::Point anchor;
    double rotation;
    AnnotationAttachment attachment;
    std::vector<std::string> lines;
};
struct AnnotationGenerationResult {
    std::string network_id;
    std::uint64_t source_revision = 0, geometry_revision = 0;
    AnnotationSettings settings;
    std::vector<DuctAnnotation> annotations;
};
struct NetworkGenerationResult;
class DuctAnnotationFactory {
public:
    [[nodiscard]] AnnotationGenerationResult create(const NetworkGenerationResult& generation,
                                                    const AnnotationSettings& settings) const;
};
} // namespace dsd::application
