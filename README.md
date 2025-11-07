<div align="center">
  <img src="machinepay.png" alt="MachinePay" width="30%">
</div>


# 🚀 MachinePay: The  x402 Payment Gatekeeper for the Web

Instantly add **crypto payments** to any website or API using the [x402 protocol](https://docs.cdp.coinbase.com/x402/docs/welcome).

[![Love this project? Give it a heart!](https://img.shields.io/badge/%E2%9D%A4%EF%B8%8F%20Love%20this%20project%3F%20Give%20it%20a%20heart!-ff69b4?style=flat-square)](https://github.com/skalenetwork/machinepay/stargazers)
[![GitHub stars](https://img.shields.io/github/stars/skalenetwork/machinepay?style=social)](https://github.com/skalenetwork/machinepay/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/skalenetwork/machinepay?style=social)](https://github.com/skalenetwork/machinepay/network/members)
[![Contribute](https://img.shields.io/badge/Become%20a%20Contributor-28a745?logo=github)](https://github.com/skalenetwork/machinepay/blob/main/CONTRIBUTING.md)
[![Open an Issue](https://img.shields.io/badge/Open%20an%20Issue-ff9800?logo=github)](https://github.com/skalenetwork/machinepay/issues/new/choose)
[![GitHub issues](https://img.shields.io/github/issues/skalenetwork/machinepay)](https://github.com/skalenetwork/machinepay/issues)
[![GitHub license](https://img.shields.io/github/license/skalenetwork/machinepay)](https://github.com/skalenetwork/machinepay/blob/main/LICENSE)
[![Build and test machinepay](https://github.com/skalenetwork/machinepay/actions/workflows/build-test-and-publish.yml/badge.svg)](https://github.com/skalenetwork/machinepay/actions/workflows/build_test_and_publish.yml)
[![Runs on Ubuntu 22.04+](https://img.shields.io/badge/Ubuntu-22.04%2B-orange?logo=ubuntu)](https://ubuntu.com/)
[![Runs on macOS 11+](https://img.shields.io/badge/macOS-11%2B-blue?logo=apple)](https://www.apple.com/macos/)
[![Runs on Windows 10+](https://img.shields.io/badge/Windows-10%2B-blue?logo=windows)](https://www.microsoft.com/windows/)

---

## ✨ Why use MachinePay?

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

Think of **MachinePay** as a **toll booth for the internet**.  
Instead of reaching a website directly, requests first pass through the proxy:

1. **🔗 You send a request** → try to access a resource
2. **🚦 MachinePay proxy intercepts** → checks if payment is included
3. **💳 Payment verified** → confirmed via the x402 protocol
4. **📡 MachinePay proxy forwards request** → to the real website
5. **🖥️ Website responds** → returns content
6. **📬 MachinePay proxy delivers to you** → completing the paid access loop

✅ Result: Websites instantly monetize access while staying secure and compliant.

---

## ⚡ Build Instructions

```bash
# Clone with dependencies
git clone --recursive https://github.com/skalenetwork/machinepay.git
cd machinepay

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


```mermaid
flowchart LR
subgraph External_World["External Users & AI Agents"]
U1["Human Users"]
A1["AI Agents"]
end

    subgraph Corporate_Network["Corporate Network"]
        direction TB
        MP["MachinePay Server (x402)\n(edge gateway)"]
        AI1["AI Services"]
        AI2["Documents / Corp Knowledge"]
        AI3["Data Streams"]
    end

    U1 -->|Requests / Payments\nvia x402| MP
    A1 -->|Autonomous Access\nvia x402| MP

    MP -->|Authorized Access| AI1
    MP -->|Authorized Access| AI2
    MP -->|Authorized Access| AI3

    %% Added color styling without changing content
    classDef users fill:#4caf50,stroke:#1b5e20,color:#ffffff,font-weight:600;
    classDef agents fill:#1976d2,stroke:#0d47a1,color:#ffffff,font-weight:600;
    classDef gateway fill:#ff9800,stroke:#e65100,color:#000000,font-weight:600;
    classDef services fill:#9c27b0,stroke:#4a148c,color:#ffffff,font-weight:600;
    classDef docs fill:#3f51b5,stroke:#1a237e,color:#ffffff,font-weight:600;
    classDef streams fill:#009688,stroke:#004d40,color:#ffffff,font-weight:600;

    U1:::users
    A1:::agents
    MP:::gateway
    AI1:::services
    AI2:::docs
    AI3:::streams

    style External_World fill:#f1f8e9,stroke:#c5e1a5,color:#2e7d32;
    style Corporate_Network fill:#e3f2fd,stroke:#90caf9,color:#0d47a1;

    linkStyle 0 stroke:#4caf50,color:#4caf50;
    linkStyle 1 stroke:#1976d2,color:#1976d2;
    linkStyle 2 stroke:#9c27b0,color:#9c27b0;
    linkStyle 3 stroke:#3f51b5,color:#3f51b5;
    linkStyle 4 stroke:#009688,color:#009688;
```
