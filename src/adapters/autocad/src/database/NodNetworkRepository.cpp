#include "autocad/database/NodNetworkRepository.h"
#include "PersistenceDatabase.h"
#include "dbdict.h"
#include "dbxrecrd.h"
#include <algorithm>
#include <charconv>
#include <cwchar>
#include <limits>

namespace Autocad::Persistence {
using namespace detail;
using namespace dsd::application::persistence;
namespace {
struct Release {
    void operator()(resbuf *value) const {
        if (value)
            acutRelRb(value);
    }
};
std::string read_record(AcDbObjectId id) {
    auto record = open<AcDbXrecord>(id);
    resbuf *raw = nullptr;
    check(record->rbChain(&raw), "Could not read NOD record");
    std::unique_ptr<resbuf, Release> chain(raw);
    std::string bytes;
    int high = -1;
    for (auto *item = raw; item; item = item->rbnext) {
        if (item->restype != 1 || !item->resval.rstring || std::wcslen(item->resval.rstring) > 240)
            throw std::invalid_argument("Invalid NOD payload chunk");
        for (const auto *p = item->resval.rstring; *p; ++p) {
            const int n = *p >= L'0' && *p <= L'9'   ? *p - L'0'
                          : *p >= L'a' && *p <= L'f' ? *p - L'a' + 10
                                                     : -1;
            if (n < 0)
                throw std::invalid_argument("Invalid NOD payload encoding");
            if (high < 0)
                high = n;
            else {
                bytes.push_back(static_cast<char>((high << 4) | n));
                high = -1;
            }
        }
        if (bytes.size() > 64 * 1024 * 1024)
            throw std::invalid_argument("NOD record exceeds size limit");
    }
    if (high >= 0)
        throw std::invalid_argument("Truncated NOD payload");
    return bytes;
}
AcDbObjectId dictionary(AcDbDatabase &db, AcDbObjectId parent, const wchar_t *name) {
    auto *owner = get<AcDbDictionary>(db, parent, AcDb::kForWrite);
    AcDbObjectId id;
    if (owner->has(name)) {
        check(owner->getAt(name, id), "Could not find NOD dictionary");
        return id;
    }
    auto value = std::make_unique<AcDbDictionary>();
    check(owner->setAt(name, value.get(), id), "Could not create NOD dictionary");
    adopted(db, value.release());
    return id;
}
void write_record(AcDbDatabase &db, AcDbObjectId parent, const wchar_t *name,
                  const std::string &bytes) {
    auto *owner = get<AcDbDictionary>(db, parent, AcDb::kForWrite);
    AcDbObjectId id;
    AcDbXrecord *record = nullptr;
    if (owner->has(name)) {
        check(owner->getAt(name, id), "Could not find NOD record");
        record = get<AcDbXrecord>(db, id, AcDb::kForWrite);
    } else {
        auto value = std::make_unique<AcDbXrecord>();
        check(owner->setAt(name, value.get(), id), "Could not create NOD record");
        record = value.release();
        adopted(db, record);
    }
    std::unique_ptr<resbuf, Release> chain;
    resbuf *last = nullptr;
    std::wstring chunk;
    const auto append = [&]() {
        auto *item = acutNewRb(1);
        if (!item)
            throw std::bad_alloc{};
        item->resval.rstring = acutNewString(chunk.c_str());
        if (!item->resval.rstring) {
            acutRelRb(item);
            throw std::bad_alloc{};
        }
        if (last)
            last->rbnext = item;
        else
            chain.reset(item);
        last = item;
        chunk.clear();
    };
    constexpr wchar_t digits[] = L"0123456789abcdef";
    for (unsigned char c : bytes) {
        chunk += digits[c >> 4];
        chunk += digits[c & 15];
        if (chunk.size() == 240)
            append();
    }
    if (!chunk.empty())
        append();
    check(record->setFromRbChain(*chain), "Could not write NOD record");
}
} // namespace
StoredDrawing NodNetworkRepository::Read(AcDbDatabase &database) const {
    StoredDrawing out;
    try {
        auto nod = open<AcDbDictionary>(database.namedObjectsDictionaryId());
        AcDbObjectId rootId;
        if (!nod->has(L"DSD"))
            return out;
        check(nod->getAt(L"DSD", rootId), "Could not read DSD dictionary");
        auto root = open<AcDbDictionary>(rootId);
        AcDbObjectId id;
        if (!root->has(L"HEADER"))
            throw std::invalid_argument("DSD persistence header is missing");
        check(root->getAt(L"HEADER", id), "Could not read drawing persistence header");
        out.header = NetworkRecordCodec::decode_header(read_record(id));
        if (!root->has(L"NETWORKS"))
            return out;
        check(root->getAt(L"NETWORKS", id), "Could not read network dictionary");
        auto networks = open<AcDbDictionary>(id);
        std::unique_ptr<AcDbDictionaryIterator> iterator(networks->newIterator());
        for (; !iterator->done(); iterator->next()) {
            // Reserve IDs even when their payload cannot be read. A new registration must
            // never overwrite an unreadable record that we promised to preserve.
            const auto key = narrow(iterator->name());
            if (key.starts_with("NET-")) {
                int number = 0;
                const auto [end, error] =
                    std::from_chars(key.data() + 4, key.data() + key.size(), number);
                if (error == std::errc{} && end == key.data() + key.size() && number > 0 &&
                    number < (std::numeric_limits<int>::max)())
                    out.header.next_network_id = (std::max)(out.header.next_network_id, number + 1);
            }
            try {
                auto network =
                    NetworkRecordCodec::decode_network(read_record(iterator->objectId()));
                if (wide(network.registered.network.id) != iterator->name())
                    throw std::invalid_argument("Network dictionary key does not match record ID");
                out.networks.push_back(std::move(network));
            } catch (const std::exception &error) {
                out.issues.push_back(narrow(iterator->name()) + ": " + error.what());
            }
        }
    } catch (const std::exception &error) {
        out.writable = false;
        out.issues.push_back(error.what());
    }
    return out;
}
void NodNetworkRepository::Write(AcDbDatabase &db, const DrawingHeader &header,
                                 const std::vector<NetworkRecord> &networks) const {
    const auto root = dictionary(db, db.namedObjectsDictionaryId(), L"DSD");
    write_record(db, root, L"HEADER", NetworkRecordCodec::encode_header(header));
    const auto dict = dictionary(db, root, L"NETWORKS");
    for (const auto &network : networks)
        write_record(db, dict, wide(network.registered.network.id).c_str(),
                     NetworkRecordCodec::encode_network(network));
}
} // namespace Autocad::Persistence
