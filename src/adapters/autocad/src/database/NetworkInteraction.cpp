#include "autocad/ui/platform/ObjectArx.h"
#include "autocad/database/NetworkInteraction.h"
#include "acdocman.h"
#include "acstring.h"
#include "autocad/ui/model/ApplicationSession.h"
#include "autocad/ui/model/NetworkActions.h"
#include "autocad/ui/model/NetworkSession.h"
#include "dbapserv.h"
#include "dbcolor.h"
#include "dbdynblk.h"
#include "dbents.h"
#include "dbhatch.h"
#include "dbpl.h"
#include "dbsymtb.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
using namespace dsd::application;
using dsd::geometry::Point;
constexpr const wchar_t* visualizationLayer = L"Visualization Rectangles";
constexpr const wchar_t* visualizationApp = L"DSD_NETWORK_VIS";
struct Cancelled {};
struct Pending {
    NetworkActions::Action action;
    HWND window;
    AcApDocument* document;
};
std::optional<Pending> pending;
std::map<std::string, std::vector<AcDbObjectId>> displayed;
HWND notificationWindow = nullptr;

class DocumentReactor final : public AcApDocManagerReactor {
  public:
    void documentToBeDestroyed(AcApDocument* document) override {
        const auto prefix =
            std::to_string(reinterpret_cast<std::uintptr_t>(document->database())) + ":";
        std::erase_if(displayed,
                      [&](const auto& entry) { return entry.first.starts_with(prefix); });
        auto& session = NetworkSession::Current();
        if (pending && pending->document == document) {
            pending.reset();
            session.busy = false;
        }
        if (session.network && session.network->document_key.starts_with(prefix)) {
            NetworkSession::ResetDraft();
            session.message = L"The source drawing was closed. Select a root in an open drawing.";
        }
        if (::IsWindow(notificationWindow))
            ::PostMessage(notificationWindow, WM_DSD_NETWORK_UPDATED, 0, 0);
    }
};
DocumentReactor documentReactor;

bool is_visualization(const AcDbEntity& entity) {
    resbuf* data = entity.xData(visualizationApp);
    const bool tagged = data != nullptr;
    if (data)
        acutRelRb(data);
    return tagged;
}

void check(Acad::ErrorStatus status, const char* message) {
    if (status != Acad::eOk)
        throw std::runtime_error(message);
}
struct CloseObject {
    void operator()(AcDbObject* object) const {
        if (!object)
            return;
        if (object->objectId().isNull())
            delete object;
        else
            object->close();
    }
};
template <class T> using Object = std::unique_ptr<T, CloseObject>;
template <class T> Object<T> open(AcDbObjectId id, AcDb::OpenMode mode = AcDb::kForRead) {
    T* object = nullptr;
    check(acdbOpenObject(object, id, mode), "Could not open a drawing object");
    return Object<T>{object};
}
std::string ascii(const wchar_t* value) {
    std::string result;
    while (*value)
        result.push_back(static_cast<char>(*value++));
    return result;
}
std::wstring wide(const std::string& value) { return {value.begin(), value.end()}; }
std::string handle(const AcDbObject& object) {
    AcDbHandle value;
    check(object.getAcDbHandle(value), "Could not read source handle");
    wchar_t text[32] = {};
    value.getIntoAsciiBuffer(text);
    return ascii(text);
}
std::wstring layer_name(const AcDbEntity& entity) {
    auto record = open<AcDbLayerTableRecord>(entity.layerId());
    AcString name;
    check(record->getName(name), "Could not read source layer");
    return name.kACharPtr();
}
std::wstring original_name(const AcDbBlockReference& block) {
    AcDbDynBlockReference dynamic(block.objectId());
    auto definition = dynamic.dynamicBlockTableRecord();
    if (definition.isNull())
        definition = block.blockTableRecord();
    auto record = open<AcDbBlockTableRecord>(definition);
    AcString name;
    check(record->getName(name), "Could not read original block name");
    return name.kACharPtr();
}
double millimetres_per_unit(AcDbDatabase& database) {
    // INSUNITS values: unitless, inches, feet, millimetres, centimetres, metres.
    switch (static_cast<int>(database.insunits())) {
    case 0:
    case 4:
        return 1;
    case 1:
        return 25.4;
    case 2:
        return 304.8;
    case 5:
        return 10;
    case 6:
        return 1000;
    default:
        throw std::invalid_argument("Use millimetres, centimetres, metres, inches, "
                                    "feet, or unitless drawing units");
    }
}
Point point(const AcGePoint3d& value, double scale) { return {value.x * scale, value.y * scale}; }

std::optional<SourcePath> read_path(const AcDbEntity& entity, double scale) {
    SourcePath result;
    result.source_id = handle(entity);
    result.layer = layer_name(entity);
    if (const auto* line = AcDbLine::cast(&entity)) {
        result.vertices = {point(line->startPoint(), scale), point(line->endPoint(), scale)};
    } else if (const auto* poly = AcDbPolyline::cast(&entity)) {
        result.closed = poly->isClosed();
        for (unsigned i = 0; i < poly->numVerts(); ++i) {
            AcGePoint3d vertex;
            check(poly->getPointAt(i, vertex), "Could not read polyline vertex");
            result.vertices.push_back(point(vertex, scale));
            if (i + 1 < poly->numVerts() || result.closed) {
                double bulge = 0;
                check(poly->getBulgeAt(i, bulge), "Could not read polyline bulge");
                result.has_curves = result.has_curves || std::abs(bulge) > 1e-12;
            }
        }
    } else if (const auto* poly = AcDb2dPolyline::cast(&entity)) {
        result.closed = poly->isClosed();
        result.has_curves = poly->polyType() != AcDb::k2dSimplePoly;
        std::unique_ptr<AcDbObjectIterator> iterator{poly->vertexIterator()};
        std::vector<double> bulges;
        for (; !iterator->done(); iterator->step()) {
            auto vertex = open<AcDb2dVertex>(iterator->objectId());
            result.vertices.push_back(point(poly->vertexPosition(*vertex), scale));
            bulges.push_back(vertex->bulge());
        }
        for (std::size_t i = 0; i < bulges.size(); ++i)
            if (i + 1 < bulges.size() || result.closed)
                result.has_curves = result.has_curves || std::abs(bulges[i]) > 1e-12;
    } else if (const auto* poly = AcDb3dPolyline::cast(&entity)) {
        result.closed = poly->isClosed();
        result.has_curves = poly->polyType() != AcDb::k3dSimplePoly;
        std::unique_ptr<AcDbObjectIterator> iterator{poly->vertexIterator()};
        for (; !iterator->done(); iterator->step()) {
            auto vertex = open<AcDb3dPolylineVertex>(iterator->objectId());
            result.vertices.push_back(point(vertex->position(), scale));
        }
    } else
        return std::nullopt;
    return result;
}
std::string document_key(AcDbDatabase& database, AcDbObjectId space) {
    auto record = open<AcDbBlockTableRecord>(space);
    return std::to_string(reinterpret_cast<std::uintptr_t>(&database)) + ":" + handle(*record);
}
AcDbObjectId select(const wchar_t* prompt) {
    ads_name name;
    ads_point picked;
    if (acedEntSel(prompt, name, picked) != RTNORM)
        throw Cancelled{};
    AcDbObjectId id;
    check(acdbGetObjectId(id, name), "Could not identify selected entity");
    return id;
}
struct Snapshot {
    std::vector<SourcePath> paths;
    std::vector<dsd::geometry::Block> blocks;
};
Snapshot snapshot(AcDbObjectId space, double scale, const DuctNetwork& draft) {
    Snapshot result;
    auto record = open<AcDbBlockTableRecord>(space);
    AcDbBlockTableRecordIterator* rawIterator = nullptr;
    check(record->newIterator(rawIterator), "Could not enumerate drawing entities");
    std::unique_ptr<AcDbBlockTableRecordIterator> iterator{rawIterator};
    for (; !iterator->done(); iterator->step()) {
        AcDbObjectId id;
        check(iterator->getEntityId(id), "Could not read entity identifier");
        auto entity = open<AcDbEntity>(id);
        if (layer_name(*entity) == visualizationLayer || is_visualization(*entity))
            continue;
        if (const auto* block = AcDbBlockReference::cast(entity.get())) {
            if (!draft.air_terminal_block_name && !draft.damper_block_name)
                continue;
            const auto name = original_name(*block);
            if ((!draft.air_terminal_block_name || name != *draft.air_terminal_block_name) &&
                (!draft.damper_block_name || name != *draft.damper_block_name))
                continue;
            AcDbExtents bounds;
            check(block->getGeomExtents(bounds), "A matching block has no usable drawing bounds");
            result.blocks.push_back(
                {handle(*block),
                 name,
                 point(block->position(), scale),
                 {point(bounds.minPoint(), scale), point(bounds.maxPoint(), scale)}});
        } else if (const auto path = read_path(*entity, scale))
            result.paths.push_back(*path);
    }
    return result;
}
AcDbObjectId ensure_layer(AcDbDatabase& database) {
    AcDbLayerTable* raw = nullptr;
    check(database.getLayerTable(raw, AcDb::kForWrite), "Could not open drawing layers");
    Object<AcDbLayerTable> table{raw};
    AcDbObjectId id;
    if (table->has(visualizationLayer)) {
        check(table->getAt(visualizationLayer, id), "Could not read visualization layer");
        auto layer = open<AcDbLayerTableRecord>(id);
        if (layer->isLocked() || layer->isFrozen() || layer->isOff())
            throw std::runtime_error("Unlock, thaw, and turn on the Visualization "
                                     "Rectangles layer before refreshing");
    } else {
        Object<AcDbLayerTableRecord> layer{new AcDbLayerTableRecord};
        check(layer->setName(visualizationLayer), "Could not name visualization layer");
        check(table->add(id, layer.get()), "Could not create visualization layer");
    }
    return id;
}
void ensure_regapp(AcDbDatabase& database) {
    AcDbRegAppTable* raw = nullptr;
    check(database.getRegAppTable(raw, AcDb::kForWrite),
          "Could not open drawing application registry");
    Object<AcDbRegAppTable> table{raw};
    if (!table->has(visualizationApp)) {
        Object<AcDbRegAppTableRecord> record{new AcDbRegAppTableRecord};
        check(record->setName(visualizationApp), "Could not name visualization registry");
        AcDbObjectId id;
        check(table->add(id, record.get()), "Could not create visualization registry");
    }
}
void tag(AcDbEntity& entity, const DuctNetwork& network, const VisualizationRectangle& rectangle) {
    const auto networkId = wide(network.id), visualId = wide(rectangle.id),
               itemId = wide(rectangle.item_id);
    const wchar_t* type = rectangle.type == NetworkItemType::Segment       ? L"segment"
                          : rectangle.type == NetworkItemType::AirTerminal ? L"air_terminal"
                                                                           : L"damper";
    resbuf* data = acutBuildList(1001, visualizationApp, 1000, networkId.c_str(), 1000,
                                 visualId.c_str(), 1000, itemId.c_str(), 1000, type, 0);
    if (!data)
        throw std::bad_alloc{};
    const auto status = entity.setXData(data);
    acutRelRb(data);
    check(status, "Could not attach visualization identity");
}
void erase(const std::vector<AcDbObjectId>& ids) {
    for (const auto& id : ids)
        if (!id.isNull() && id.isValid() && !id.isErased()) {
            auto entity = open<AcDbEntity>(id, AcDb::kForWrite);
            check(entity->erase(), "Could not replace a previous visualization");
        }
}
void render(AcDbDatabase& database, AcDbObjectId space, double scale, const DuctNetwork& network,
            bool showSegments) {
    const auto layer = ensure_layer(database);
    ensure_regapp(database);
    auto owner = open<AcDbBlockTableRecord>(space, AcDb::kForWrite);
    auto& previous = displayed[network.document_key + ":" + network.id];
    std::vector<AcDbObjectId> created;
    created.reserve(2 * (network.segmented_lines_vis_rectangles.size() +
                         network.air_terminals_blocks_vis_rectangles.size() +
                         network.damper_blocks_vis_rectangles.size()));
    const auto append = [&](const VisualizationRectangle& rectangle) {
        Object<AcDbPolyline> boundary{new AcDbPolyline(4)};
        boundary->setDatabaseDefaults(&database);
        check(boundary->setLayer(layer), "Could not set visualization layer");
        check(boundary->setColorIndex(network.color_index), "Could not set visualization color");
        check(boundary->setElevation(0.0), "Could not set visualization elevation");
        AcCmTransparency transparency;
        transparency.setAlpha(128);
        check(boundary->setTransparency(transparency), "Could not set visualization transparency");
        for (unsigned i = 0; i < 4; ++i)
            check(boundary->addVertexAt(
                      i, {rectangle.corners[i].x / scale, rectangle.corners[i].y / scale}),
                  "Could not build visualization boundary");
        check(boundary->setClosed(Adesk::kTrue), "Could not close visualization boundary");
        AcDbObjectId boundaryId;
        check(owner->appendAcDbEntity(boundaryId, boundary.get()),
              "Could not add visualization boundary");
        created.push_back(boundaryId);
        tag(*boundary, network, rectangle);
        boundary.reset();
        Object<AcDbHatch> hatch{new AcDbHatch};
        hatch->setDatabaseDefaults(&database);
        check(hatch->setLayer(layer), "Could not set hatch layer");
        check(hatch->setColorIndex(network.color_index), "Could not set hatch color");
        check(hatch->setTransparency(transparency), "Could not set hatch transparency");
        check(hatch->setElevation(0.0), "Could not set hatch elevation");
        check(hatch->setPattern(AcDbHatch::kPreDefined, L"SOLID"),
              "Could not set solid hatch pattern");
        check(hatch->setAssociative(true), "Could not set hatch boundary association");
        AcDbObjectId hatchId;
        check(owner->appendAcDbEntity(hatchId, hatch.get()), "Could not add visualization hatch");
        created.push_back(hatchId);
        AcDbObjectIdArray loop;
        loop.append(boundaryId);
        check(hatch->appendLoop(AcDbHatch::kExternal, loop), "Could not set hatch boundary");
        check(hatch->evaluateHatch(true), "Could not evaluate visualization hatch");
        tag(*hatch, network, rectangle);
    };
    try {
        if (showSegments)
            for (const auto& rectangle : network.segmented_lines_vis_rectangles)
                append(rectangle);
        for (const auto& rectangle : network.air_terminals_blocks_vis_rectangles)
            append(rectangle);
        for (const auto& rectangle : network.damper_blocks_vis_rectangles)
            append(rectangle);
        erase(previous);
        previous.swap(created);
    } catch (...) {
        try {
            erase(created);
        } catch (...) {
        }
        throw;
    }
}
bool request(NetworkActions::Action action, HWND window) {
    auto& session = NetworkSession::Current();
    if (pending || session.busy) {
        session.message = L"Finish the current network selection first.";
        return false;
    }
    auto* document = acDocManager->mdiActiveDocument();
    if (!document) {
        session.message = L"Open a drawing before selecting network geometry.";
        return false;
    }
    pending = Pending{action, window, document};
    notificationWindow = window;
    session.busy = true;
    session.message = L"Select the requested source entity in the drawing.";
    const auto status =
        acDocManager->sendStringToExecute(document, L"_DSD_NETWORK_ACTION ", true, false, false);
    if (status != Acad::eOk) {
        pending.reset();
        session.busy = false;
        session.message = L"Could not start network selection.";
        return false;
    }
    return true;
}
} // namespace

void Autocad::Network::Initialize() {
    acDocManager->addReactor(&documentReactor);
    NetworkActions::SetHandler(request);
}
void Autocad::Network::Shutdown() {
    NetworkActions::SetHandler(nullptr);
    acDocManager->removeReactor(&documentReactor);
    pending.reset();
    displayed.clear();
    notificationWindow = nullptr;
    NetworkSession::Current().busy = false;
}
void Autocad::Network::ExecutePendingAction() {
    if (!pending)
        return;
    const auto operation = *pending;
    pending.reset();
    auto& session = NetworkSession::Current();
    try {
        auto* document = acDocManager->mdiActiveDocument();
        if (!document || document != operation.document)
            throw std::runtime_error("The active drawing changed; repeat the selection");
        auto* database = document->database();
        const auto space = database->currentSpaceId();
        const double scale = millimetres_per_unit(*database);
        const auto key = document_key(*database, space);
        if (session.network && session.network->document_key != key)
            throw std::runtime_error("Reset the draft before selecting geometry in "
                                     "another drawing or layout");
        DuctNetwork draft = session.network ? *session.network : DuctNetwork{};
        if (draft.id.empty()) {
            draft.id = network_item_id("NET", NetworkSession::NextId());
            draft.color_index =
                NetworkColorPool[(static_cast<std::size_t>(NetworkSession::NextId()) - 1) %
                                 NetworkColorPool.size()];
        }
        draft.document_key = key;
        if (operation.action == NetworkActions::Action::PickRoot) {
            auto selected = open<AcDbEntity>(select(L"\nSelect root LINE or straight POLYLINE: "));
            if (selected->ownerId() != space || layer_name(*selected) == visualizationLayer ||
                is_visualization(*selected))
                throw std::invalid_argument(
                    "Select an original source line in the current drawing space");
            const auto root = read_path(*selected, scale);
            if (!root)
                throw std::invalid_argument("Root must be a LINE or straight POLYLINE");
            if (!draft.root || draft.root->source_id != root->source_id) {
                draft.air_terminal_block_name.reset();
                draft.damper_block_name.reset();
            }
            draft.root = root;
        } else {
            if (!draft.root)
                throw std::invalid_argument("Select a root line first");
            if (operation.action == NetworkActions::Action::PickAirTerminal ||
                operation.action == NetworkActions::Action::PickDamper) {
                auto selected =
                    open<AcDbEntity>(select(L"\nSelect an equipment block reference: "));
                const auto* block = AcDbBlockReference::cast(selected.get());
                if (!block || selected->ownerId() != space)
                    throw std::invalid_argument(
                        "Select a block reference in the current drawing space");
                const auto name = original_name(*block);
                if (operation.action == NetworkActions::Action::PickAirTerminal)
                    draft.air_terminal_block_name = name;
                else
                    draft.damper_block_name = name;
            }
        }
        const auto drawing = snapshot(space, scale, draft);
        const auto root =
            std::find_if(drawing.paths.begin(), drawing.paths.end(),
                         [&](const auto& path) { return path.source_id == draft.root->source_id; });
        if (root == drawing.paths.end())
            throw std::runtime_error("The source root no longer exists; select a root again");
        draft.root = *root;
        draft.lines = drawing.paths;
        auto prepared = std::make_unique<DuctNetwork>(
            DuctUi::Application().prepare_network(std::move(draft), drawing.blocks));
        render(*database, space, scale, *prepared, session.form.segments);
        if (!session.network)
            session.form.id = NetworkSession::NextId()++;
        session.network = std::move(prepared);
        NetworkSession::UpdateForm(session.form);
        session.form.segmentCount.Format(L"%zu", session.network->segmented_lines_infos.size());
        session.form.totalFlow = L"0 CFM";
        session.form.status = L"Draft";
        session.message.Format(L"Draft %hs: %zu ducts, %zu air terminals, %zu "
                               L"dampers. Registration awaits the analyzer.",
                               session.network->id.c_str(),
                               session.network->segmented_lines_infos.size(),
                               session.network->air_terminal_blocks_infos.size(),
                               session.network->damper_blocks_infos.size());
    } catch (const Cancelled&) {
        session.message = L"Selection cancelled. Previous draft retained.";
    } catch (const std::exception& error) {
        session.message = CString(error.what());
    } catch (CException* error) {
        error->Delete();
        session.message = L"Could not update network visualization. Please try again.";
    }
    session.busy = false;
    if (::IsWindow(operation.window))
        ::PostMessage(operation.window, WM_DSD_NETWORK_UPDATED, 0, 0);
}

std::optional<Autocad::Network::VisualizationIdentity>
Autocad::Network::ResolveVisualization(AcDbObjectId id) {
    auto entity = open<AcDbEntity>(id);
    resbuf* data = entity->xData(visualizationApp);
    if (!data)
        return std::nullopt;
    std::vector<std::wstring> fields;
    for (auto* item = data; item; item = item->rbnext)
        if (item->restype == 1000 && item->resval.rstring)
            fields.emplace_back(item->resval.rstring);
    acutRelRb(data);
    if (fields.size() != 4)
        return std::nullopt;
    return VisualizationIdentity{fields[0], fields[1], fields[2], fields[3]};
}
void Autocad::Network::IdentifyVisualization() {
    try {
        const auto identity =
            ResolveVisualization(select(L"\nSelect a visualization boundary or hatch: "));
        if (!identity) {
            acutPrintf(L"\nThis entity is not a Duct System Designer visualization.");
            return;
        }
        acutPrintf(L"\nNetwork: %ls | Visualization: %ls | Item: %ls | Type: %ls",
                   identity->network_id.c_str(), identity->visualization_id.c_str(),
                   identity->item_id.c_str(), identity->type.c_str());
    } catch (const Cancelled&) {
    } catch (const std::exception& error) {
        acutPrintf(L"\n%hs", error.what());
    }
}
