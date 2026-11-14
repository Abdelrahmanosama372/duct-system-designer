#pragma once

#include "ductsystem/models/rectangular/rectangular_duct_fitting.hpp"

namespace dsd::ductsystem {

struct RectangularEndcap : RectangularDuctFitting {
    using RectangularDuctFitting::RectangularDuctFitting;
    void accept(SheetMetalComponentVisitor& visitor) override;
};

} // namespace dsd::ductsystem
