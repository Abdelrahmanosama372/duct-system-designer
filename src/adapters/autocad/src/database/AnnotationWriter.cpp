#include "autocad/database/AnnotationWriter.h"
#include "autocad/ui/platform/ObjectArx.h"
#include "dbmtext.h"
#include "dbsymtb.h"
#include "dbtrans.h"
#include <cmath>
#include <memory>
#include <stdexcept>

namespace Autocad::AnnotationWriter {
namespace {
constexpr const wchar_t* appName = L"DSD_DUCT_ANNOTATIONS";
void check(Acad::ErrorStatus status, const char* message) {
    if (status != Acad::eOk)
        throw std::runtime_error(message);
}
template <class T> T* get(AcDbDatabase& database, AcDbObjectId id) {
    T* object = nullptr;
    check(database.transactionManager()->getObject(object, id),
          "Could not read annotation drawing settings");
    return object;
}
void register_app(AcDbDatabase& database) {
    AcDbRegAppTable* table = nullptr;
    check(
        database.transactionManager()->getObject(table, database.regAppTableId(), AcDb::kForWrite),
        "Could not access annotation registry");
    if (table->has(appName))
        return;
    auto record = std::make_unique<AcDbRegAppTableRecord>();
    check(record->setName(appName), "Could not name annotation registry");
    AcDbObjectId id;
    check(table->add(id, record.get()), "Could not create annotation registry");
    record.release()->close();
}
} // namespace
std::vector<AcDbObjectId> Append(AcDbDatabase& database, AcDbBlockTableRecord& owner, double scale,
                                 const dsd::application::AnnotationGenerationResult& result) {
    result.settings.validate();
    AcDbObjectId layer, style;
    check(get<AcDbLayerTable>(database, database.layerTableId())
              ->getAt(result.settings.layer.c_str(), layer),
          "Selected annotation layer no longer exists");
    const auto* layerRecord = get<AcDbLayerTableRecord>(database, layer);
    if (layerRecord->isLocked() || layerRecord->isFrozen() || layerRecord->isOff())
        throw std::invalid_argument("Unlock, thaw and turn on the annotation layer");
    check(get<AcDbTextStyleTable>(database, database.textStyleTableId())
              ->getAt(result.settings.text_style.c_str(), style),
          "Selected annotation text style no longer exists");
    register_app(database);
    std::vector<AcDbObjectId> created;
    created.reserve(result.annotations.size());
    for (const auto& annotation : result.annotations) {
        auto text = std::make_unique<AcDbMText>();
        text->setDatabaseDefaults(&database);
        check(text->setLayer(layer), "Could not set annotation layer");
        check(text->setColorIndex(256), "Could not set annotation color by layer");
        check(text->setTextStyle(style), "Could not set annotation text style");
        check(text->setNormal({0, 0, 1}), "Could not set annotation plane");
        check(text->setDirection({std::cos(annotation.rotation), std::sin(annotation.rotation), 0}),
              "Could not set annotation direction");
        using dsd::application::AnnotationAttachment;
        const auto attachment =
            annotation.attachment == AnnotationAttachment::BottomCenter ? AcDbMText::kBottomCenter
            : annotation.attachment == AnnotationAttachment::TopCenter  ? AcDbMText::kTopCenter
                                                                        : AcDbMText::kMiddleCenter;
        check(text->setAttachment(attachment), "Could not center annotation");
        check(text->setTextHeight(result.settings.text_height_mm / scale),
              "Could not set annotation height");
        check(text->setWidth(0), "Could not disable annotation wrapping");
        std::wstring contents = L"\\pxqc;";
        for (std::size_t i = 0; i < annotation.lines.size(); ++i) {
            if (i)
                contents += L"\\P";
            contents.append(annotation.lines[i].begin(), annotation.lines[i].end());
        }
        text->setContents(contents.c_str());
        check(text->setLocation({annotation.anchor.x / scale, annotation.anchor.y / scale, 0}),
              "Could not position annotation");
        AcDbObjectId id;
        check(owner.appendAcDbEntity(id, text.get()), "Could not append annotation");
        auto* added = text.release();
        const auto status = database.transactionManager()->addNewlyCreatedDBRObject(added, true);
        if (status != Acad::eOk) {
            added->close();
            check(status, "Could not track annotation");
        }
        created.push_back(id);
        const std::wstring network(result.network_id.begin(), result.network_id.end());
        const std::wstring item(annotation.item_id.begin(), annotation.item_id.end());
        auto* data = acutBuildList(1001, appName, 1000, network.c_str(), 1000, item.c_str(), 1071,
                                   static_cast<Adesk::Int32>(annotation.component_id), 0);
        if (!data)
            throw std::bad_alloc{};
        const auto tagStatus = added->setXData(data);
        acutRelRb(data);
        check(tagStatus, "Could not identify annotation");
    }
    return created;
}
} // namespace Autocad::AnnotationWriter
