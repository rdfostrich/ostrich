#ifndef OSTRICH_SNAPSHOT_CREATION_STRATEGY_H
#define OSTRICH_SNAPSHOT_CREATION_STRATEGY_H

#include <vector>

struct CreationStrategyMetadata {
    int num_version = 0;
    int patch_id = 0;
    size_t cur_delta_size = 0;
    size_t cur_agg_delta_size = 0;
    size_t median_delta_size = 0;
    size_t mean_delta_size = 0;
    size_t last_snapshot_size = 0;
    std::vector<uint64_t> ingestion_times;
};


class SnapshotCreationStrategy {
public:
    SnapshotCreationStrategy() = default;
    virtual bool doCreate(const CreationStrategyMetadata& metadata) const = 0;
    virtual ~SnapshotCreationStrategy() = default;

    static SnapshotCreationStrategy* get_strategy(const std::string& strategy, const std::string& param);
};


class NeverCreateSnapshot: public SnapshotCreationStrategy {
public:
    NeverCreateSnapshot() = default;
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


class AlwaysCreateSnapshot: public SnapshotCreationStrategy {
public:
    AlwaysCreateSnapshot() = default;
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


class CreateSnapshotEveryN: public SnapshotCreationStrategy {
private:
    unsigned step;
public:
    CreateSnapshotEveryN();
    explicit CreateSnapshotEveryN(unsigned step);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


class SizeCreationStrategy: public SnapshotCreationStrategy {
private:
    double threshold;
public:
    SizeCreationStrategy();
    explicit SizeCreationStrategy(double threshold);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


class TimeCreationStrategy: public SnapshotCreationStrategy {
private:
    double ratio;
public:
    TimeCreationStrategy();
    explicit TimeCreationStrategy(double ratio);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};

#endif //OSTRICH_SNAPSHOT_CREATION_STRATEGY_H
