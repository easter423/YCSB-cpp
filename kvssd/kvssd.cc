#include "kvssd.h"

#include "kvssd_hashmap_db.h"
#include "kvssd_hashmap_db_impl.h"

namespace {
const std::string PROP_BACKEND = "hashmap";
}  // anonymous namespace

class KvssdDbWrapper : public ycsbc::DB {
   private:
    std::unique_ptr<kvssd::KVSSD> kvssd;

   public:
    KvssdDbWrapper(kvssd::KVSSD *k) : kvssd(k) {};
    ycsbc::DB::Status Read(const std::string &table, const std::string &key,
                           const std::vector<std::string> *fields,
                           std::vector<ycsbc::DB::Field> &result) {
        kvssd_hashmap::ReadRow(this->kvssd, key, result);
        return kOK;
    }
    ycsbc::DB::Status Scan(const std::string &table, const std::string &key, int len,
                           const std::vector<std::string> *fields,
                           std::vector<std::vector<ycsbc::DB::Field>> &result) {
        return kNotImplemented;
    }
    ycsbc::DB::Status Update(const std::string &table, const std::string &key,
                             std::vector<ycsbc::DB::Field> &values) {
        kvssd_hashmap::UpdateRow(this->kvssd, key, values);
        return kOK;
    }
    ycsbc::DB::Status Insert(const std::string &table, const std::string &key,
                             std::vector<ycsbc::DB::Field> &values) {
        kvssd_hashmap::InsertRow(this->kvssd, key, values);
        return kOK;
    }
    ycsbc::DB::Status Delete(const std::string &table, const std::string &key) {
        kvssd_hashmap::DeleteRow(this->kvssd, key);
        return kOK;
    }
};

ycsbc::DB *NewKvssdDB() {
    std::string backend = PROP_BACKEND;
    ycsbc::DB *ret = nullptr;
    if (backend == "hashmap") {
        kvssd::KVSSD *k = new kvssd_hashmap::Hashmap_KVSSD();
        ret = new KvssdDbWrapper(k);
    }
    if (backend == "kvssd") {
        // ret = new kvssd::KVSSD();
    }
    return ret;
}

const bool registered = ycsbc::DBFactory::RegisterDB("kvssd", NewKvssdDB);
