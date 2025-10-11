#pragma once


class MachinePayApp;

class X402Processor
{
public:
    explicit X402Processor(MachinePayApp& app);
private:
    MachinePayApp& app_;
};
