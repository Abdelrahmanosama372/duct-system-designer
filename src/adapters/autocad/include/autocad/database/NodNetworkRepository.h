#pragma once
#include "application/network_persistence.hpp"
class AcDbDatabase;
namespace Autocad::Persistence {
struct StoredDrawing {
    dsd::application::persistence::DrawingHeader header;
    std::vector<dsd::application::persistence::NetworkRecord> networks;
    std::vector<std::string> issues;
    bool writable = true;
};
class NodNetworkRepository {
  public:
    StoredDrawing Read(AcDbDatabase &database) const;
    // Caller supplies the active transaction. Unknown/damaged network records are preserved.
    void Write(AcDbDatabase &database, const dsd::application::persistence::DrawingHeader &header,
               const std::vector<dsd::application::persistence::NetworkRecord> &networks) const;
};
} // namespace Autocad::Persistence
