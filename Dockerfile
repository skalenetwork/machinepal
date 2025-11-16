# syntax=docker/dockerfile:1

FROM ubuntu:22.04

# Install runtime dependencies and runit
RUN apt-get update && \
    apt-get install -y \
        libstdc++6 \
        runit \
        && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /machinepay

# Copy the built executables
COPY build/machinepay /usr/bin/machinepay
COPY build/mptest /usr/bin/mptest

# Copy runit service script
COPY --chmod=755 run_machinepay.sh /etc/service/machinepay/run

# Set entrypoint to runit
ENTRYPOINT ["/usr/sbin/runsvdir", "/etc/service"]
