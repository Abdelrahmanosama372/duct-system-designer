#pragma once

#include <optional>

namespace dsd::sizing {

struct SizingContext {
    const double flow; // CFM
    const std::optional<double> preferred_width; // mm; a ceiling on selected width

    explicit SizingContext(double flow, std::optional<double> preferred_width = std::nullopt);
};

struct DuctDimension {
    const int width; // mm
    const int height; // mm

    bool operator==(const DuctDimension&) const = default;
};

} // namespace dsd::sizing
