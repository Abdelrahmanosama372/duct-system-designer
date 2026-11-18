#pragma once
#include "ductsystem/duct_system.hpp"
#include "sizing/duct_sizer.hpp"

namespace dsd::ductsystem {
// Each pass is separate so insertion never changes a traversal being sized.
class DuctSystemSizer {
public:
    void size_ducts(DuctSystem &system, const sizing::DuctSizer &sizer) const;
    void insert_reducers(DuctSystem &system) const;
    void size_fittings(DuctSystem &system) const;
};
} // namespace dsd::ductsystem
