#pragma once




class OrganizationConfig;
class ResourceConfig;
class MachinePayConfig;

class X402PaymentRequirements {

public:

    static std::string getPaymentRequirementsAsString(ptr<OrganizationConfig> organization,
                                                              ptr<ResourceConfig> resource,
                                                              ptr<MachinePayConfig> config);
};

