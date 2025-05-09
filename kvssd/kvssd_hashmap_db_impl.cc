#include "kvssd_hashmap_db_impl.h"

#include <cstdio>
#include <iostream>

namespace kvssd_hashmap {

// Field vector to string pointer
void SerializeRow(const std::vector<ycsbc::DB::Field> &values, std::string *data) {
    for (const ycsbc::DB::Field &field : values) {
        auto len = static_cast<uint32_t>(field.name.size());
        const size_t old = data->size();
        data->resize(old + sizeof(len));
        std::memcpy(data->data() + old, &len, sizeof(len));
        data->append(field.name.data(), field.name.size());

        len = static_cast<uint32_t>(field.value.size());
        const size_t old2 = data->size();
        data->resize(old2 + sizeof(len));
        std::memcpy(data->data() + old2, &len, sizeof(len));
        data->append(field.value.data(), field.value.size());
    }
}

// char* to Field vector pointer
void DeserializeRow(std::vector<ycsbc::DB::Field> *values, const char *data_ptr, size_t data_len) {
    const char *p = data_ptr;
    const char *lim = p + data_len;
    while (p != lim) {
        assert(p < lim);
        uint32_t vlen;
        std::memcpy(&vlen, p, sizeof(vlen));
        p += sizeof(vlen);
        std::string field(p, static_cast<const size_t>(vlen));
        p += vlen;
        uint32_t tlen;
        std::memcpy(&tlen, p, sizeof(tlen));
        p += sizeof(tlen);
        std::string value(p, static_cast<const size_t>(tlen));
        p += tlen;
        values->push_back({field, value});
    }
}

std::unique_ptr<kvs_row, KvsRowDeleter> CreateRow(std::string_view key_in,
                                                  const std::vector<ycsbc::DB::Field> &value_in) {
    auto key_length = static_cast<uint16_t>(key_in.size());
    void *key = malloc(key_length);
    std::memcpy(key, (void *)(key_in.data()), key_length);

    void *value = nullptr;
    std::string value_sz;
    uint32_t value_length = 0;
    uint32_t actual_value_size = 0;
    uint32_t offset = 0;
    if (!value_in.empty()) {
        SerializeRow(value_in, &value_sz);
        value = malloc(value_sz.size());
        std::memcpy(value, value_sz.data(), value_sz.size());
        value_length = static_cast<uint32_t>(value_sz.size());
        actual_value_size = static_cast<uint32_t>(value_sz.size());
        offset = 0;
    }

    auto newKey = std::make_unique<kvssd::kvs_key>();
    newKey->key = key;
    newKey->length = key_length;
    auto newValue = std::make_unique<kvssd::kvs_value>();
    newValue->value = value;
    newValue->length = value_length;
    newValue->actual_value_size = actual_value_size;
    newValue->offset = offset;

    auto newRow = std::unique_ptr<kvs_row, KvsRowDeleter>(new kvs_row);
    newRow->key = newKey.release();
    newRow->value = newValue.release();

    return newRow;
}

#ifdef DEBUG

// Print kvs_value
void PrintRow(const kvssd::kvs_value &value) {
    std::vector<ycsbc::DB::Field> value_vec;
    DeserializeRow(&value_vec, static_cast<char *>(value.value), value.length);
    if (value_vec.empty()) {
        printf("The value has empty field.\n");
        return;
    }
    printf("Name: Value\n");
    for (auto const &field : value_vec) {
        printf("%s: %s\n", field.name.data(), field.value.data());
    }
}

// Print Field vector value
void PrintFieldVector(const std::vector<ycsbc::DB::Field> &value) {
    printf("\n");
    if (value.empty()) {
        printf("The value has empty field.\n");
        return;
    }
    printf("Name: Value\n");
    for (auto const &field : value) {
        printf("%s: %s\n", field.name.data(), field.value.data());
    }
    printf("\n");
}

#endif

void CheckAPI(const kvssd::kvs_result ret) {
    if (ret != kvssd::kvs_result::KVS_SUCCESS) {
        throw ycsbc::utils::Exception(std::string(kvssd::kvstrerror[static_cast<int>(ret)]));
    }
}

// Wrapper Functions
void ReadRow(kvssd::KVSSD &kvssd, const std::string &key, std::vector<ycsbc::DB::Field> &value) {
    value = {};
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, {});
    CheckAPI(kvssd.Read(*newRow->key, *newRow->value));
    DeserializeRow(&value, static_cast<char *>(newRow->value->value), newRow->value->length);
}

void InsertRow(kvssd::KVSSD &kvssd, const std::string &key,
               const std::vector<ycsbc::DB::Field> &value) {
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, value);
    CheckAPI(kvssd.Insert(*newRow->key, *newRow->value));
}

void UpdateRow(kvssd::KVSSD &kvssd, const std::string &key,
               const std::vector<ycsbc::DB::Field> &value) {
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, value);
    CheckAPI(kvssd.Update(*newRow->key, *newRow->value));
}

void DeleteRow(kvssd::KVSSD &kvssd, const std::string &key) {
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, {});
    CheckAPI(kvssd.Delete(*newRow->key));
}

}  // namespace kvssd_hashmap
