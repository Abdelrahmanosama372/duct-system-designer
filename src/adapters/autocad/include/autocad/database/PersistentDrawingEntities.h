#pragma once
#include "application/network_persistence.hpp"
#include "dbid.h"
class AcDbDatabase;
namespace Autocad::Persistence {
class PersistentDrawingEntities {
  public:
    std::vector<dsd::application::persistence::EntityDefinition> Capture(
        AcDbDatabase &database, AcDbObjectId space, const std::string &network_id,
        const std::vector<dsd::application::persistence::EntityDefinition> &previous = {}) const;
    bool SourceExists(AcDbDatabase &database, AcDbObjectId space, const std::string &handle,
                      bool block) const;
    bool OutputExists(AcDbDatabase &database, AcDbObjectId space,
                      const dsd::application::persistence::EntityDefinition &definition) const;
    bool SymbolsMissing(AcDbDatabase &database,
                        const dsd::application::persistence::NetworkRecord &record) const;
    // Requires an active transaction. Restores only missing entities and repairs hatch links.
    void Recover(AcDbDatabase &database, AcDbObjectId space,
                 dsd::application::persistence::NetworkRecord &record) const;
};
} // namespace Autocad::Persistence
