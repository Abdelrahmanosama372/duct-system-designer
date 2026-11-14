# Network draft and visualization

The UI owns the unregistered `DuctNetwork` through `NetworkDraftSession::network`.
`DsdApplication::prepare_network` processes a value and returns the prepared value;
it does not retain the draft. Registration and analyzers are deferred, so Register
stays disabled and the registered list starts empty.

## Responsibilities

- `NetworkDiscovery`: same-layer endpoint connectivity, with an adjustable 5 mm
  tolerance. Interior crossings are ignored. Each source is visited once.
- `NetworkSegmentor`: splits at equipment insertion points and turns exceeding
  5 degrees, preserves branch junctions, and merges smaller turns into a chord.
  Final ducts at or below 100 mm cause an exception.
- `VisualizationRectangleFactory`: creates portable rectangle geometry and
  attaches the item's metadata and identity.
- `DsdApplication`: owns these services and coordinates equipment matching,
  segmentation, ID tree construction, and rectangle creation.
- AutoCAD `NetworkInteraction`: reads drawing entities, queues selection in a
  command context, and creates drawing boundaries and hatches.

All network geometry is XY only. Source Z does not participate in distance,
connectivity, segmentation, equipment matching, or rectangle construction.
Prepared source paths, blocks, and rectangle corners have Z = 0. Curved
polylines are rejected.

## Geometry and metadata

Segment rectangles are 100 mm wide and inset 50 mm from each endpoint. Equipment
rectangles expand the drawing block's XY bounds by 50 mm on every side.
Dimensions and tolerances are in `NetworkProcessingOptions` in millimetres.
The adapter converts supported drawing units to millimetres before processing.
Unitless drawings are treated as millimetres.

| Item | Local ID | Default metadata |
|---|---|---|
| Duct | `DUCT-001` | `flow=0`, `width=AC`, `height=AC` |
| Air terminal | `AT-001` | `flow=0`, neck `width=none`, neck `height=none`, `face_size=none` |
| Damper | `VCD-001` | `damper_type=none`, `blade_arrangement=none`, `width=AC`, `height=AC` |

Templates match original block names, including dynamic block definitions.
Only matching blocks whose insertion points lie within the connection tolerance
of a discovered edge are included. Equipment selections are optional.

The ID tree visits each item once and skips loop back-edges. Unchanged items
retain their IDs and metadata during refresh. New items receive subsequent IDs.

AutoCAD draws closed polylines and associative SOLID hatches on the exact layer
`Visualization Rectangles`, using the network's light indexed color and 50%
transparency. Both entities carry `DSD_NETWORK_VIS` XData with network,
visualization, item, and type IDs. `ResolveVisualization` reads this identity;
`DUCT_VIS_INFO` allows manual inspection of either entity.

Refreshing replaces the current network's preview entities. Reset leaves its
existing drawing rectangles in place, as agreed. Drawing-close cleanup discards
in-memory entity references. Full network persistence is not implemented.
Flow, BOD, dimension annotations, and properties-panel selection integration are
not part of this step.

## Verification

Portable GoogleTests cover discovery, loops, crossing exclusion, tolerance,
angle thresholds, equipment cuts, minimum duct length, stable metadata and IDs,
rectangle bounds, equipment defaults, and XY-only end-to-end processing.
The Windows native harness checks selection states, green borders, reset,
empty startup, and blocked registration. Windows checks must be run on Windows;
the Linux build does not compile MFC or ObjectARX.

For an AutoCAD smoke test:

1. Build the Release ARX with ObjectARX 2027 and load it into AutoCAD 2027.
2. Open `NETWORK_PANEL`. Confirm the registered list is empty and equipment
   selection is disabled before choosing a root.
3. Draw connected LINE/straight POLYLINE entities on one layer, plus crossing
   and other-layer lines. Select the root and inspect the discovered rectangles.
4. Repeat with different source Z coordinates; the XY results must stay identical.
5. Pick terminal and damper templates with insertion points on the network.
   Confirm green borders, segmentation, equipment offsets, and item identities
   using `DUCT_VIS_INFO` on boundaries and hatches.
6. Refresh after drawing edits, cancel a selection, and reset. Confirm Register
   remains disabled and no registered cards appear.

## Files added or edited for this step

Application and geometry:

- `CMakeLists.txt`
- `src/CMakeLists.txt`
- `src/core/geometry/include/geometry/block.hpp`
- `src/application/CMakeLists.txt`
- `src/application/include/application/application.hpp`
- `src/application/include/application/duct_network.hpp`
- `src/application/include/application/network_discovery.hpp`
- `src/application/include/application/network_segmentor.hpp`
- `src/application/include/application/visualization_rectangle_factory.hpp`
- `src/application/src/network_processing.cpp`
- `src/application/src/network_discovery.cpp`
- `src/application/src/network_segmentor.cpp`
- `src/application/src/network_geometry.hpp`
- `src/application/src/network_identity.hpp`
- `src/application/src/network_equipment.hpp`
- `src/application/src/network_equipment.cpp`
- `src/application/src/network_topology.hpp`
- `src/application/src/network_topology.cpp`
- `src/application/src/visualization_rectangle_factory.cpp`

AutoCAD adapter and UI:

- `src/adapters/autocad/CMakeLists.txt`
- `src/adapters/autocad/build-msvc.cmd`
- `src/adapters/autocad/include/autocad/database/NetworkInteraction.h`
- `src/adapters/autocad/src/database/NetworkInteraction.cpp`
- `src/adapters/autocad/include/autocad/ui/controls/UiButton.h`
- `src/adapters/autocad/include/autocad/ui/model/ApplicationSession.h`
- `src/adapters/autocad/src/ui/model/ApplicationSession.cpp`
- `src/adapters/autocad/include/autocad/ui/model/NetworkActions.h`
- `src/adapters/autocad/src/ui/model/NetworkActions.cpp`
- `src/adapters/autocad/include/autocad/ui/model/NetworkSession.h`
- `src/adapters/autocad/src/ui/model/NetworkSession.cpp`
- `src/adapters/autocad/src/ui/model/ProjectInformation.cpp`
- `src/adapters/autocad/include/autocad/ui/panels/NetworkPanel.h`
- `src/adapters/autocad/src/ui/panels/NetworkPanel.cpp`
- `src/adapters/autocad/include/autocad/ui/panels/ReportsPanel.h`
- `src/adapters/autocad/include/autocad/ui/panels/DrawingGenerationPanel.h`
- `src/adapters/autocad/src/commands/UiCommands.cpp`

Tests and documentation:

- `tests/CMakeLists.txt`
- `tests/application/network_test_helpers.hpp`
- `tests/application/test_network_discovery.cpp`
- `tests/application/test_network_segmentor.cpp`
- `tests/application/test_visualization_rectangle_factory.cpp`
- `tests/application/test_network_processing.cpp`
- `src/adapters/autocad/tests/NativeUiChecks.cpp`
- `src/adapters/autocad/tests/run-msvc.cmd`
- `docs/network-visualization.md`

Existing README changes and other unrelated workspace files are outside this step.
