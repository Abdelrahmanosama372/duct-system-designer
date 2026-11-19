#include "autocad/database/PersistentDrawingEntities.h"
#include "PersistenceDatabase.h"
#include "acstring.h"
#include "dbcolor.h"
#include "dbents.h"
#include "dbhatch.h"
#include "dbmtext.h"
#include "dbpl.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace Autocad::Persistence {
using namespace detail;
namespace geometry = dsd::geometry;
using namespace dsd::application::persistence;
namespace {
constexpr const wchar_t *applications[] = {L"DSD_GEOMETRY", L"DSD_NETWORK_VIS",
                                           L"DSD_DUCT_ANNOTATIONS"};
struct FreeRb {
    void operator()(resbuf *p) const {
        if (p)
            acutRelRb(p);
    }
};
using Rb = std::unique_ptr<resbuf, FreeRb>;
bool identity(const AcDbEntity &entity, EntityDefinition &out, const std::string &network) {
    for (int i = 0; i < 3; ++i) {
        Rb data(entity.xData(applications[i]));
        if (!data)
            continue;
        out.tag_app = applications[i];
        out.tag_strings.clear();
        out.component_id.reset();
        for (auto *p = data.get(); p; p = p->rbnext) {
            if (p->restype == 1000 && p->resval.rstring)
                out.tag_strings.emplace_back(p->resval.rstring);
            if (p->restype == 1071)
                out.component_id = static_cast<int>(p->resval.rlong);
        }
        if (out.tag_strings.empty() || out.tag_strings.front() != wide(network))
            return false;
        out.group = i == 0   ? OutputGroup::Geometry
                    : i == 1 ? OutputGroup::Visualization
                             : OutputGroup::Annotation;
        return true;
    }
    return false;
}
std::wstring name(const AcDbSymbolTableRecord &record) {
    AcString text;
    check(record.getName(text), "Could not read symbol name");
    return text.kACharPtr();
}
geometry::Point point(const AcGePoint3d &p) { return {p.x, p.y}; }
AcGePoint3d point(const geometry::Point &p) { return {p.x, p.y, 0}; }
std::uint32_t rgb(const AcCmColor &c) {
    return (static_cast<std::uint32_t>(c.red()) << 16) |
           (static_cast<std::uint32_t>(c.green()) << 8) | c.blue();
}
AcCmColor color(short index, bool true_color, std::uint32_t value) {
    AcCmColor c;
    if (true_color)
        c.setRGB(static_cast<Adesk::UInt8>(value >> 16), static_cast<Adesk::UInt8>(value >> 8),
                 static_cast<Adesk::UInt8>(value));
    else
        c.setColorIndex(index);
    return c;
}
void pattern(AcDbObjectId id, std::wstring &text, double &length, std::vector<double> &dashes) {
    auto record = open<AcDbLinetypeTableRecord>(id);
    text = name(*record);
    length = record->patternLength();
    for (int i = 0; i < record->numDashes(); ++i)
        dashes.push_back(record->dashLengthAt(i));
}
TextStyleDefinition text_style(AcDbObjectId id) {
    TextStyleDefinition out;
    auto record = open<AcDbTextStyleTableRecord>(id);
    out.name = name(*record);
    ads_name entity;
    check(acdbGetAdsName(entity, id), "Could not access text style fields");
    Rb data(acdbEntGet(entity));
    if (!data)
        throw std::runtime_error("Could not read text style fields");
    for (auto *p = data.get(); p; p = p->rbnext) {
        if (p->restype == 3 && p->resval.rstring)
            out.font_file = p->resval.rstring;
        if (p->restype == 4 && p->resval.rstring)
            out.big_font_file = p->resval.rstring;
        if (p->restype == 40)
            out.height = p->resval.rreal;
        if (p->restype == 41)
            out.width_factor = p->resval.rreal;
        if (p->restype == 50)
            out.oblique_angle = p->resval.rreal;
        if (p->restype == 70)
            out.vertical = (p->resval.rint & 4) != 0;
        if (p->restype == 71)
            out.flags = p->resval.rint;
    }
    return out;
}
AcDbObjectId ensure_linetype(AcDbDatabase &db, const std::wstring &text, double length,
                             const std::vector<double> &dashes) {
    auto *table = get<AcDbLinetypeTable>(db, db.linetypeTableId(), AcDb::kForWrite);
    AcDbObjectId id;
    if (table->has(text.c_str())) {
        check(table->getAt(text.c_str(), id), "Could not read linetype");
        return id;
    }
    auto record = std::make_unique<AcDbLinetypeTableRecord>();
    check(record->setName(text.c_str()), "Invalid saved linetype name");
    check(record->setPatternLength(length), "Invalid saved linetype length");
    check(record->setNumDashes(static_cast<int>(dashes.size())), "Invalid saved dash count");
    for (std::size_t i = 0; i < dashes.size(); ++i)
        check(record->setDashLengthAt(static_cast<int>(i), dashes[i]),
              "Invalid saved dash pattern");
    check(table->add(id, record.get()), "Could not restore linetype");
    adopted(db, record.release());
    return id;
}
AcDbObjectId ensure_layer(AcDbDatabase &db, const LayerDefinition &d) {
    auto *table = get<AcDbLayerTable>(db, db.layerTableId(), AcDb::kForWrite);
    AcDbObjectId id;
    if (table->has(d.name.c_str())) {
        check(table->getAt(d.name.c_str(), id), "Could not read restored layer");
        return id;
    }
    auto record = std::make_unique<AcDbLayerTableRecord>();
    check(record->setName(d.name.c_str()), "Invalid saved layer name");
    record->setColor(color(d.color, d.true_color, d.rgb));
    AcCmTransparency layerTransparency;
    layerTransparency.setAlpha(static_cast<Adesk::UInt8>(d.transparency));
    record->setTransparency(layerTransparency);
    record->setLineWeight(static_cast<AcDb::LineWeight>(d.lineweight));
    record->setIsOff(d.off);
    record->setIsFrozen(d.frozen); // Keep new layers writable until their objects are restored.
    record->setLinetypeObjectId(ensure_linetype(db, d.linetype, d.pattern_length, d.dashes));
    check(table->add(id, record.get()), "Could not restore layer");
    adopted(db, record.release());
    return id;
}
AcDbObjectId ensure_style(AcDbDatabase &db, const TextStyleDefinition &d) {
    auto *table = get<AcDbTextStyleTable>(db, db.textStyleTableId(), AcDb::kForWrite);
    AcDbObjectId id;
    if (table->has(d.name.c_str())) {
        check(table->getAt(d.name.c_str(), id), "Could not read restored text style");
        return id;
    }
    auto record = std::make_unique<AcDbTextStyleTableRecord>();
    check(record->setName(d.name.c_str()), "Invalid saved style name");
    check(record->setFileName(d.font_file.c_str()), "Could not restore style font");
    check(record->setBigFontFileName(d.big_font_file.c_str()), "Could not restore big font");
    check(record->setTextSize(d.height), "Could not restore style height");
    check(record->setXScale(d.width_factor), "Could not restore style width");
    check(record->setObliquingAngle(d.oblique_angle), "Could not restore style oblique angle");
    record->setIsVertical(d.vertical);
    record->setFlagBits(static_cast<Adesk::UInt8>(d.flags));
    check(table->add(id, record.get()), "Could not restore text style");
    adopted(db, record.release());
    return id;
}
void tag(AcDbDatabase &db, AcDbEntity &entity, const EntityDefinition &d) {
    auto *table = get<AcDbRegAppTable>(db, db.regAppTableId(), AcDb::kForWrite);
    if (!table->has(d.tag_app.c_str())) {
        auto record = std::make_unique<AcDbRegAppTableRecord>();
        check(record->setName(d.tag_app.c_str()), "Invalid saved tag application");
        AcDbObjectId id;
        check(table->add(id, record.get()), "Could not restore tag application");
        adopted(db, record.release());
    }
    Rb data;
    resbuf *tail = nullptr;
    const auto string = [&](int code, const std::wstring &value) {
        auto *p = acutNewRb(code);
        if (!p)
            throw std::bad_alloc{};
        p->resval.rstring = acutNewString(value.c_str());
        if (!p->resval.rstring) {
            acutRelRb(p);
            throw std::bad_alloc{};
        }
        if (tail)
            tail->rbnext = p;
        else
            data.reset(p);
        tail = p;
    };
    string(1001, d.tag_app);
    for (const auto &text : d.tag_strings)
        string(1000, text);
    if (d.component_id) {
        auto *p = acutNewRb(1071);
        if (!p)
            throw std::bad_alloc{};
        p->resval.rlong = *d.component_id;
        tail->rbnext = p;
    }
    check(entity.setXData(data.get()), "Could not restore entity identity");
}
bool kind_matches(const AcDbEntity &entity, EntityKind kind) {
    switch (kind) {
    case EntityKind::Line:
        return AcDbLine::cast(&entity) != nullptr;
    case EntityKind::Polyline:
        return AcDbPolyline::cast(&entity) != nullptr;
    case EntityKind::Hatch:
        return AcDbHatch::cast(&entity) != nullptr;
    case EntityKind::Text:
        return AcDbMText::cast(&entity) != nullptr;
    }
    return false;
}
} // namespace
std::vector<EntityDefinition>
PersistentDrawingEntities::Capture(AcDbDatabase &db, AcDbObjectId space, const std::string &network,
                                   const std::vector<EntityDefinition> &previous) const {
    std::vector<EntityDefinition> out;
    auto owner = open<AcDbBlockTableRecord>(space);
    AcDbBlockTableRecordIterator *raw = nullptr;
    check(owner->newIterator(raw), "Could not inspect network output");
    std::unique_ptr<AcDbBlockTableRecordIterator> iterator(raw);
    for (; !iterator->done(); iterator->step()) {
        AcDbObjectId id;
        check(iterator->getEntityId(id), "Could not read output ID");
        auto entity = open<AcDbEntity>(id);
        EntityDefinition d;
        if (!identity(*entity, d, network))
            continue;
        d.handle = handle(*entity);
        auto layer = open<AcDbLayerTableRecord>(entity->layerId());
        d.layer.name = name(*layer);
        const auto lc = layer->color();
        d.layer.color = lc.colorIndex();
        d.layer.true_color = lc.isByColor();
        d.layer.rgb = rgb(lc);
        d.layer.off = layer->isOff();
        d.layer.frozen = layer->isFrozen();
        d.layer.locked = layer->isLocked();
        d.layer.lineweight = static_cast<int>(layer->lineWeight());
        d.layer.transparency =
            layer->transparency().isByAlpha() ? layer->transparency().alpha() : 255;
        pattern(layer->linetypeObjectId(), d.layer.linetype, d.layer.pattern_length,
                d.layer.dashes);
        const auto ec = entity->color();
        d.color = ec.colorIndex();
        d.true_color = ec.isByColor();
        d.rgb = rgb(ec);
        d.transparency = entity->transparency().isByAlpha() ? entity->transparency().alpha() : 255;
        d.transparency_by_layer =
            entity->transparency().isByLayer() || entity->transparency().isInvalid();
        d.transparency_by_block = entity->transparency().isByBlock();
        d.visible = entity->visibility() == AcDb::kVisible;
        d.linetype_scale = entity->linetypeScale();
        pattern(entity->linetypeId(), d.linetype, d.pattern_length, d.dashes);
        if (const auto *line = AcDbLine::cast(entity.get())) {
            d.kind = EntityKind::Line;
            d.points = {point(line->startPoint()), point(line->endPoint())};
        } else if (const auto *poly = AcDbPolyline::cast(entity.get())) {
            d.kind = EntityKind::Polyline;
            d.closed = poly->isClosed();
            for (unsigned i = 0; i < poly->numVerts(); ++i) {
                AcGePoint3d p;
                double bulge = 0;
                check(poly->getPointAt(i, p), "Could not save outline vertex");
                check(poly->getBulgeAt(i, bulge), "Could not save outline arc");
                d.points.push_back(point(p));
                d.bulges.push_back(bulge);
            }
        } else if (const auto *text = AcDbMText::cast(entity.get())) {
            d.kind = EntityKind::Text;
            d.points = {point(text->location())};
            d.text_height = text->textHeight();
            d.text_width = text->width();
            const auto direction = text->direction();
            d.rotation = std::atan2(direction.y, direction.x);
            d.attachment = static_cast<int>(text->attachment());
            auto *contents = text->contents();
            if (contents) {
                d.text = contents;
                acutDelString(contents);
            }
            d.style = text_style(text->textStyle());
        } else if (const auto *hatch = AcDbHatch::cast(entity.get())) {
            d.kind = EntityKind::Hatch;
            AcDbObjectIdArray boundaries;
            check(hatch->getAssocObjIds(boundaries), "Could not save hatch association");
            if (boundaries.length() != 1) {
                const auto saved =
                    std::find_if(previous.begin(), previous.end(), [&](const auto &old) {
                        return old.handle == d.handle && old.kind == EntityKind::Hatch;
                    });
                if (saved == previous.end())
                    throw std::runtime_error("Network hatch has no saved outline boundary; refresh "
                                             "the visualization before registering");
                auto definition = *saved;
                definition.visible = d.visible;
                out.push_back(std::move(definition));
                continue;
            }
            wchar_t boundaryHandle[32]{};
            boundaries[0].handle().getIntoAsciiBuffer(boundaryHandle);
            d.boundary_handle = narrow(boundaryHandle);
            d.hatch_pattern = hatch->patternName();
            d.hatch_scale = hatch->patternScale();
            d.hatch_angle = hatch->patternAngle();
        } else
            throw std::runtime_error("Unsupported network output entity");
        out.push_back(std::move(d));
    }
    return out;
}
bool PersistentDrawingEntities::SourceExists(AcDbDatabase &db, AcDbObjectId space,
                                             const std::string &text, bool block) const {
    const auto id = resolve(db, text);
    if (id.isNull())
        return false;
    auto object = open<AcDbObject>(id);
    const auto *entity = AcDbEntity::cast(object.get());
    if (!entity || entity->ownerId() != space)
        return false;
    if (block)
        return AcDbBlockReference::cast(entity) != nullptr;
    return AcDbLine::cast(entity) || AcDbPolyline::cast(entity) || AcDb2dPolyline::cast(entity) ||
           AcDb3dPolyline::cast(entity);
}
bool PersistentDrawingEntities::OutputExists(AcDbDatabase &db, AcDbObjectId space,
                                             const EntityDefinition &d) const {
    const auto id = resolve(db, d.handle);
    if (id.isNull())
        return false;
    auto object = open<AcDbObject>(id);
    const auto *entity = AcDbEntity::cast(object.get());
    EntityDefinition actual;
    if (!entity || !kind_matches(*entity, d.kind) || entity->ownerId() != space ||
        d.tag_strings.empty())
        return false;
    return identity(*entity, actual, narrow(d.tag_strings.front().c_str())) &&
           actual.tag_app == d.tag_app && actual.tag_strings == d.tag_strings &&
           actual.component_id == d.component_id;
}
bool PersistentDrawingEntities::SymbolsMissing(AcDbDatabase &db,
                                               const NetworkRecord &record) const {
    auto layers = open<AcDbLayerTable>(db.layerTableId());
    auto styles = open<AcDbTextStyleTable>(db.textStyleTableId());
    auto linetypes = open<AcDbLinetypeTable>(db.linetypeTableId());
    for (const auto &d : record.entities) {
        if (!layers->has(d.layer.name.c_str()) || !linetypes->has(d.layer.linetype.c_str()) ||
            !linetypes->has(d.linetype.c_str()))
            return true;
        if (d.kind == EntityKind::Text && !styles->has(d.style.name.c_str()))
            return true;
    }
    return false;
}
void PersistentDrawingEntities::Recover(AcDbDatabase &db, AcDbObjectId space,
                                        NetworkRecord &record) const {
    NetworkRecoveryService planner;
    const auto plan = planner.plan(
        record, [&](const auto &h, bool b) { return SourceExists(db, space, h, b); },
        [&](const auto &d) { return OutputExists(db, space, d); });
    if (!plan.consistent())
        throw std::runtime_error("Missing sources prevent network output recovery");
    std::set<std::size_t> missing(plan.missing_entities.begin(), plan.missing_entities.end());
    auto *owner = get<AcDbBlockTableRecord>(db, space, AcDb::kForWrite);
    std::map<std::string, std::string> replaced;
    std::map<std::wstring, bool> locks;
    for (const auto &d : record.entities) {
        const bool existed = get<AcDbLayerTable>(db, db.layerTableId())->has(d.layer.name.c_str());
        const auto layer = ensure_layer(db, d.layer);
        auto *value = get<AcDbLayerTableRecord>(db, layer, AcDb::kForWrite);
        if (!locks.contains(d.layer.name))
            locks.emplace(d.layer.name, existed ? value->isLocked() : d.layer.locked);
        value->setIsLocked(false);
        (void)ensure_linetype(db, d.linetype, d.pattern_length, d.dashes);
        if (d.kind == EntityKind::Text)
            (void)ensure_style(db, d.style);
    }
    const auto append = [&](EntityDefinition &d, std::unique_ptr<AcDbEntity> entity) {
        const auto layer = ensure_layer(db, d.layer);
        entity->setDatabaseDefaults(&db);
        check(entity->setLayer(layer), "Could not restore entity layer");
        check(entity->setColor(color(d.color, d.true_color, d.rgb)),
              "Could not restore entity color");
        check(entity->setLinetype(ensure_linetype(db, d.linetype, d.pattern_length, d.dashes)),
              "Could not restore entity linetype");
        check(entity->setLinetypeScale(d.linetype_scale),
              "Could not restore entity linetype scale");
        AcCmTransparency transparency;
        if (d.transparency_by_block)
            transparency.setMethod(AcCmTransparency::kByBlock);
        else if (d.transparency_by_layer)
            transparency.setMethod(AcCmTransparency::kByLayer);
        else
            transparency.setAlpha(static_cast<Adesk::UInt8>(d.transparency));
        check(entity->setTransparency(transparency), "Could not restore transparency");
        check(entity->setVisibility(d.visible ? AcDb::kVisible : AcDb::kInvisible),
              "Could not restore output visibility");
        AcDbObjectId id;
        check(owner->appendAcDbEntity(id, entity.get()), "Could not restore network output");
        auto *added = entity.release();
        adopted(db, added);
        tag(db, *added, d);
        replaced[d.handle] = handle(*added);
        d.handle = handle(*added);
        return id;
    };
    for (auto index : plan.missing_entities) {
        auto &d = record.entities[index];
        if (d.kind == EntityKind::Hatch)
            continue;
        if (d.kind == EntityKind::Line) {
            if (d.points.size() != 2)
                throw std::runtime_error("Invalid saved line");
            append(d, std::make_unique<AcDbLine>(point(d.points[0]), point(d.points[1])));
        } else if (d.kind == EntityKind::Polyline) {
            if (d.points.size() != d.bulges.size())
                throw std::runtime_error("Invalid saved polyline");
            auto poly = std::make_unique<AcDbPolyline>(static_cast<unsigned>(d.points.size()));
            for (std::size_t i = 0; i < d.points.size(); ++i)
                check(poly->addVertexAt(static_cast<unsigned>(i), {d.points[i].x, d.points[i].y},
                                        d.bulges[i]),
                      "Could not restore vertex");
            poly->setClosed(d.closed);
            append(d, std::move(poly));
        } else if (d.kind == EntityKind::Text) {
            if (d.points.size() != 1)
                throw std::runtime_error("Invalid saved text");
            auto text = std::make_unique<AcDbMText>();
            const auto id = append(d, std::move(text));
            auto *restored = get<AcDbMText>(db, id, AcDb::kForWrite);
            check(restored->setTextStyle(ensure_style(db, d.style)),
                  "Could not restore text style");
            check(restored->setTextHeight(d.text_height), "Could not restore text height");
            check(restored->setWidth(d.text_width), "Could not restore text width");
            check(restored->setNormal({0, 0, 1}), "Could not restore text plane");
            check(restored->setDirection({std::cos(d.rotation), std::sin(d.rotation), 0}),
                  "Could not restore text direction");
            check(restored->setAttachment(static_cast<AcDbMText::AttachmentPoint>(d.attachment)),
                  "Could not restore text alignment");
            restored->setContents(d.text.c_str());
            check(restored->setLocation(point(d.points[0])), "Could not restore text position");
        }
    }
    for (std::size_t index = 0; index < record.entities.size(); ++index) {
        auto &d = record.entities[index];
        if (d.kind != EntityKind::Hatch)
            continue;
        const auto new_boundary = replaced.find(d.boundary_handle);
        const bool repair = new_boundary != replaced.end();
        if (repair)
            d.boundary_handle = new_boundary->second;
        if (!missing.contains(index) && !repair)
            continue;
        const auto boundary = resolve(db, d.boundary_handle);
        if (boundary.isNull())
            throw std::runtime_error("Saved hatch boundary is missing");
        AcDbObjectId id = missing.contains(index) ? append(d, std::make_unique<AcDbHatch>())
                                                  : resolve(db, d.handle);
        auto *hatch = get<AcDbHatch>(db, id, AcDb::kForWrite);
        while (hatch->numLoops())
            check(hatch->removeLoopAt(0), "Could not repair hatch loop");
        check(hatch->setPatternScale(d.hatch_scale), "Could not restore hatch scale");
        check(hatch->setPatternAngle(d.hatch_angle), "Could not restore hatch angle");
        check(hatch->setPattern(AcDbHatch::kPreDefined, d.hatch_pattern.c_str()),
              "Could not restore hatch pattern");
        check(hatch->setAssociative(true), "Could not restore hatch associativity");
        AcDbObjectIdArray loop;
        loop.append(boundary);
        check(hatch->appendLoop(AcDbHatch::kExternal, loop), "Could not restore hatch boundary");
        check(hatch->evaluateHatch(true), "Could not restore hatch fill");
    }
    for (const auto &[name, locked] : locks) {
        AcDbObjectId id;
        check(get<AcDbLayerTable>(db, db.layerTableId())->getAt(name.c_str(), id),
              "Could not finish restored layer");
        get<AcDbLayerTableRecord>(db, id, AcDb::kForWrite)->setIsLocked(locked);
    }
}
} // namespace Autocad::Persistence
