#pragma once
#include "autocad/ui/platform/ObjectArx.h"

std::unique_ptr<CAdUiPalette> MakeNetworkPanel();
std::unique_ptr<CAdUiPalette> MakePropertiesPanel();
std::unique_ptr<CAdUiPalette> MakeReportsPanel();
std::unique_ptr<CAdUiPalette> MakeProjectInformationPanel();
std::unique_ptr<CAdUiPalette> MakeDrawingGenerationPanel();
void DisconnectNetworkPanel();
void ResetProjectInformationState();
