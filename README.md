A patch-supporting triple store implementation.
Storage is done by using snapshots and incremental diffs.
This is implemented in such a way that lookups can be done efficiently without needing to store complete dataset snapshots every time data has been changed,
while still allowing for older versions to be queried.

Insertion is done by first inserting a dataset snapshot, which is encoded in HDT.
After that, patches can be inserted, which contain additions and deletions based on the last patch or snapshot.

Lookups for any patch id can be done by triple pattern and offsets.

# Compiler variables
`PATCH_INSERT_BUFFER_SIZE`: The size of the triple parser buffer during patch insertion. (default `100`)
`FLUSH_POSITIONS_COUNT`: The amount of triples after which the patch positions should be flushed to disk, to avoid memory issues. (default `500000`)
`FLUSH_TRIPLES_COUNT`: The amount of triples after which the store should be flushed to disk, to avoid memory issues. (default `500000`)
`KC_MEMORY_MAP_SIZE`: The KC memory map size per tree. (default `1LL << 27` = 128MB)
`KC_PAGE_CACHE_SIZE`: The KC page cache size per tree. (default `1LL << 25` = 32MB)