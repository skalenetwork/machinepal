# syntax=docker/dockerfile:1

# ==========================================
# STAGE 1: Builder
# ==========================================
# Assuming this image contains /app/vcpkg fully bootstrapped
FROM ghcr.io/skalenetwork/machinepay-deps:latest AS builder

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /app

# Set vcpkg root (must match where it was installed in the deps image)
ENV VCPKG_ROOT=/app/vcpkg \
    VCPKG_DISABLE_METRICS=1

# 1. Copy Manifests first (Optimization)
# We copy these before source code so we don't invalidate dependency checks
# if only main.cpp changes.
COPY vcpkg.json .
COPY CMakeLists.txt .

# 2. Copy Source Code
COPY src src
COPY main.cpp .

# 3. Configure (CMake)
# Using the pre-downloaded tools from the base image
RUN $VCPKG_ROOT/downloads/tools/cmake-*/cmake-*/bin/cmake -S . -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_TARGET_TRIPLET=x64-linux \
      -DCMAKE_MAKE_PROGRAM=$VCPKG_ROOT/downloads/tools/ninja-*/ninja

# 4. Build
RUN $VCPKG_ROOT/downloads/tools/cmake-*/cmake-*/bin/cmake --build build --target machinepay

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

WORKDIR /machinepay
# Ensure appuser owns it so the app can write data there
RUN chown appuser:appuser /machinepay

# 2. Copy Binary to SYSTEM path (Safe from volume overwrites)
COPY --from=builder /app/build/machinepay /usr/local/bin/machinepay

# 3. Copy Scripts
COPY --chmod=755 docker/run_machinepay.sh /etc/service/machinepay/run
COPY --chmod=755 docker/first_run.sh /usr/local/bin/first_run.sh
COPY --chmod=755 docker/entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/machinepay/entrypoint.sh"]
