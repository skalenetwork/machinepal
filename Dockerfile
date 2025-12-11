# ==========================================
# STAGE 1: Builder
# ==========================================
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install Build Dependencies
# Added python3 which is often required for Boost build scripts
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        bison \
        flex \
        ca-certificates \
        curl \
        git \
        pkg-config \
        unzip \
        zip \
        tar \
        build-essential \
        python3 \
        linux-libc-dev \
        autoconf \
        libtool \
        automake \
        autoconf-archive \
        libtoolize

    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

ENV VCPKG_ROOT=/app/vcpkg \
    VCPKG_DISABLE_METRICS=1


RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_ROOT && \
    $VCPKG_ROOT/bootstrap-vcpkg.sh


COPY vcpkg.json .

RUN $VCPKG_ROOT/vcpkg install --triplet x64-linux

# 4. Copy Source Code
COPY . .


RUN $VCPKG_ROOT/downloads/tools/cmake-*/cmake-*/bin/cmake -S . -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_TARGET_TRIPLET=x64-linux \
      -DCMAKE_MAKE_PROGRAM=$VCPKG_ROOT/downloads/tools/ninja-*/ninja \
    && $VCPKG_ROOT/downloads/tools/cmake-*/cmake-*/bin/cmake --build build --target machinepay

# ==========================================
# STAGE 2: Runtime
# ==========================================
FROM ubuntu:22.04 AS runtime

# Install Runtime Dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        libstdc++6 \
        runit \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user
RUN useradd -m -s /bin/bash appuser

WORKDIR /machinepay

# Copy the binary from the builder stage
COPY --from=builder /app/build/machinepay /machinepay/machinepay

# Copy service scripts
COPY --chmod=755 docker/run_machinepay.sh /etc/service/machinepay/run
COPY --chmod=755 docker/first_run.sh /machinepay/first_run.sh

# Entrypoint
ENTRYPOINT ["/usr/sbin/runsvdir", "/etc/service"]