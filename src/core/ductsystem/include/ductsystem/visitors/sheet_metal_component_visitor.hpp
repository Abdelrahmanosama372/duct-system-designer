#pragma once

namespace dsd::ductsystem {

struct SheetMetalComponent;
struct RectangularDuct;
struct RectangularElbow;
struct RectangularShoeTakeoff;
struct RectangularReducer;
struct RectangularEndcap;

// Operations on the supported sheet-metal components.
class SheetMetalComponentVisitor {
public:
    virtual ~SheetMetalComponentVisitor() = default;

    void visit(SheetMetalComponent& component);
    virtual void visit_rectangular_duct(RectangularDuct& component) = 0;
    virtual void visit_rectangular_elbow(RectangularElbow& component) = 0;
    virtual void visit_rectangular_shoetakeoff(
        RectangularShoeTakeoff& component) = 0;
    virtual void visit_rectangular_reducer(RectangularReducer& component) = 0;
    virtual void visit_rectangular_endcap(RectangularEndcap& component) = 0;
};

} // namespace dsd::ductsystem
