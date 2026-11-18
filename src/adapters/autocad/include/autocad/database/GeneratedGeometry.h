#pragma once
#include "application/registered_network.hpp"
#include "autocad/ui/model/NetworkActions.h"
#include "dbid.h"
class AcDbDatabase;
class AcDbEntity;

namespace Autocad::GeneratedGeometry {
void Replace(AcDbDatabase &database, AcDbObjectId space, double millimetresPerUnit,
             const dsd::application::DuctNetwork &network,
             const dsd::application::NetworkGenerationResult &result,
             const NetworkActions::GeometryRequest &request);
void Remove(AcDbDatabase &database, const dsd::application::DuctNetwork &network);
bool IsGenerated(const AcDbEntity &entity);
void ForgetDocument(const std::string &documentPrefix);
void Clear();
} // namespace Autocad::GeneratedGeometry
