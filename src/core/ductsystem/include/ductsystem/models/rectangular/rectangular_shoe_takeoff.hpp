#pragma once

#include "ductsystem/models/rectangular/rectangular_duct_fitting.hpp"

namespace dsd::ductsystem {

struct RectangularShoeTakeoff : RectangularDuctFitting {
    TakeoffSide side;
    std::optional<int> branch_duct_id;
    std::optional<double> takeoff_length; // mm

    RectangularShoeTakeoff(int id, double flow, PressureLevel pressure_level, Material material,
                          TakeoffSide side, std::optional<double> width = std::nullopt,
                          std::optional<double> height = std::nullopt,
                          std::optional<int> branch_duct_id = std::nullopt,
                          std::optional<double> takeoff_length = std::nullopt);

    [[nodiscard]] std::optional<double> width1() const;
    void accept(SheetMetalComponentVisitor& visitor) override;
};

} // namespace dsd::ductsystem
