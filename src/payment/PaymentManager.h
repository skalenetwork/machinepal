#pragma once

#include <memory>

class MachinePayApp; // forward declaration

class PaymentManager {
public:
    explicit PaymentManager(MachinePayApp& app);
    MachinePayApp& app() const { return app_; }
private:
    MachinePayApp& app_;
};
