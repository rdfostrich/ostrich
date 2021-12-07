# OSTRICH
_Offset-enabled TRIple store for CHangesets_

[![Build Status](https://travis-ci.org/rdfostrich/ostrich.svg?branch=master)](https://travis-ci.org/rdfostrich/ostrich)
[![Docker Automated Build](https://img.shields.io/docker/automated/rdfostrich/ostrich.svg)](https://hub.docker.com/r/rdfostrich/ostrich/)
[![DOI](https://zenodo.org/badge/97819866.svg)](https://zenodo.org/badge/latestdoi/97819866)

**OSTRICH** is an _RDF triple store_ that allows _multiple versions_ of a dataset to be stored and queried at the same time.

The store is a hybrid between _snapshot_, _delta_ and _timestamp-based_ storage,
which provides a good trade-off between storage size and query time.
It provides several built-in algorithms to enable efficient iterator-based queries _at_ a certain version, _between_ any two versions, and _for_ versions. These queries support limits and offsets for any triple pattern.

Insertion is done by first inserting a dataset snapshot, which is encoded in [HDT](rdfhdt.org).
After that, deltas can be inserted, which contain additions and deletions based on the last delta or snapshot.

More details on OSTRICH can be found in our [**journal**](https://rdfostrich.github.io/article-jws2018-ostrich/) or [demo](https://rdfostrich.github.io/article-demo/) articles.

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
build/ostrich_test
```

### Query
```bash
build/ostrich-query-version-materialized patch_id s p o
build/ostrich-query-delta-materialized patch_id_start patch_id_end s p o
build/ostrich-query-version patch_id_start s p o
```

### Insert
```bash
build/ostrich-insert [-v] patch_id [+|- file_1.nt [file_2.nt [...]]]*
```

Input deltas must be sorted in SPO-order.

### Evaluate
Only load changesets from a path structured as `path_to_patch_directory/patch_id/main.nt.additions.txt` and `path_to_patch_directory/patch_id/main.nt.deletions.txt`.
```bash
build/ostrich-evaluate path_to_patch_directory patch_id_start patch_id_end
```
CSV-formatted insert data will be emitted: `version,added,durationms,rate,accsize`.

Load changesets AND query with triple patterns from the given file on separate lines, with the given number of replications.
```bash
build/ostrich-evaluate path_to_patch_directory patch_id_start patch_id_end patch_to_queries/queries.txt s|p|o nr_replications
```
CSV-formatted query data will be emitted (time in microseconds) for all versions for the three query types: `patch,offset,limit,count-ms,lookup-mus,results`.

## Docker

Alternatively, OSTRICH can be built and run using Docker.

### Build
```bash
docker build -t ostrich .
```

Instead of building the container yourself, you can use the pre-built image from [DockerHub](https://hub.docker.com/r/rdfostrich/ostrich/).
```bash
docker pull rdfostrich/ostrich
```

### Test
```bash
docker run --rm -it --entrypoint /opt/patchstore/build/ostrich_test ostrich
```

### Query
```bash
docker run --rm -it --entrypoint /opt/ostrich/build/ostrich-query-version-materialized ostrich patch_id s p o
docker run --rm -it --entrypoint /opt/ostrich/build/ostrich-delta-version-materialized ostrich patch_id_start patch_id_end s p o
docker run --rm -it --entrypoint /opt/ostrich/build/ostrich-query-version ostrich s p o
```

### Insert
```bash
docker run --rm -it --entrypoint /opt/ostrich/build/ostrich-insert ostrich [-v] patch_id [+|- file_1.nt [file_2.nt [...]]]*
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

`MIN_ADDITION_COUNT`: The minimum addition triple count so that it will be stored in the db. Changing this value only has effect during insertion time. Lookups are compatible with any value. (default `200`)

## Cite

If you are using or extending OSTRICH as part of a scientific publication,
we would appreciate a citation of our [article](https://rdfostrich.github.io/article-jws2018-ostrich/).

```bibtex
@article{taelman_jws_ostrich_2018,
  author = {Taelman, Ruben and Vander Sande, Miel and Van Herwegen, Joachim and Mannens, Erik and Verborgh, Ruben},
  title = {Triple Storage for Random-Access Versioned Querying of RDF Archives},
  journal = {Journal of Web Semantics},
  year = {2018},
  month = aug,
  url = {https://rdfostrich.github.io/article-jws2018-ostrich/}
}
```

## License
This software is written by [Ruben Taelman](http://rubensworks.net/) and colleagues.

This code is copyrighted by [Ghent University – imec](http://idlab.ugent.be/)
and released under the [MIT license](http://opensource.org/licenses/MIT).
