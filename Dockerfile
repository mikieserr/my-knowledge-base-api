FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        git \
        autoconf \
        autoconf-archive \
        automake \
        libtool \
        ca-certificates \
        curl \
        zip \
        unzip \
        tar \
        python3 \
        libssl-dev \
        libcurl4-openssl-dev \
        libpq-dev \
        libpqxx-dev \
        uuid-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh \
    && ln -s /opt/vcpkg/vcpkg /usr/local/bin/vcpkg

RUN vcpkg install minio-cpp nlohmann-json poppler libzip pugixml gtest

WORKDIR /src

COPY CMakeLists.txt ./
COPY cmake/ ./cmake/
COPY libs/ ./libs/
COPY apps/ ./apps/
COPY tests/ ./tests/

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && cmake --build build -j"$(nproc)" --target attachments-sandbox

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3 \
        libcurl4 \
        libpq5 \
        ca-certificates \
        tini \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -r -u 1000 -m -s /usr/sbin/nologin kbuser

COPY --from=builder /src/build/apps/attachments-sandbox/attachments-sandbox /usr/local/bin/

USER kbuser
WORKDIR /home/kbuser

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/usr/local/bin/attachments-sandbox"]