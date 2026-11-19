#pragma once
#include "autocad/ui/platform/ObjectArx.h"
#include "dbsymtb.h"
#include "dbtrans.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace Autocad::Persistence::detail {
inline void check(Acad::ErrorStatus status, const char *message) {
    if (status != Acad::eOk)
        throw std::runtime_error(message);
}
struct Close {
    void operator()(AcDbObject *object) const {
        if (!object)
            return;
        if (object->objectId().isNull())
            delete object;
        else
            object->close();
    }
};
template <class T> using Object = std::unique_ptr<T, Close>;
template <class T> Object<T> open(AcDbObjectId id, AcDb::OpenMode mode = AcDb::kForRead) {
    T *object = nullptr;
    check(acdbOpenObject(object, id, mode), "Could not open persistent drawing object");
    return Object<T>{object};
}
template <class T>
T *get(AcDbDatabase &database, AcDbObjectId id, AcDb::OpenMode mode = AcDb::kForRead) {
    AcDbObject *object = nullptr;
    check(database.transactionManager()->getObject(object, id, mode),
          "Could not access persistent drawing object");
    auto *typed = T::cast(object);
    if (!typed)
        throw std::runtime_error("Persistent drawing object has an unexpected type");
    return typed;
}
inline std::wstring wide(const std::string &text) { return {text.begin(), text.end()}; }
inline std::string narrow(const wchar_t *text) {
    std::string out;
    while (*text)
        out.push_back(static_cast<char>(*text++));
    return out;
}
inline std::string handle(const AcDbObject &object) {
    AcDbHandle value;
    object.getAcDbHandle(value);
    wchar_t buffer[32]{};
    value.getIntoAsciiBuffer(buffer);
    return narrow(buffer);
}
inline AcDbObjectId resolve(AcDbDatabase &database, const std::string &text) {
    if (text.empty() || text.size() > 16 ||
        text.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        return AcDbObjectId::kNull;
    AcDbObjectId id;
    const AcDbHandle value(wide(text).c_str());
    if (database.getAcDbObjectId(id, false, value) != Acad::eOk || id.isNull() || !id.isValid() ||
        id.isErased())
        return AcDbObjectId::kNull;
    return id;
}
class Transaction {
    AcDbTransactionManager &manager_;
    bool active_ = true;

  public:
    explicit Transaction(AcDbDatabase &db) : manager_(*db.transactionManager()) {
        if (!manager_.startTransaction())
            throw std::runtime_error("Could not begin persistence transaction");
    }
    ~Transaction() {
        if (active_)
            manager_.abortTransaction();
    }
    void commit() {
        check(manager_.endTransaction(), "Could not commit persistent drawing state");
        active_ = false;
    }
};
inline void adopted(AcDbDatabase &database, AcDbObject *object) {
    const auto status = database.transactionManager()->addNewlyCreatedDBRObject(object, true);
    if (status != Acad::eOk) {
        object->close();
        check(status, "Could not track persistent drawing object");
    }
}
} // namespace Autocad::Persistence::detail
