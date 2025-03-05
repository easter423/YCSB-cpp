#include "kvssd_hashmap_db_impl.h"
#include <cstdio>
#include <iostream>

namespace kvssd_hashmap {

// Field vector to string pointer
void SerializeRow(const std::vector<ycsbc::DB::Field> &values, std::string *data) {
    for (const ycsbc::DB::Field &field : values) {
        uint32_t len = field.name.size();
        data->append(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        data->append(field.name.data(), field.name.size());
        len = field.value.size();
        data->append(reinterpret_cast<char *>(&len), sizeof(uint32_t));
        data->append(field.value.data(), field.value.size());
    }
}

// char* to Field vector pointer
void DeserializeRow(std::vector<ycsbc::DB::Field> *values, const char *data_ptr, size_t data_len) {
    const char *p = data_ptr;
    const char *lim = p + data_len;
    std::cout << "hello!" << std::endl;
    while (p != lim) {
        assert(p < lim);
        std::cout << "I want to die " << lim - p << std::endl;
        uint32_t vlen = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string field(p, static_cast<const size_t>(vlen));
        p += vlen;
        uint32_t tlen = *reinterpret_cast<const uint32_t *>(p);
        p += sizeof(uint32_t);
        std::string value(p, static_cast<const size_t>(tlen));
        p += tlen;
        std::cout << p - data_ptr << "/" << lim - p << std::endl;
        values->push_back({field, value});
    }
    std::cout << "bye bye~" << std::endl;
}

std::unique_ptr<kvs_row, KvsRowDeleter> CreateRow(const std::string &key_in,
                                                  const std::vector<ycsbc::DB::Field> &value_in) {
    uint16_t key_length = key_in.size();
    void *key = malloc(key_length);
    std::memcpy(key, (void *)(key_in.data()), key_length);
    std::string value_sz;
    SerializeRow(value_in, &value_sz);
    void *value = malloc(value_sz.size());
    std::memcpy(value, value_sz.data(), value_sz.size());
    uint32_t value_length = value_sz.size();
    uint32_t actual_value_size = value_sz.size();
    uint32_t offset = 0;

    kvssd::kvs_key *newKey = new kvssd::kvs_key;
    newKey->key = key;
    newKey->length = key_length;
    kvssd::kvs_value *newValue = new kvssd::kvs_value;
    newValue->value = value;
    newValue->length = value_length;
    newValue->actual_value_size = actual_value_size;
    newValue->offset = offset;

    kvs_row *newRow = new kvs_row;
    newRow->key = newKey;
    newRow->value = newValue;

    return std::unique_ptr<kvs_row, KvsRowDeleter>(newRow);
}

// kvs_value의 값을 출력하는 함수
void PrintRow(const kvssd::kvs_value &value) {
    std::vector<ycsbc::DB::Field> value_vec;
    DeserializeRow(&value_vec, static_cast<char *>(value.value), value.length);
    if (value_vec.size() == 0) {
        printf("The value has empty field.\n");
        return;
    }
    printf("Name: Value\n");
    for (auto const &field : value_vec) {
        printf("%s: %s\n", field.name.data(), field.value.data());
    }
}

// Field vector의 값을 출력하는 함수
void PrintFieldVector(const std::vector<ycsbc::DB::Field> &value) {
    printf("\n");
    if (value.size() == 0) {
        printf("The value has empty field.\n");
        return;
    }
    printf("Name: Value\n");
    for (auto const &field : value) {
        printf("%s: %s\n", field.name.data(), field.value.data());
    }
    printf("\n");
}

void CheckAPI(const kvssd::kvs_result ret) {
    if (ret != kvssd::kvs_result::KVS_SUCCESS) {
        throw ycsbc::utils::Exception(std::string(kvssd::kvstrerror[static_cast<int>(ret)]));
    }
}

// Wrapper 함수 4가지
void ReadRow(const std::unique_ptr<kvssd::KVSSD> &kvssd, const std::string &key,
             std::vector<ycsbc::DB::Field> &value) {
    value = {};
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, value);
    CheckAPI(kvssd->Read(*newRow->key, *newRow->value));
    DeserializeRow(&value, static_cast<char *>(newRow->value->value), newRow->value->length);
}

void InsertRow(const std::unique_ptr<kvssd::KVSSD> &kvssd, const std::string &key,
               const std::vector<ycsbc::DB::Field> &value) {
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, value);
    CheckAPI(kvssd->Insert(*newRow->key, *newRow->value));
}

void UpdateRow(const std::unique_ptr<kvssd::KVSSD> &kvssd, const std::string &key,
               const std::vector<ycsbc::DB::Field> &value) {
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, value);
    CheckAPI(kvssd->Update(*newRow->key, *newRow->value));
}

void DeleteRow(const std::unique_ptr<kvssd::KVSSD> &kvssd, const std::string &key) {
    std::unique_ptr<kvs_row, KvsRowDeleter> newRow = CreateRow(key, {});
    CheckAPI(kvssd->Delete(*newRow->key));
}

}  // namespace kvssd_hashmap
