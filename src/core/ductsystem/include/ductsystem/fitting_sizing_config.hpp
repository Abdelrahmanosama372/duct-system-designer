#pragma once

namespace dsd::ductsystem {

struct FittingSizingConfig {
    const double takeoff_length; // mm
    const double reducer_length; // mm
    const double elbow_inside_radius_ratio; // inside radius / duct width

    FittingSizingConfig(double takeoff_length, double reducer_length,
                        double elbow_inside_radius_ratio);
};

} // namespace dsd::ductsystem
