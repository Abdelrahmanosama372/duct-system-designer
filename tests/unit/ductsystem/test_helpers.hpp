#pragma once

#include "ductsystem/models/rectangular/rectangular_duct.hpp"
#include "ductsystem/sheet_metal_node.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

namespace dsd::ductsystem::test {

inline constexpr auto pressure = PressureLevel::LowPressure;
inline constexpr auto material = Material::GalvanizedSteel;

inline std::shared_ptr<SheetMetalNode> duct_node(int id) {
    return std::make_shared<SheetMetalNode>(
        std::make_unique<RectangularDuct>(id, 100, pressure, material));
}

inline std::string invalid_number_name(const ::testing::TestParamInfo<double>& info) {
    if (std::isnan(info.param)) return "NaN";
    if (std::isinf(info.param)) {
        return info.param > 0 ? "PositiveInfinity" : "NegativeInfinity";
    }
    return info.param == 0 ? "Zero" : "Negative";
}

} // namespace dsd::ductsystem::test
