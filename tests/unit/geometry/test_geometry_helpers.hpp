#pragma once

#include "geometry/rectangular_geometry_builder.hpp"
#include <gtest/gtest.h>

namespace dsd::geometry::test {

inline constexpr auto pressure = ductsystem::PressureLevel::LowPressure;
inline constexpr auto material = ductsystem::Material::GalvanizedSteel;

inline void expect_point(const Point& actual, const Point& expected) {
    EXPECT_NEAR(actual.x, expected.x, 1e-7);
    EXPECT_NEAR(actual.y, expected.y, 1e-7);
}
inline void expect_line(const Line& actual, Point start, Point end) {
    expect_point(actual.start, start);
    expect_point(actual.end, end);
}

} // namespace dsd::geometry::test
