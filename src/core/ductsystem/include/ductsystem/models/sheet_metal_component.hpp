#pragma once

#include "ductsystem/types.hpp"

namespace dsd::ductsystem {

class SheetMetalComponentVisitor;

struct SheetMetalComponent {
    int id;
    double flow; // CFM; zero is allowed before airflow assignment
    PressureLevel pressure_level;
    Material material;

    SheetMetalComponent(int id, double flow, PressureLevel pressure_level, Material material);
    virtual ~SheetMetalComponent() = default;
    virtual void accept(SheetMetalComponentVisitor& visitor);
};

} // namespace dsd::ductsystem
