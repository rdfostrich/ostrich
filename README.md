A patch-supporting triple store implementation.
Storage is done by using snapshots and incremental diffs.
This is implemented in such a way that lookups can be done efficiently without needing to store complete dataset snapshots every time data has been changed,
while still allowing for older versions to be queried.

Insertion is done by first inserting a dataset snapshot, which is encoded in HDT.
After that, patches can be inserted, which contain additions and deletions based on the last patch or snapshot.

Lookups for any patch id can be done by triple pattern and offsets.

