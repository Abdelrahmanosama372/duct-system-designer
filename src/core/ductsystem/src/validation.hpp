#pragma once

#include "ductsystem/types.hpp"
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

namespace dsd::ductsystem::detail {

inline void positive(double value, const char* name) {
    if (!std::isfinite(value) || value <= 0) {
        throw std::invalid_argument(std::string(name) + " must be positive and finite");
    }
}

inline void dimension(std::optional<double> value, const char* name) {
    if (value) positive(*value, name);
}

inline void validate(Material material) {
    switch (material) {
    case Material::GalvanizedSteel:
    case Material::Aluminum: return;
    }
    throw std::invalid_argument("Invalid material");
}

inline void validate(PressureLevel pressure) {
    switch (pressure) {
    case PressureLevel::LowPressure:
    case PressureLevel::MediumPressure:
    case PressureLevel::HighPressure: return;
    }
    throw std::invalid_argument("Invalid pressure level");
}

inline void validate(SystemType type) {
    switch (type) {
    case SystemType::Supply:
    case SystemType::Return: return;
    }
    throw std::invalid_argument("Invalid system type");
}

inline void validate(TakeoffSide side) {
    switch (side) {
    case TakeoffSide::Left:
    case TakeoffSide::Right: return;
    }
    throw std::invalid_argument("Invalid takeoff side");
}

} // namespace dsd::ductsystem::detail
