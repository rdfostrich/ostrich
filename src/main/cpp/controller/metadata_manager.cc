#include "metadata_manager.h"

MetadataManager::MetadataManager(const std::string& path) {
    database = new kyotocabinet::HashDB;
    bool status = database->open(path + "ingestion_metadata.kch", kyotocabinet::HashDB::OWRITER | kyotocabinet::HashDB::OCREATE);
    if (!status) {
        std::string err_msg = "cannot open metadata store ";
        err_msg.append(" error: ");
        err_msg.append(database->error().name());
        throw std::runtime_error(err_msg);
    }
}

MetadataManager::~MetadataManager() {
    bool status = database->close();
    if (status) {
        delete database;
    }
}

std::vector<uint64_t> MetadataManager::store_uint64(const std::string& prefix, int id, uint64_t value) {
    std::string key = prefix + std::to_string(id);
    std::vector<uint64_t> values = get_uint64(prefix, id);
    values.push_back(value);
    size_t buffer_size = values.size() * sizeof(uint64_t);
    char* v_buf = new char[buffer_size];
    char* v_buf_p = v_buf;
    for (uint64_t v: values) {
        std::memcpy(v_buf_p, &v, sizeof(uint64_t));
        v_buf_p += sizeof(uint64_t);
    }
    database->set(key.c_str(), key.size(), v_buf, buffer_size);
    delete[] v_buf;
    return values;
}

std::vector<double> MetadataManager::store_double(const std::string& prefix, int id, double value) {
    std::string key = prefix + std::to_string(id);
    std::vector<double> values = get_double(prefix, id);
    values.push_back(value);
    size_t buffer_size = values.size() * sizeof(double);
    char* v_buf = new char[buffer_size];
    char* v_buf_p = v_buf;
    for (double v: values) {
        std::memcpy(v_buf_p, &v, sizeof(double));
        v_buf_p += sizeof(double);
    }
    database->set(key.c_str(), key.size(), v_buf, buffer_size);
    delete[] v_buf;
    return values;
}

std::vector<uint64_t> MetadataManager::get_uint64(const std::string& prefix, int id) {
    std::string key = prefix + std::to_string(id);
    std::vector<uint64_t> values;
    size_t vs;
    char* buf = database->get(key.c_str(), key.size(), &vs);
    if (buf != nullptr) {
        for(int i=0; i<vs; i+=sizeof(uint64_t)) {
            uint64_t tmp;
            std::memcpy(&tmp, buf+i, sizeof(uint64_t));
            values.push_back(tmp);
        }
    }
    delete[] buf;
    return values;
}

std::vector<double> MetadataManager::get_double(const std::string& prefix, int id) {
    std::string key = prefix + std::to_string(id);
    std::vector<double> values;
    size_t vs;
    char* buf = database->get(key.c_str(), key.size(), &vs);
    if (buf != nullptr) {
        for(int i=0; i<vs; i+=sizeof(double)) {
            double tmp;
            std::memcpy(&tmp, buf+i, sizeof(double));
            values.push_back(tmp);
        }
    }
    delete[] buf;
    return values;
}

