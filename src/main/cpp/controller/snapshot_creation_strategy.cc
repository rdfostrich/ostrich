#include <string>
#include <numeric>
#include "snapshot_creation_strategy.h"


bool NeverCreateSnapshot::doCreate(const CreationStrategyMetadata& metadata) const {
    return false;
}

bool AlwaysCreateSnapshot::doCreate(const CreationStrategyMetadata &metadata) const {
    return true;
}

bool CreateSnapshotEveryN::doCreate(const CreationStrategyMetadata &metadata) const {
//    if (metadata.num_version % step == 0)
    if (metadata.patch_id % step == 0)
        return true;
    return false;
}

CreateSnapshotEveryN::CreateSnapshotEveryN(unsigned step): step(step) {}

CreateSnapshotEveryN::CreateSnapshotEveryN(): step(5) {}


SizeCreationStrategy::SizeCreationStrategy(): threshold(10.0) {}

SizeCreationStrategy::SizeCreationStrategy(double threshold): threshold(threshold) {}

bool SizeCreationStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    size_t cur_agg_delta_size = metadata.agg_delta_sizes[metadata.agg_delta_sizes.size()-1];
    double sum = std::accumulate(metadata.delta_sizes.begin(), metadata.delta_sizes.end(), 0.0);
    double size = metadata.delta_sizes.size();
    size_t mean_delta_size =  sum / size;
    return ((double) cur_agg_delta_size / (double) mean_delta_size) > threshold;
}


TimeCreationStrategy::TimeCreationStrategy(): ratio(3.0) {}

TimeCreationStrategy::TimeCreationStrategy(double ratio): ratio(ratio) {}

bool TimeCreationStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    if (metadata.ingestion_times.size() < 3) {
        return false;
    }
    uint64_t last = metadata.ingestion_times[metadata.ingestion_times.size()-1];
    uint64_t first = metadata.ingestion_times[1];  // time to ingest the first patch (snapshot ingestion is always much faster, so not really useful)
    return ((double) last / (double) first) > ratio;
}


SizeCreationStrategy2::SizeCreationStrategy2(): threshold(0.12) {}

SizeCreationStrategy2::SizeCreationStrategy2(double threshold): threshold(threshold) {}

bool SizeCreationStrategy2::doCreate(const CreationStrategyMetadata &metadata) const {
    size_t cur_agg_delta_size = metadata.agg_delta_sizes[metadata.agg_delta_sizes.size()-1];
    return ((double) cur_agg_delta_size / (double) metadata.last_snapshot_size) > threshold;
}


SnapshotCreationStrategy *SnapshotCreationStrategy::get_strategy(const std::string& strategy, const std::string& param) {
    if (strategy == "interval") {
        unsigned interval = std::stoul(param);
        return new CreateSnapshotEveryN(interval);
    }
    if (strategy == "size") {
        double threshold = std::stod(param);
        return new SizeCreationStrategy(threshold);
    }
    if (strategy == "time") {
        double ratio = std::stod(param);
        return new TimeCreationStrategy(ratio);
    }
    if (strategy == "size2") {
        double threshold = std::stod(param);
        return new SizeCreationStrategy2(threshold);
    }
    if (strategy == "change") {
        double threshold = std::stod(param);
        return new ChangeRatioCreationStrategy(threshold);
    }
    return nullptr;
}


ChangeRatioCreationStrategy::ChangeRatioCreationStrategy(): threshold(5.0) {}

ChangeRatioCreationStrategy::ChangeRatioCreationStrategy(double threshold): threshold(threshold) {}

bool ChangeRatioCreationStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    double current_agg_delta_size = metadata.agg_delta_sizes[metadata.agg_delta_sizes.size()-1];
    double size_union = metadata.last_snapshot_size + metadata.current_version_size;
    double cr = (current_agg_delta_size/size_union)*100;
    return cr > threshold;
}


