#ifndef OSTRICH_SNAPSHOT_CREATION_STRATEGY_H
#define OSTRICH_SNAPSHOT_CREATION_STRATEGY_H

#include <vector>


struct CreationStrategyMetadata {
    int patch_id = 0;
    std::vector<size_t> delta_sizes;
    std::vector<size_t> agg_delta_sizes;
    size_t last_snapshot_size = 0;
    size_t current_version_size = 0;
    std::vector<uint64_t> ingestion_times;
    std::vector<double> change_ratios;
};


class SnapshotCreationStrategy {
public:
    SnapshotCreationStrategy() = default;
    virtual bool doCreate(const CreationStrategyMetadata& metadata) const = 0;
    virtual ~SnapshotCreationStrategy() = default;

    static SnapshotCreationStrategy* get_strategy(const std::string& strategy, const std::string& param);
    static void split_parameters(const std::string& str, const std::string& delimiter, std::vector<std::string>& tokens);
    static SnapshotCreationStrategy* get_composite_strategy(const std::string& strategy_names, const std::string& strategy_params);
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


// Strategy creating new snapshots at periodic rate
class CreateSnapshotEveryN: public SnapshotCreationStrategy {
private:
    unsigned step;
public:
    CreateSnapshotEveryN();
    explicit CreateSnapshotEveryN(unsigned step);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


// Size strategy where the ratio is computed between the size of the aggregated delta and the size of the mean delta in the DC
class SizeCreationStrategy: public SnapshotCreationStrategy {
private:
    double threshold;
public:
    SizeCreationStrategy();
    explicit SizeCreationStrategy(double threshold);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


// Size strategy where the ratio is computed between the size of the snapshot and the size of the aggregated delta
class SizeCreationStrategy2: public SnapshotCreationStrategy {
private:
    double threshold;
public:
    SizeCreationStrategy2();
    explicit SizeCreationStrategy2(double threshold);
    bool doCreate(const CreationStrategyMetadata &metadata) const override;
};


// Time strategy where the threshold is base on the increase between the ingestion of the first delta and the current delta
class TimeCreationStrategy: public SnapshotCreationStrategy {
private:
    double ratio;
public:
    TimeCreationStrategy();
    explicit TimeCreationStrategy(double ratio);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


// Size strategy using the change ratio between the initial version of a DC and current version
class ChangeRatioCreationStrategy: public SnapshotCreationStrategy {
private:
    double threshold;
public:
    ChangeRatioCreationStrategy();
    explicit ChangeRatioCreationStrategy(double threshold);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


class CompositeSnapshotStrategy: public SnapshotCreationStrategy {
public:
    virtual void add_strategy(SnapshotCreationStrategy* strategy) = 0;
    bool doCreate(const CreationStrategyMetadata& metadata) const override = 0;
};

class OR_CompositeSnapshotStrategy: public CompositeSnapshotStrategy {
private:
    std::vector<SnapshotCreationStrategy*> strategies;

public:
    ~OR_CompositeSnapshotStrategy() override;
    void add_strategy(SnapshotCreationStrategy* strategy) override;
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


class AND_CompositeSnapshotStrategy: public CompositeSnapshotStrategy {
private:
    std::vector<SnapshotCreationStrategy*> strategies;

public:
    ~AND_CompositeSnapshotStrategy() override;
    void add_strategy(SnapshotCreationStrategy* strategy) override;
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};

#endif //OSTRICH_SNAPSHOT_CREATION_STRATEGY_H
