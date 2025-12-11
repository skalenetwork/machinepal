# ==========================================
# STAGE 1: Builder
# ==========================================
FROM ghcr.io/skalenetwork/machinepay-deps:latest AS builder

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive


WORKDIR /app

ENV VCPKG_ROOT=/app/vcpkg \
    VCPKG_DISABLE_METRICS=1


COPY src src
COPY CMakeLists.txt .
COPY main.cpp .
COPY vcpkg.json .
RUN rm -rf build cmake-build-release cmake-build-debug

RUN $VCPKG_ROOT/downloads/tools/cmake-*/cmake-*/bin/cmake -S . -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_TARGET_TRIPLET=x64-linux \
      -DCMAKE_MAKE_PROGRAM=$VCPKG_ROOT/downloads/tools/ninja-*/ninja

RUN  $VCPKG_ROOT/downloads/tools/cmake-*/cmake-*/bin/cmake --build build --target machinepay

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