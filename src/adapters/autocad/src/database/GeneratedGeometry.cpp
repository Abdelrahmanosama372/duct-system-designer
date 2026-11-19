#include "autocad/database/GeneratedGeometry.h"
#include "PersistenceDatabase.h"
#include "autocad/database/PersistentDrawingEntities.h"
#include "autocad/database/AnnotationWriter.h"
#include "autocad/ui/platform/ObjectArx.h"
#include "dbcolor.h"
#include "dbents.h"
#include "dbpl.h"
#include "dbsymtb.h"
#include "dbtrans.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace Autocad::GeneratedGeometry {
namespace {
using namespace dsd::geometry;
constexpr const wchar_t *appName = L"DSD_GEOMETRY";
struct DisplayedNetwork {
    std::vector<AcDbObjectId> geometry, annotations;
};
std::map<std::string, DisplayedNetwork> displayed;
void check(Acad::ErrorStatus status, const char *message) {
    if (status != Acad::eOk)
        throw std::runtime_error(message);
}
class Transaction {
    AcDbTransactionManager &manager_;
    bool active_ = true;

public:
    explicit Transaction(AcDbDatabase &database) : manager_(*database.transactionManager()) {
        if (!manager_.startTransaction())
            throw std::runtime_error("Could not start geometry transaction");
    }
    ~Transaction() {
        if (active_)
            manager_.abortTransaction();
    }
    template <class T> T *get(AcDbObjectId id, AcDb::OpenMode mode = AcDb::kForRead) {
        AcDbObject *object = nullptr;
        check(manager_.getObject(object, id, mode), "Could not access generated geometry");
        auto *result = T::cast(object);
        if (!result)
            throw std::runtime_error("Unexpected drawing object type");
        return result;
    }
    template <class T>
    T *add(std::unique_ptr<T> object, AcDbObjectId &id, AcDbBlockTableRecord &owner) {
        check(owner.appendAcDbEntity(id, object.get()), "Could not append generated geometry");
        auto *result = object.release(); // Database now owns it, including on abort.
        const auto status = manager_.addNewlyCreatedDBRObject(result, true);
        if (status != Acad::eOk) {
            result->close();
            check(status, "Could not track generated geometry");
        }
        return result;
    }
    void commit() {
        check(manager_.endTransaction(), "Could not commit generated geometry");
        active_ = false;
    }
};
AcDbObjectId layer(AcDbDatabase &database, Transaction &transaction, const std::wstring &name,
                   short color) {
    auto *table = transaction.get<AcDbLayerTable>(database.layerTableId(), AcDb::kForWrite);
    AcDbObjectId id;
    if (table->has(name.c_str())) {
        check(table->getAt(name.c_str(), id), "Could not find geometry layer");
        const auto *record = transaction.get<AcDbLayerTableRecord>(id);
        if (record->isLocked() || record->isOff() || record->isFrozen())
            throw std::runtime_error("Unlock, thaw and turn on the selected geometry layers");
    } else {
        auto record = std::make_unique<AcDbLayerTableRecord>();
        check(record->setName(name.c_str()), "Invalid geometry layer name");
        AcCmColor layerColor;
        layerColor.setColorIndex(color);
        record->setColor(layerColor);
        check(table->add(id, record.get()), "Could not create geometry layer");
        record.release()->close(); // Creation is still covered by the transaction.
    }
    return id;
}
AcDbObjectId ensure_center_linetype(AcDbDatabase &database) {
    const auto find = [&]() {
        AcDbLinetypeTable *table = nullptr;
        check(database.getLinetypeTable(table, AcDb::kForRead), "Could not read drawing linetypes");
        AcDbObjectId id;
        const auto status = table->getAt(L"CENTER", id);
        table->close();
        if (status != Acad::eOk && status != Acad::eKeyNotFound)
            check(status, "Could not read CENTER linetype");
        return id;
    };
    auto id = find();
    if (!id.isNull())
        return id;
    // Use AutoCAD's support search path; allow either standard linetype library.
    if (database.loadLineTypeFile(L"CENTER", L"acadiso.lin") != Acad::eOk)
        check(database.loadLineTypeFile(L"CENTER", L"acad.lin"), "Could not load CENTER linetype");
    id = find();
    if (id.isNull())
        throw std::runtime_error("CENTER linetype was not loaded");
    return id;
}
void register_app(AcDbDatabase &database, Transaction &transaction) {
    auto *table = transaction.get<AcDbRegAppTable>(database.regAppTableId(), AcDb::kForWrite);
    if (table->has(appName))
        return;
    auto record = std::make_unique<AcDbRegAppTableRecord>();
    check(record->setName(appName), "Could not name geometry application");
    AcDbObjectId id;
    check(table->add(id, record.get()), "Could not register geometry application");
    record.release()->close();
}
void tag(AcDbEntity &entity, const std::string &network, int component) {
    const std::wstring id(network.begin(), network.end());
    resbuf *data = acutBuildList(1001, appName, 1000, id.c_str(), 1071,
                                 static_cast<Adesk::Int32>(component), 0);
    if (!data)
        throw std::bad_alloc{};
    const auto status = entity.setXData(data);
    acutRelRb(data);
    check(status, "Could not identify generated component");
}
struct Edge {
    Point start, end;
    double bulge;
};
std::unique_ptr<AcDbPolyline> boundary(const GeometricRepresentation &representation,
                                       double scale) {
    std::vector<Edge> edges;
    for (const auto &primitive : representation.primitives) {
        if (const auto *line = std::get_if<Line>(&primitive))
            edges.push_back({line->start, line->end, 0});
        else {
            const auto &arc = std::get<dsd::geometry::Arc>(primitive);
            edges.push_back({arc.start, arc.end(), std::tan(arc.angle / 4)});
        }
    }
    auto poly = std::make_unique<AcDbPolyline>(static_cast<unsigned>(edges.size()));
    Point first = edges.front().start, current = first;
    const auto points_close = [](const Point &a, const Point &b) {
        return std::hypot(a.x - b.x, a.y - b.y) < 1e-5;
    };
    unsigned vertex = 0;
    while (!edges.empty()) {
        const auto next = std::find_if(edges.begin(), edges.end(), [&](const auto &edge) {
            return points_close(current, edge.start) || points_close(current, edge.end);
        });
        if (next == edges.end())
            throw std::runtime_error("Component outline is disconnected");
        const bool reverse = !points_close(current, next->start);
        check(poly->addVertexAt(vertex++, {current.x / scale, current.y / scale},
                                reverse ? -next->bulge : next->bulge),
              "Could not build component outline");
        current = reverse ? next->start : next->end;
        edges.erase(next);
    }
    if (!points_close(current, first))
        throw std::runtime_error("Component outline is not closed");
    poly->setClosed(true);
    return poly;
}
void erase(Transaction &transaction, const std::vector<AcDbObjectId> &entities) {
    for (auto it = entities.rbegin(); it != entities.rend(); ++it)
        if (it->isValid() && !it->isErased())
            check(transaction.get<AcDbEntity>(*it, AcDb::kForWrite)->erase(),
                  "Could not remove previous generated geometry");
}
std::string key(const dsd::application::DuctNetwork &network) {
    return network.document_key + ":" + network.id;
}
} // namespace
void Replace(AcDbDatabase &database, AcDbObjectId space, double scale,
             const dsd::application::DuctNetwork &network,
             const dsd::application::NetworkGenerationResult &result,
             const NetworkActions::GeometryRequest &request) {
    auto &previous = displayed[key(network)];
    std::vector<AcDbObjectId> created;
    Transaction transaction{database};
    const auto ductLayer = layer(database, transaction, request.duct_layer, network.color_index);
    const bool withCenterlines = request.representation ==
        NetworkActions::GeometryRepresentation::OutlineWithCenterlines;
    AcDbObjectId centerlineLayer, centerLinetype;
    if (withCenterlines) {
        centerLinetype = ensure_center_linetype(database);
        centerlineLayer = layer(database, transaction, request.centerline_layer, network.color_index);
    }
    register_app(database, transaction);
    auto *owner = transaction.get<AcDbBlockTableRecord>(space, AcDb::kForWrite);
    for (const auto &[id, representation] : result.geometry) {
        const auto append = [&](auto entity, AcDbObjectId outputLayer, AcDbObjectId linetype) {
            entity->setDatabaseDefaults(&database);
            check(entity->setLayer(outputLayer), "Could not set generated entity layer");
            check(entity->setLinetype(linetype), "Could not set generated entity linetype");
            AcDbObjectId entityId;
            auto *added = transaction.add(std::move(entity), entityId, *owner);
            created.push_back(entityId);
            tag(*added, network.id, id);
        };
        if (representation.primitives.size() == 1) {
            const auto &line = std::get<Line>(representation.primitives.front());
            append(std::make_unique<AcDbLine>(
                AcGePoint3d{line.start.x / scale, line.start.y / scale, 0},
                AcGePoint3d{line.end.x / scale, line.end.y / scale, 0}),
                ductLayer, database.continuousLinetype());
        } else {
            append(boundary(representation, scale), ductLayer, database.continuousLinetype());
        }
        if (withCenterlines) {
            for (const auto &line : representation.layout_skeleton)
                append(std::make_unique<AcDbLine>(
                    AcGePoint3d{line.start.x / scale, line.start.y / scale, 0},
                    AcGePoint3d{line.end.x / scale, line.end.y / scale, 0}),
                    centerlineLayer, centerLinetype);
        }
    }
    std::vector<AcDbObjectId> annotations;
    if (result.annotations)
        annotations = AnnotationWriter::Append(database, *owner, scale, *result.annotations);
    erase(transaction, previous.annotations);
    erase(transaction, previous.geometry);
    transaction.commit();
    previous.geometry.swap(created);
    previous.annotations.swap(annotations);
}
void Remove(AcDbDatabase &database, const dsd::application::DuctNetwork &network) {
    const auto found = displayed.find(key(network));
    if (found == displayed.end())
        return;
    Transaction transaction{database};
    erase(transaction, found->second.annotations);
    erase(transaction, found->second.geometry);
    transaction.commit();
    displayed.erase(found);
}
void ReplaceAnnotations(AcDbDatabase& database, AcDbObjectId space, double scale,
    const dsd::application::DuctNetwork& network,
    const dsd::application::AnnotationGenerationResult& result) {
    auto& previous = displayed[key(network)];
    Transaction transaction{database};
    auto* owner = transaction.get<AcDbBlockTableRecord>(space, AcDb::kForWrite);
    auto created = AnnotationWriter::Append(database, *owner, scale, result);
    erase(transaction, previous.annotations);
    transaction.commit();
    previous.annotations.swap(created);
}
void RemoveAnnotations(AcDbDatabase& database, const dsd::application::DuctNetwork& network) {
    const auto found = displayed.find(key(network));
    if (found == displayed.end()) return;
    Transaction transaction{database};
    erase(transaction, found->second.annotations);
    transaction.commit();
    found->second.annotations.clear();
}
bool IsGenerated(const AcDbEntity &entity) {
    auto *data = entity.xData(appName);
    if (!data)
        return false;
    acutRelRb(data);
    return true;
}
void ForgetDocument(const std::string &prefix) {
    std::erase_if(displayed, [&](const auto &item) { return item.first.starts_with(prefix); });
}
void Clear() { displayed.clear(); }
} // namespace Autocad::GeneratedGeometry

void Autocad::GeneratedGeometry::AdoptPersistentEntities(
    AcDbDatabase &database, const dsd::application::persistence::NetworkRecord &record) {
    auto &output = displayed[key(record.registered.network)];
    output.geometry.clear();
    output.annotations.clear();
    using dsd::application::persistence::OutputGroup;
    const auto space = Autocad::Persistence::detail::resolve(database, record.space_handle);
    const Autocad::Persistence::PersistentDrawingEntities resolver;
    for (const auto &entity : record.entities) {
        if (space.isNull() || !resolver.OutputExists(database, space, entity))
            continue;
        const auto id = Autocad::Persistence::detail::resolve(database, entity.handle);
        if (id.isNull())
            continue;
        if (entity.group == OutputGroup::Geometry)
            output.geometry.push_back(id);
        if (entity.group == OutputGroup::Annotation)
            output.annotations.push_back(id);
    }
}
