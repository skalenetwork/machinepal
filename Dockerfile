# syntax=docker/dockerfile:1

# ==========================================
# STAGE 1: Builder
# ==========================================
FROM ghcr.io/skalenetwork/machinepay-deps:latest AS builder

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /app

# --- FIX: Install Build Tools (CMake & Ninja) ---
# If your deps image doesn't include these, you must install them here.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Set vcpkg root
ENV VCPKG_ROOT=/app/vcpkg \
    VCPKG_DISABLE_METRICS=1

# 1. Copy Manifests
COPY vcpkg.json .
COPY CMakeLists.txt .

# 2. Copy Source Code
COPY src src
COPY main.cpp .

# 3. Configure (CMake)
# Now 'cmake' will be found in /usr/bin/cmake
RUN cmake -S . -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_TARGET_TRIPLET=x64-linux

# 4. Build
RUN cmake --build build --target machinepay

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

RUN useradd -m -s /bin/bash machinepay

WORKDIR /machinepay
RUN chown machinepay:machinepay /machinepay

COPY --from=builder /app/build/machinepay /usr/local/bin/machinepay

# Copy Scripts
COPY --chmod=755 docker/run /etc/service/machinepay/run
COPY --chmod=755 docker/finish /etc/service/machinepay/finish
COPY --chmod=755 docker/first_run.sh /usr/local/bin/first_run.sh
COPY --chmod=755 docker/entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD [""]