#pragma once

#include "ductsystem/visitors/sheet_metal_component_visitor.hpp"
#include "geometry/placement_context_manager.hpp"
#include "geometry/rectangular_geometry_builder.hpp"

namespace dsd::ductsystem {

// Manager, builder, and the manager's tree must outlive this visitor.
class GeometryConstructorVisitor final : public SheetMetalComponentVisitor {
public:
    GeometryConstructorVisitor(geometry::PlacementContextManager& context_manager,
                               geometry::RectangularGeometricRepresentationBuilder& builder);
    void construct();
    [[nodiscard]] const geometry::GeometryRepresentations& representations() const {
        return representations_;
    }

    void visit_rectangular_duct(RectangularDuct& component) override;
    void visit_rectangular_elbow(RectangularElbow& component) override;
    void visit_rectangular_shoetakeoff(RectangularShoeTakeoff& component) override;
    void visit_rectangular_reducer(RectangularReducer& component) override;
    void visit_rectangular_endcap(RectangularEndcap& component) override;

private:
    geometry::PlacementContextManager& context_manager_;
    geometry::RectangularGeometricRepresentationBuilder& builder_;
    geometry::GeometryRepresentations representations_;
    void duct(RectangularDuct& component, const geometry::PlacementContext& context);
};

} // namespace dsd::ductsystem
