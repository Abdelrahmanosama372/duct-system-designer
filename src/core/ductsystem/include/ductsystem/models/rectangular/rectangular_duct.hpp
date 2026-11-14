#pragma once

#include "ductsystem/models/rectangular/rectangular_sheet_metal_component.hpp"

namespace dsd::ductsystem {

struct RectangularDuct : RectangularSheetMetalComponent {
    std::optional<double> length; // mm
    std::optional<int> ref_line_id; // Source geometry identifier only

    RectangularDuct(int id, double flow, PressureLevel pressure_level, Material material,
                    std::optional<double> width = std::nullopt,
                    std::optional<double> height = std::nullopt,
                    std::optional<double> length = std::nullopt,
                    std::optional<int> ref_line_id = std::nullopt);
    void accept(SheetMetalComponentVisitor& visitor) override;
};

} // namespace dsd::ductsystem
