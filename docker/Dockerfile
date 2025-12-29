# syntax=docker/dockerfile:1

# ==========================================
# STAGE 1: Builder
# ==========================================
FROM ghcr.io/skalenetwork/machinepal-build:latest AS builder
FROM ubuntu:22.04 AS runtime

# Install Runtime Dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        libstdc++6 \
        coreutils \
        gosu \
    && rm -rf /var/lib/apt/lists/*


WORKDIR /machinepal


COPY --from=builder /app/machinepal/build/machinepal /usr/local/bin/machinepal
COPY --from=builder /app/machinepal/build/mptest /usr/local/bin/mptest
COPY --chmod=755  ./entrypoint.sh /entrypoint.sh


# Ensure entrypoint is present and executable in the final image
RUN test -f /entrypoint.sh && test -x /entrypoint.sh

# Add a bash init hook that prints even before entrypoint.sh body runs (non-interactive bash only)
RUN printf '%s\n' \
  'echo "[BASH_ENV] bash invoked: pid=$$ uid=$(id -u) gid=$(id -g) argv0=$0" >&2' \
  > /etc/bash.bash_env && chmod 0644 /etc/bash.bash_env
ENV BASH_ENV=/etc/bash.bash_env

# Ensure executable bits and world readability for runtime user (non-root)
RUN chmod 0755 /usr/local/bin/machinepal /entrypoint.sh \
    && chown root:root /usr/local/bin/machinepal /entrypoint.sh

# Encourage unbuffered/line-buffered stdout/stderr for processes inside the container
ENV PYTHONUNBUFFERED=1 \
    STDOUT_LINE_BUFFERED=1

# Use bash explicitly to avoid shebang/CRLF/exec-format edge cases
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]
