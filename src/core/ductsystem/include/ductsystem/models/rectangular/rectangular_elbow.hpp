#pragma once

#include "ductsystem/models/rectangular/rectangular_duct_fitting.hpp"

namespace dsd::ductsystem {

struct RectangularElbow : RectangularDuctFitting {
    double angle; // Signed radians, 0 < abs(angle) <= pi

    RectangularElbow(int id, double flow, PressureLevel pressure_level, Material material,
                     double angle, std::optional<double> width = std::nullopt,
                     std::optional<double> height = std::nullopt);

    [[nodiscard]] std::optional<double> inside_radius(double ratio) const;
    [[nodiscard]] std::optional<double> centerline_radius(double ratio) const;
    void accept(SheetMetalComponentVisitor& visitor) override;
};

} // namespace dsd::ductsystem
