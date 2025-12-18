# syntax=docker/dockerfile:1

# ==========================================
# STAGE 1: Builder
# ==========================================
FROM ghcr.io/skalenetwork/machinepal-deps:latest AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Change this via:
#   docker build --build-arg CACHEBUST=$(date +%s) -f Dockerfile .
ARG CACHEBUST=1

WORKDIR /app

# Make CACHEBUST part of the command so changing it reliably breaks this layer cache
RUN echo "Cache bust: ${CACHEBUST}" \
 && cd machinepal \
 && git pull

RUN echo "Cache bust: ${CACHEBUST}" && cd machinepal && git submodule update --init --recursive

ENV VCPKG_ROOT=/app/machinepal/external/vcpkg \
     VCPKG_DISABLE_METRICS=1


# 3. Configure (CMake)`
# Now 'cmake' will be found in /usr/bin/cmake
RUN cd /app/machinepal && cmake -S . -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_TARGET_TRIPLET=x64-linux

# 4. Build
RUN cd /app/machinepal && cmake --build build


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
        coreutils \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -s /bin/bash machinepal

WORKDIR /machinepal
RUN chown machinepal:machinepal /machinepal

# Ensure runit service directory exists with predictable perms
RUN mkdir -p /etc/service/machinepal \
    && chown -R root:root /etc/service \
    && chmod 0755 /etc/service /etc/service/machinepal

# Create writable runtime state dir(s) for non-root execution
RUN mkdir -p /var/lib/machinepal /var/log/machinepal \
    && chown -R machinepal:machinepal /var/lib/machinepal /var/log/machinepal \
    && chmod 0755 /var/lib/machinepal /var/log/machinepal

COPY --from=builder /app/machinepal/build/machinepal /usr/local/bin/machinepal
COPY --from=builder --chmod=755 /app/machinepal/docker/run /etc/service/machinepal/run
COPY --from=builder --chmod=755 /app/machinepal/docker/finish /etc/service/machinepal/finish
COPY --from=builder --chmod=755 /app/machinepal/docker/first_run.sh /usr/local/bin/first_run.sh
COPY --from=builder --chmod=755 /app/machinepal/docker/entrypoint.sh /entrypoint.sh

# Ensure entrypoint is present and executable in the final image
RUN test -f /entrypoint.sh && test -x /entrypoint.sh

# Add a bash init hook that prints even before entrypoint.sh body runs (non-interactive bash only)
RUN printf '%s\n' \
  'echo "[BASH_ENV] bash invoked: pid=$$ uid=$(id -u) gid=$(id -g) argv0=$0" >&2' \
  > /etc/bash.bash_env && chmod 0644 /etc/bash.bash_env
ENV BASH_ENV=/etc/bash.bash_env

# Ensure executable bits and world readability for runtime user (non-root)
RUN chmod 0755 /usr/local/bin/machinepal /usr/local/bin/first_run.sh /entrypoint.sh \
    && chmod 0755 /etc/service/machinepal/run /etc/service/machinepal/finish \
    && chown root:root /usr/local/bin/machinepal /usr/local/bin/first_run.sh /entrypoint.sh \
    && chown root:root /etc/service/machinepal/run /etc/service/machinepal/finish

USER machinepal

# Encourage unbuffered/line-buffered stdout/stderr for processes inside the container
ENV PYTHONUNBUFFERED=1 \
    STDOUT_LINE_BUFFERED=1

# Use bash explicitly to avoid shebang/CRLF/exec-format edge cases
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]
CMD [""]
