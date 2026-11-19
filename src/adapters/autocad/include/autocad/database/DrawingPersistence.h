#pragma once
#include "application/network_persistence.hpp"
namespace Autocad::Persistence {
void Initialize();
void Shutdown();
void RecoverCurrentDrawing();
void RequireConsistentSources(const std::string &network_id);
void ForgetOutputs(const std::string &network_id, dsd::application::persistence::OutputGroup group);
} // namespace Autocad::Persistence
