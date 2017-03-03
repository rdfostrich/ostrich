# OSTRICH
_Offset-enabled TRIple store for CHangesets_

A triple store that allows multiple dataset versions to be stored and queried.
The store is delta-based, meaning that the overhead for storing multiple overlapping versions is reduced, when compared to full materialization.
Version materialized, version and delta materialized queries can be performed over the different versions, with a focus on the performance of the first two.
These queries support limits and offsets for any triple pattern.

Insertion is done by first inserting a dataset snapshot, which is encoded in HDT.
After that, patches can be inserted, which contain additions and deletions based on the last patch or snapshot.

## Building

OSTRICH requires ZLib, Kyoto Cabinet and CMake (compilation only) to be installed.

Compile:
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Running

The OSTRICH dataset will always be loaded from the current directory.

### Tests
```bash
build/tpfpatch_store_test
```

### Query
```bash
build/tpfpatch_store-query-version-materialized patch_id s p o
build/tpfpatch_store-query-delta-materialized patch_id_start patch_id_end s p o
build/tpfpatch_store-query-version patch_id_start s p o
```

### Insert
```bash
build/tpfpatch_store-insert [-v] patch_id [+|- file_1.nt [file_2.nt [...]]]*
```

### Evaluate
Only load changesets from a path structured as `path_to_patch_directory/patch_id/main.nt.additions.txt` and `path_to_patch_directory/patch_id/main.nt.deletions.txt`.
```bash
build/tpfpatch_store path_to_patch_directory patch_id_start patch_id_end
```
CSV-formatted insert data will be emitted: `version,added,durationms,rate,accsize`.

Load changesets AND query with triple patterns from the given file on separate lines, with the given number of replications.
```bash
build/tpfpatch_store path_to_patch_directory patch_id_start patch_id_end patch_to_queries/queries.txt s|p|o nr_replications
```
CSV-formatted query data will be emitted (time in microseconds) for all versions for the three query types: `patch,offset,lookup-mus-1,lookup-mus-50,lookup-mus-100,lookup-mus-inf`.

## Docker

Alternatively, OSTRICH can be built and run using Docker.

### Build
```bash
docker build -t ostrich .
```

### Test
```bash
docker run --rm -it --entrypoint /opt/patchstore/build/tpfpatch_store_test ostrich
```

### Query
```bash
docker run --rm -it --entrypoint /opt/patchstore/build/tpfpatch_store-query-version-materialized ostrich patch_id s p o
docker run --rm -it --entrypoint /opt/patchstore/build/tpfpatch_store-delta-version-materialized ostrich patch_id_start patch_id_end s p o
docker run --rm -it --entrypoint /opt/patchstore/build/tpfpatch_store-query-version ostrich s p o
```

### Insert
```bash
docker run --rm -it --entrypoint /opt/patchstore/build/tpfpatch_store-insert ostrich [-v] patch_id [+|- file_1.nt [file_2.nt [...]]]*
```

### Evaluate

Only load changesets from a path structured as `path_to_patch_directory/patch_id/main.nt.additions.txt` and `path_to_patch_directory/patch_id/main.nt.deletions.txt`.
```bash
docker run --rm -it -v path_to_patch_directory:/var/patches ostrich /var/patches patch_id_start patch_id_end
```

Load changesets AND query with triple patterns from the given file on separate lines, with the given number of replications.
```bash
docker run --rm -it -v path_to_patch_directory:/var/patches -v patch_to_queries:/var/queries ostrich /var/patches patch_id_start patch_id_end /var/queries/queries.txt s|p|o nr_replications
```

Enable debug mode:
```bash
docker run --rm -it -v path_to_patch_directory:/var/patches -v patch_to_queries:/var/queries -v path_to_crash_dir:/crash --privileged=true ostrich --debug /var/patches patch_id_start patch_id_end /var/queries/queries.txt s|p|o nr_replications
```

## Compiler variables
`PATCH_INSERT_BUFFER_SIZE`: The size of the triple parser buffer during patch insertion. (default `100`)

`FLUSH_POSITIONS_COUNT`: The amount of triples after which the patch positions should be flushed to disk, to avoid memory issues. (default `500000`)

`FLUSH_TRIPLES_COUNT`: The amount of triples after which the store should be flushed to disk, to avoid memory issues. (default `500000`)

`KC_MEMORY_MAP_SIZE`: The KC memory map size per tree. (default `1LL << 27` = 128MB)

`KC_PAGE_CACHE_SIZE`: The KC page cache size per tree. (default `1LL << 25` = 32MB)

`KC_PAGE_MAX_PURGE_ADDITION_COUNT_SIZE`: All triple addition counts below this value will be removed from the addition count db. (default `200`)