#include "common.h"
#include "X402Processor.h"
#include "MachinePayApp.h"



X402Processor::X402Processor(MachinePayApp& app)
    : app_(app)
{
    // TODO: Add any initialization logic if needed
}

bool X402Processor::hasValidPaymentHeader(const proxygen::HTTPMessage* _req, std::string& _paymentInfo) {
    // TODO: Parse and verify real X-PAYMENT header.
    // This stub accepts "demo-ok".
    const auto& headerTable = _req->getHeaders();
    std::string payment = headerTable.getSingleOrEmpty("X-PAYMENT");
    if (payment.empty()) return false;
    if (payment == "demo-ok") {
        _paymentInfo = R"({\"txHash\":\"0xabc123...\",\"amount\":\"0.25\",\"asset\":\"USDC\",\"network\":\"base-1net\"})";
        return true;
    }
    return false;
}
