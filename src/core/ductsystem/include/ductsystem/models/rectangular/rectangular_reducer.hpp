#pragma once

#include "ductsystem/models/rectangular/rectangular_duct_fitting.hpp"

namespace dsd::ductsystem {

struct RectangularReducer : RectangularDuctFitting {
    std::optional<double> inlet_width; // mm
    std::optional<double> inlet_height; // mm
    std::optional<double> outlet_width; // mm
    std::optional<double> outlet_height; // mm
    std::optional<double> length; // mm

    RectangularReducer(int id, double flow, PressureLevel pressure_level, Material material,
                       std::optional<double> width = std::nullopt,
                       std::optional<double> height = std::nullopt,
                       std::optional<double> inlet_width = std::nullopt,
                       std::optional<double> inlet_height = std::nullopt,
                       std::optional<double> outlet_width = std::nullopt,
                       std::optional<double> outlet_height = std::nullopt,
                       std::optional<double> length = std::nullopt);
    void accept(SheetMetalComponentVisitor& visitor) override;
};

} // namespace dsd::ductsystem
