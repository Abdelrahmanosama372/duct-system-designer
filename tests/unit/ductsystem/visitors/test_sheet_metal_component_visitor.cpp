#include "../test_helpers.hpp"
#include "ductsystem/visitors/sheet_metal_component_visitor.hpp"
#include "ductsystem/models/rectangular/rectangular_elbow.hpp"
#include "ductsystem/models/rectangular/rectangular_shoe_takeoff.hpp"
#include "ductsystem/models/rectangular/rectangular_reducer.hpp"
#include "ductsystem/models/rectangular/rectangular_endcap.hpp"
#include <stdexcept>

namespace dsd::ductsystem::test {
namespace {

class ComponentKindVisitor final : public SheetMetalComponentVisitor {
public:
    double kind = 0;
    void visit_rectangular_duct(RectangularDuct&) override { kind = 1; }
    void visit_rectangular_elbow(RectangularElbow&) override { kind = 2; }
    void visit_rectangular_shoetakeoff(RectangularShoeTakeoff&) override { kind = 3; }
    void visit_rectangular_reducer(RectangularReducer&) override { kind = 4; }
    void visit_rectangular_endcap(RectangularEndcap&) override { kind = 5; }
};

void expect_dispatch(SheetMetalComponent& component, double expected_kind) {
    ComponentKindVisitor visitor;
    component.accept(visitor);
    EXPECT_EQ(visitor.kind, expected_kind);
    visitor.kind = 0;
    visitor.visit(component);
    EXPECT_EQ(visitor.kind, expected_kind);
}

TEST(ComponentVisitorTest, DispatchesDuctThroughBaseReference) {
    RectangularDuct component{0, 100, pressure, material};
    expect_dispatch(component, 1);
}
TEST(ComponentVisitorTest, DispatchesElbowThroughBaseReference) {
    RectangularElbow component{0, 100, pressure, material, 1};
    expect_dispatch(component, 2);
}
TEST(ComponentVisitorTest, DispatchesShoeThroughBaseReference) {
    RectangularShoeTakeoff component{0, 100, pressure, material, TakeoffSide::Left};
    expect_dispatch(component, 3);
}
TEST(ComponentVisitorTest, DispatchesReducerThroughBaseReference) {
    RectangularReducer component{0, 100, pressure, material};
    expect_dispatch(component, 4);
}
TEST(ComponentVisitorTest, DispatchesEndcapThroughBaseReference) {
    RectangularEndcap component{0, 100, pressure, material};
    expect_dispatch(component, 5);
}

TEST(ComponentVisitorTest, RejectsUnsupportedBaseComponents) {
    ComponentKindVisitor visitor;
    SheetMetalComponent base{0, 100, pressure, material};
    RectangularDuctFitting fitting{1, 100, pressure, material};
    EXPECT_THROW((void)visitor.visit(base), std::invalid_argument);
    EXPECT_THROW((void)visitor.visit(fitting), std::invalid_argument);
}

} // namespace
} // namespace dsd::ductsystem::test
