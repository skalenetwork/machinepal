<div align="center">
  <img src="machinepay.png" alt="MachinePay" width="30%">
</div>


# 🚀 MachinePay: The  x402 Payment Gatekeeper for the Web

Instantly add **crypto payments** to any website or API using the [x402 protocol](https://docs.cdp.coinbase.com/x402/docs/welcome).


[![Build and test proxy](https://github.com/skalenetwork/machinepay/actions/workflows/build_test_and_publish.yml/badge.svg)](https://github.com/kladkogex/machinepay/actions/workflows/build_test_and_publish.yml)

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
git clone --recursive https://github.com/skalenetwork/x402proxy.git
cd x402proxy

# Bootstrap vcpkg
./external/vcpkg/bootstrap-vcpkg.sh
./external/vcpkg/vcpkg install

# Build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_FEATURE_FLAGS=manifests   -DVCPKG_TARGET_TRIPLET=x64-linux 

cmake --build build -j
