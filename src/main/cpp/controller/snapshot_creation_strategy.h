//
// Created by olivier on 30/06/2021.
//

#ifndef OSTRICH_SNAPSHOT_CREATION_STRATEGY_H
#define OSTRICH_SNAPSHOT_CREATION_STRATEGY_H


struct CreationStrategyMetadata {
    int num_version;
};


class SnapshotCreationStrategy {
public:
    SnapshotCreationStrategy() = default;
    virtual bool doCreate(const CreationStrategyMetadata& metadata) const = 0;
    virtual ~SnapshotCreationStrategy() = default;
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
    CreateSnapshotEveryN(unsigned step);
    bool doCreate(const CreationStrategyMetadata& metadata) const override;
};


#endif //OSTRICH_SNAPSHOT_CREATION_STRATEGY_H
