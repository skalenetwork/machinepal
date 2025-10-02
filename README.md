**Why use x402proxy?**

- It instantly adds x402 payments to existing websites and web APIs.
- It is fully x402 compliant.
- It is easy to deploy and configure.
- It is a high performance C++ based implementation, supporting large number of concurrent connections.
- It supports both Base and SKALE networks.
- It supports flexible payment models and x402 options.
- It provides detailed logging and monitoring.
- It is open source and free to use.

## How the x402proxy Reverse Proxy Architecture Works

Think of x402proxy as a toll booth on the internet. Instead of connecting to a website directly, your request goes through x402proxy, which verifies payment before allowing access to the destination.  

x402proxy uses the [**x402 protocol by Coinbase**](https://docs.cdp.coinbase.com/x402/docs/welcome) to charge for access to web resources. It supports payments on both **Base** and **SKALE** networks, making it compatible with multiple blockchains.  

Here’s the process step by step:  

1. **You send a request**  
   You try to access a webpage or resource.  

2. **x402proxy intercepts the request**  
   Your request doesn’t go straight to the website. Instead, it first reaches x402proxy, which checks whether payment has been included. If not, x402proxy prompts you to pay before proceeding.  

3. **Payment verification**  
   Once payment is confirmed via the x402 protocol, x402proxy authorizes the request.  

4. **x402proxy forwards the request**  
   The proxy relays your request to the actual website on your behalf.  

5. **The website responds**  
   The destination website sends its response back to x402proxy.  

6. **x402proxy delivers the response to you**  
   Finally, x402proxy returns the website’s response, completing the paid transaction loop.  

   x402proxy takes the website's response and passes it back to you.


# Build instructions

Clone project, install and build dependencies

```bash
git clone --recursive git@github.com:skalenetwork/x402proxy.git 
cd x402proxy/external/vcpkg
./bootstrap-vcpkg.sh 
export PATH=$PATH:$PWD 
cd ../..
external/vcpkg/vcpkg install
```

Build the project

```
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_FEATURE_FLAGS=manifests
cmake --build build --parallel
```



