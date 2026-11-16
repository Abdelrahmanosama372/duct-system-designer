#pragma once
namespace Autocad::VisualizationSelection {
void Initialize();
void Shutdown();
// Defer drawing deselection and focus handoff until the drawing command returns.
void CompletePropertyApply();
} // namespace Autocad::VisualizationSelection
