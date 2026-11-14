#pragma once

#include "ductsystem/fitting_sizing_config.hpp"
#include "ductsystem/types.hpp"

namespace dsd::ductsystem {

struct DuctSystemConfig {
    const Material material;
    const PressureLevel pressure_level;
    const FittingSizingConfig fittings;
    const SystemType system_type;

    DuctSystemConfig(Material material, PressureLevel pressure_level,
                     FittingSizingConfig fittings, SystemType system_type);
};

} // namespace dsd::ductsystem
