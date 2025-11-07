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

