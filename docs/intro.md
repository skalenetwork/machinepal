---
sidebar_position: 1
---

<div align="center">
  <a href="https://github.com/skalenetwork/machinepay">
    <img src="/img/machinepay.png" alt="MachinePay Logo" width="250" />
  </a>
</div>

<h1 align="center">🚀 MachinePay: The Internet's Missing Money Layer</h1>

<p align="center">
  Ever wished you could charge a few cents for an API call, a file download, or access to a premium article—without forcing users through clunky sign-up forms and credit card fields?
</p>

<p align="center">
  <strong>Your wish is granted.</strong>
</p>

<p align="center">
  MachinePay is a <strong>plug-and-play payment gatekeeper</strong> that lets you instantly add crypto-powered micropayments to <em>any</em> website or API using the <a href="https://docs.cdp.coinbase.com/x402/docs/welcome">x402 protocol</a>.
</p>

<div align="center">

[![Love this project? Give it a heart!](https://img.shields.io/badge/%E2%9D%A4%EF%B8%8F%20Love%20this%20project%3F%20Give%20it%20a%20heart!-ff69b4?style=flat-square)](https://github.com/skalenetwork/machinepay/stargazers)
[![GitHub stars](https://img.shields.io/github/stars/skalenetwork/machinepay?style=social)](https://github.com/skalenetwork/machinepay/stargazers)
[![Contribute](https://img.shields.io/badge/Become%20a%20Contributor-28a745?logo=github)](https://github.com/skalenetwork/machinepay/blob/main/CONTRIBUTING.md)
[![Open an Issue](https://img.shields.io/badge/Open%20an%20Issue-ff9800?logo=github)](https://github.com/skalenetwork/machinepay/issues/new/choose)

</div>

---

## ✨ Stop Giving It Away. Start Earning.

MachinePay is built for creators, developers, and businesses who want to monetize their digital content frictionlessly.

- ⚡ **Go Live in 5 Minutes:** Add x402 payments to your existing infrastructure with almost no code.
- 🌉 **Multi-Chain by Default:** Natively supports both **Base** and the gas-less **SKALE Network**.
- 🚀 **Built for Scale:** The asynchronous core handles over 1 million concurrent connections. Don't blink.
- 💸 **You're in Control:** Offer subscriptions, pay-per-request, or metered access. It's your business.
- 🛠️ **Deploy Anywhere:** Run it on your own hardware, a cloud server, or as a Docker container.
- 💯 **Free & Open Source:** Built for the community, by the community.

---

## 🏗️ How It Works: The Digital Toll Booth

Think of **MachinePay** as a **toll booth for your digital highway**. Instead of requests hitting your website or API directly, they first pass through the gatekeeper.

1.  **🔗 A user requests a resource** → `GET /my-secret-api-endpoint`
2.  **🚦 MachinePay intercepts** → Sees no payment is included.
3.  **🛑 Access Denied (for now!)** → MachinePay sends back a `402 Payment Required` error, including a crypto invoice to pay.
4.  **💳 User's browser/client pays the invoice** → The request is sent again, this time with proof of payment.
5.  **✅ Payment Verified** → MachinePay confirms the on-chain payment.
6.  **📡 Request Forwarded** → The original request is now sent to your real website/API.
7.  **📬 Content Delivered** → Your server's response is passed back to the user.

The result? You just monetized your content without lifting a finger. Secure, compliant, and instant.
