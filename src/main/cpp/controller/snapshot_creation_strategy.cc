#include "snapshot_creation_strategy.h"


bool NeverCreateSnapshot::doCreate(const CreationStrategyMetadata& metadata) const {
    return false;
}

bool AlwaysCreateSnapshot::doCreate(const CreationStrategyMetadata &metadata) const {
    return true;
}

bool CreateSnapshotEveryN::doCreate(const CreationStrategyMetadata &metadata) const {
    if (metadata.num_version % step == 0)
        return true;
    return false;
}

CreateSnapshotEveryN::CreateSnapshotEveryN(unsigned step): step(step) {}

CreateSnapshotEveryN::CreateSnapshotEveryN(): step(5) {}
