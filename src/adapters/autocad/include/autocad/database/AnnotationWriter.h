#pragma once
#include "application/duct_annotations.hpp"
#include "dbid.h"
class AcDbDatabase;
class AcDbBlockTableRecord;
namespace Autocad::AnnotationWriter {
// Called within the host's active transaction. The caller owns entity tracking.
std::vector<AcDbObjectId> Append(AcDbDatabase& database, AcDbBlockTableRecord& owner,
                                 double millimetresPerUnit,
                                 const dsd::application::AnnotationGenerationResult& result);
} // namespace Autocad::AnnotationWriter
