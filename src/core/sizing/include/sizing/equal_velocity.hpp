#pragma once

#include "sizing/duct_sizer.hpp"

namespace dsd::sizing {

class EqualVelocityDuctSizer final : public DuctSizer {
public:
    explicit EqualVelocityDuctSizer(const DuctSizerConfig& config);
    [[nodiscard]] DuctDimension size_duct(const SizingContext& context) const override;

private:
    double velocity_m_s_;
};

} // namespace dsd::sizing
