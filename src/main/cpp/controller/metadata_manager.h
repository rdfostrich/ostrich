#ifndef OSTRICH_METADATA_MANAGER_H
#define OSTRICH_METADATA_MANAGER_H


#include <kchashdb.h>


class MetadataManager {
private:
    kyotocabinet::HashDB* database;

public:
    explicit MetadataManager(const std::string& path);
    ~MetadataManager();

    std::vector<uint64_t> store_uint64(const std::string& prefix, int id, uint64_t value);
    std::vector<double> store_double(const std::string& prefix, int id, double value);
    std::vector<uint64_t> get_uint64(const std::string& prefix, int id);
    std::vector<double> get_double(const std::string& prefix, int id);
};


#endif //OSTRICH_METADATA_MANAGER_H
