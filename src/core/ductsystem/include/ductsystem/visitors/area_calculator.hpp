#pragma once

#include "ductsystem/fitting_sizing_config.hpp"
#include "ductsystem/visitors/sheet_metal_component_visitor.hpp"
#include <stdexcept>
#include <optional>

namespace dsd::ductsystem {

class AreaCalculationError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

// Holds the latest component area in m^2; dimensions are in mm.
// No per-ID cache or total is accumulated.
class AreaCalculatorVisitor final : public SheetMetalComponentVisitor {
public:
    explicit AreaCalculatorVisitor(const FittingSizingConfig& config);

    void visit_rectangular_duct(RectangularDuct& component) override;
    void visit_rectangular_elbow(RectangularElbow& component) override;
    void visit_rectangular_shoetakeoff(
        RectangularShoeTakeoff& component) override;
    void visit_rectangular_reducer(RectangularReducer& component) override;
    void visit_rectangular_endcap(RectangularEndcap& component) override;

    [[nodiscard]] double area_m2() const;

private:
    const FittingSizingConfig config_;
    std::optional<double> area_;
};

} // namespace dsd::ductsystem
