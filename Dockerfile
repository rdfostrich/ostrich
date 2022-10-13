FROM buildpack-deps:jammy

# Download sources
RUN cd /opt && curl -LO https://dbmx.net/kyotocabinet/pkg/kyotocabinet-1.2.79.tar.gz
RUN cd /opt && tar -xzf kyotocabinet-1.2.79.tar.gz && mv kyotocabinet-1.2.79 kyotocabinet && rm kyotocabinet-1.2.79.tar.gz
RUN apt-get update

# Download and install boost
RUN cd /opt && curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/boost_1_80_0.tar.gz
RUN cd /opt && tar -xzf boost_1_80_0.tar.gz && mv boost_1_80_0 boost && rm boost_1_80_0.tar.gz
RUN cd /opt/boost && ./bootstrap.sh --with-libraries=iostreams && ./b2 install

# Install kyoto cabinet
RUN apt-get -y install liblzo2-dev liblzma-dev zlib1g-dev build-essential
RUN cd /opt/kyotocabinet && ./configure --enable-lzo --enable-lzma && make && make install

# install raptor2
RUN apt-get install -y libraptor2-dev

# Install Serd
RUN apt-get install -y libserd-dev

# Install CMake
RUN apt-get install -y cmake

# Install GDB for debugging
RUN apt-get install -y gdb

# Copy sources
COPY deps /opt/ostrich/deps
COPY src /opt/ostrich/src
COPY CMakeLists.txt /opt/ostrich/CMakeLists.txt
COPY run.sh /opt/ostrich/run.sh
COPY run-debug.sh /opt/ostrich/run-debug.sh

# Build OSTRICH
RUN mkdir /opt/ostrich/build
RUN cd /opt/ostrich/build && cmake -DCMAKE_BUILD_TYPE=Release .. -Wno-deprecated
RUN cd /opt/ostrich/build && make

WORKDIR /var/evalrun

# Default command
ENTRYPOINT ["/opt/ostrich/run.sh"]
CMD ["ingest", "/var/patches", "1", "58", "/var/queries"]
