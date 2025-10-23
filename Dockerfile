# syntax=docker/dockerfile:1

FROM ubuntu:22.04

# Install runtime dependencies (adjust as needed)
RUN apt-get update && \
    apt-get install -y \
        libstdc++6 \
        libboost-system1.74.0 \
        libboost-thread1.74.0 \
        libboost-locale1.74.0 \
        libboost-url1.74.0 \
        libboost-multiprecision1.74.0 \
        libcurl4 \
        libyaml-cpp0.7 \
        && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy the built executable from the build context
COPY build/machinepay .

# Set entrypoint
ENTRYPOINT ["./machinepay"]