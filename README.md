<div align="center">
  <img src="machinepal.png" alt="MachinePal" width="30%">
</div>


# RunMachinePal Now

In an empty directory, run:

```bash
docker run --rm -v "$PWD:/machinepay" ghcr.io/skalenetwork/machinepal --init
```

This initializes MachinePal with the default configuration in the current folder.

Then run:

```bash
docker run --network host -v "$PWD:/machinepay" ghcr.io/skalenetwork/machinepal
```

> Note: `--network host` (with a space) is the correct Docker syntax. On Linux it attaches the container to the host network.



# 🚀 MachinePal: The  x402 Payment Agent for the Web

Instantly add **crypto payments** to any website or API using this AI Payment Agent that implements [x402 protocol](https://docs.cdp.coinbase.com/x402/docs/welcome). Sell anything: APIs, resources and products.

[![Love this project? Give it a heart!](https://img.shields.io/badge/%E2%9D%A4%EF%B8%8F%20Love%20this%20project%3F%20Give%20it%20a%20heart!-ff69b4?style=flat-square)](https://github.com/skalenetwork/machinepal/stargazers)
[![GitHub stars](https://img.shields.io/github/stars/skalenetwork/machinepal?style=social)](https://github.com/skalenetwork/machinepal/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/skalenetwork/machinepal?style=social)](https://github.com/skalenetwork/machinepal/network/members)
[![Contribute](https://img.shields.io/badge/Become%20a%20Contributor-28a745?logo=github)](https://github.com/skalenetwork/machinepal/blob/main/CONTRIBUTING.md)
[![Open an Issue](https://img.shields.io/badge/Open%20an%20Issue-ff9800?logo=github)](https://github.com/skalenetwork/machinepal/issues/new/choose)
[![GitHub issues](https://img.shields.io/github/issues/skalenetwork/machinepal)](https://github.com/skalenetwork/machinepal/issues)
[![GitHub license](https://img.shields.io/github/license/skalenetwork/machinepal)](https://github.com/skalenetwork/machinepal/blob/main/LICENSE)
[![Build and test machinepal](https://github.com/skalenetwork/machinepal/actions/workflows/build-test-and-publish.yml/badge.svg)](https://github.com/skalenetwork/machinepal/actions/workflows/build_test_and_publish.yml)
[![Runs on Ubuntu 22.04+](https://img.shields.io/badge/Ubuntu-22.04%2B-orange?logo=ubuntu)](https://ubuntu.com/)
[![Runs on macOS 11+](https://img.shields.io/badge/macOS-11%2B-blue?logo=apple)](https://www.apple.com/macos/)
[![Runs on Windows 10+](https://img.shields.io/badge/Windows-10%2B-blue?logo=windows)](https://www.microsoft.com/windows/)

---

## ✨ Why use MachinePal? 

- ⚡ **Plug & play** — add x402 payments to existing websites & APIs in minutes
- 🔒 **Fully x402 compliant** — built on the Coinbase standard
- 🛠️ **Easy to deploy & configure** — no complex setup
- 🚀 **Ultra-high performance asynchronous HTTP server** — scales to 1M+ concurrent connections
- 🌉 **Multi-chain support** — works with both **Base** and **SKALE**
- 💸 **Flexible payment models** — subscriptions, pay-per-request, metered access
- 📊 **Deep logging & monitoring** — full visibility of payment traffic
- 💯 **Open source & free** — community-driven

---

## 🏗️ How it Works (The Toll Booth Analogy)

Think of **MachinePal** as a **toll booth for the internet**.  
Instead of reaching a website directly, requests first pass through the proxy:

1. **🔗 You send a request** → try to access a resource
2. **🚦 MachinePal proxy intercepts** → checks if payment is included
3. **💳 Payment verified** → confirmed via the x402 protocol
4. **📡 MachinePal proxy forwards request** → to the real website
5. **🖥️ Website responds** → returns content
6. **📬 MachinePal proxy delivers to you** → completing the paid access loop

✅ Result: Websites instantly monetize access while staying secure and compliant.

---


## MachinePal Architecture diagram


```mermaid
flowchart LR
subgraph External_World["External Users & AI Agents"]
U1["Consumers"]
A1["AI Agents"]
end

    %% Corporate Network 1
    subgraph Corporate_Network["Corporate Network 1"]
        direction TB
        MP["MachinePal x402 payment gateway"]
        AI1["AI Services"]
        AI2["Products"]
        AI3["Data"]
        HU1["Employees"]
        HA1["AI Agents"]
    end

    %% Corporate Network 2
    subgraph Corporate_Network_2["Corporate Network 2"]
        direction TB
        MP2["MachinePal x402 payment gateway"]
        AI1B["AI Services"]
        AI2B["Products"]
        AI3B["Data"]
        HU2["Employees"]
        HA2["AI Agents"]
    end

    %% External access to Network 1
    U1 -->|Requests / Payments via x402| MP
    A1 -->|Autonomous Access via x402| MP

    %% Authorized access (Network 1)
    MP -->|Authorized Access| AI1
    MP -->|Authorized Access| AI2
    MP -->|Authorized Access| AI3

    %% Internal users (Network 1)
    HU1 -->|Internal Requests / Payments via x402| MP
    HA1 -->|Internal Autonomous Access via x402| MP

    %% Internal users (Network 2)
    HU2 -->|Internal Requests / Payments via x402| MP2
    HA2 -->|Internal Autonomous Access via x402| MP2

    %% Authorized access (Network 2)
    MP2 -->|Authorized Access| AI1B
    MP2 -->|Authorized Access| AI2B
    MP2 -->|Authorized Access| AI3B

    %% Aggregated cross-network paid resource exchange (bidirectional)
    MP <-->|Cross-Network Paid Resource Access| MP2

    %% Styling
    classDef users fill:#4caf50,stroke:#1b5e20,color:#ffffff,font-weight:600;
    classDef agents fill:#1976d2,stroke:#0d47a1,color:#ffffff,font-weight:600;
    classDef gateway fill:#ff9800,stroke:#e65100,color:#000000,font-weight:600;
    classDef services fill:#9c27b0,stroke:#4a148c,color:#ffffff,font-weight:600;
    classDef docs fill:#3f51b5,stroke:#1a237e,color:#ffffff,font-weight:600;
    classDef streams fill:#009688,stroke:#004d40,color:#ffffff,font-weight:600;

    U1:::users
    A1:::agents
    HU1:::users
    HA1:::agents
    HU2:::users
    HA2:::agents
    MP:::gateway
    MP2:::gateway
    AI1:::services
    AI2:::docs
    AI3:::streams
    AI1B:::services
    AI2B:::docs
    AI3B:::streams

    style Corporate_Network fill:#e3f2fd,stroke:#90caf9,color:#0d47a1;
    style Corporate_Network_2 fill:#fce4ec,stroke:#f06292,color:#880e4f;
    style External_World fill:#f1f8e9,stroke:#c5e1a5,color:#2e7d32;

    %% Link styling (external + aggregated cross-network)
    linkStyle 0 stroke:#4caf50,color:#4caf50,stroke-width:2px;
    linkStyle 1 stroke:#1976d2,color:#1976d2,stroke-width:2px;
    %% Cross-network edge index (after preceding edges) is 12
    linkStyle 12 stroke:#ff9800,color:#ff9800,stroke-width:3px,stroke-dasharray:4 2;
```





## ⚡ Build Instructions


Install prerequisites:
```bash
    apt-get update && \
    apt-get install -y --no-install-recommends \
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
        bison \
        flex \
```

```bash
# Clone with dependencies
git clone --recursive https://github.com/skalenetwork/machinepal.git
cd machinepal

# Bootstrap vcpkg
./external/vcpkg/bootstrap-vcpkg.sh
./external/vcpkg/vcpkg install

# Build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_FEATURE_FLAGS=manifests   -DVCPKG_TARGET_TRIPLET=x64-linux 

cmake --build build -j
```
