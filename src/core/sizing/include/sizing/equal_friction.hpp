#pragma once

#include "sizing/duct_sizer.hpp"

namespace dsd::sizing {

class EqualFrictionDuctSizer final : public DuctSizer {
public:
    explicit EqualFrictionDuctSizer(const DuctSizerConfig& config);
    [[nodiscard]] DuctDimension size_duct(const SizingContext& context) const override;
    [[nodiscard]] double find_equivalent_diameter(double flow) const; // CFM -> mm
    [[nodiscard]] double pressure_loss_pa_m(double flow, double diameter) const; // CFM, mm
    [[nodiscard]] double friction_factor(double reynolds, double diameter) const; // mm; Darcy

private:
    double target_pa_m_;
};

} // namespace dsd::sizing
