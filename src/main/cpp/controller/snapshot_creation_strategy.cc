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


void SnapshotCreationStrategy::split_parameters(const std::string& str, const std::string& delimiter, std::vector<std::string>& tokens) {
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delimiter, prev);
        if (pos == std::string::npos) {
            pos = str.length();
        }
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) {
            tokens.push_back(token);
        }
        prev = pos + delimiter.length();
    } while (pos < str.length() && prev < str.length());
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
    if (strategy == "aggchange") {
        double threshold = std::stod(param);
        return new AggregatedChangeRatioStrategy(threshold);
    }
    if (strategy == "locaggchange") {
        double threshold = std::stod(param);
        return new LocalAggregatedChangeRatioStrategy(threshold);
    }
    if (strategy == "never") {
        return new NeverCreateSnapshot;
    }
    return nullptr;
}

SnapshotCreationStrategy *SnapshotCreationStrategy::get_composite_strategy(const std::string &strategy_names,
                                                                           const std::string &strategy_params) {
    std::vector<std::string> op;
    split_parameters(strategy_names, "_", op);
    if (op.size() > 1) {
        CompositeSnapshotStrategy* strat;
        if (op[0] == "and") {
            strat = new AND_CompositeSnapshotStrategy;
        } else {
            strat = new OR_CompositeSnapshotStrategy;
        }
        std::vector<std::string> s_names, parameters;
        split_parameters(op[1], "-", s_names);
        split_parameters(strategy_params, "-", parameters);
        if (!s_names.empty() && !parameters.empty()) {
            if (s_names.size() == parameters.size()) {
                for (int i = 0; i < s_names.size(); i++) {
                    strat->add_strategy(get_strategy(s_names[i], parameters[i]));
                }
                return strat;
            }
        }
        delete strat;
        return nullptr;
    } else {
        return get_strategy(strategy_names, strategy_params);
    }
}


ChangeRatioCreationStrategy::ChangeRatioCreationStrategy(): threshold(0.05) {}

ChangeRatioCreationStrategy::ChangeRatioCreationStrategy(double threshold): threshold(threshold) {}

bool ChangeRatioCreationStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    double cr = metadata.change_ratios[metadata.change_ratios.size()-1];
    return cr > threshold;
}


OR_CompositeSnapshotStrategy::~OR_CompositeSnapshotStrategy() {
    for (auto s: strategies)
        delete s;
}

void OR_CompositeSnapshotStrategy::add_strategy(SnapshotCreationStrategy *strategy) {
    strategies.push_back(strategy);
}

bool OR_CompositeSnapshotStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    bool create = false;
    for (auto s: strategies) {
        create = create || s->doCreate(metadata);
    }
    return create;
}


AND_CompositeSnapshotStrategy::~AND_CompositeSnapshotStrategy() {
    for (auto s: strategies)
        delete s;
}

void AND_CompositeSnapshotStrategy::add_strategy(SnapshotCreationStrategy *strategy) {
    strategies.push_back(strategy);
}

bool AND_CompositeSnapshotStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    bool create = true;
    for (auto s: strategies) {
        create = create && s->doCreate(metadata);
    }
    return create;
}


AggregatedChangeRatioStrategy::AggregatedChangeRatioStrategy(): threshold(1.5) {}

AggregatedChangeRatioStrategy::AggregatedChangeRatioStrategy(double threshold): threshold(threshold) {}

bool AggregatedChangeRatioStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    double sum = std::accumulate(metadata.change_ratios.begin(), metadata.change_ratios.end(), 0.0);
    return sum >= threshold;
}


LocalAggregatedChangeRatioStrategy::LocalAggregatedChangeRatioStrategy(): threshold(0.5) {}

LocalAggregatedChangeRatioStrategy::LocalAggregatedChangeRatioStrategy(double threshold): threshold(threshold) {}

bool LocalAggregatedChangeRatioStrategy::doCreate(const CreationStrategyMetadata &metadata) const {
    double sum = std::accumulate(metadata.loc_change_ratios.begin(), metadata.loc_change_ratios.end(), 0.0);
    return sum >= threshold;
}
