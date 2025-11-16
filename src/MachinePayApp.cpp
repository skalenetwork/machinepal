#include "MachinePayApp.h"
#include "db/MachinePayDb.h"
#include "facilitator_clients/FacilitatorClientManager.h"
#include "facilitators/EasyNetFacilitator.h"

MachinePayApp::MachinePayApp(const std::map<std::string, std::string>& configValuesFromCliAndEnv)
{
    try
    {
        spdlog::info("Looking for config");
        configManager_ = ConfigManager::initManager(configValuesFromCliAndEnv);
        configPath_ = configManager_->fileManager()->canonicalConfigDirPath();
        Init::initLogLevelFromConfig(configManager());
        paymentManager_ = std::make_shared<PaymentManager>(*this);
        facilitatorClientManager_ = std::make_shared<FacilitatorClientManager>(*this);
        easyNetFacilitator_ = std::make_shared<EasyNetFacilitator>(*this);
        //machinePayDB_ = std::make_shared<MachinePayDb>(*this, DbType::SQLite);
        machinePayDB_ = std::make_shared<EasyNetDb>(*this, DbType::SQLite);
    }
    catch (...)
    {
        RETHROW_NESTED2("Failed to initialize MachinePayApp");
    }
}
ptr< FacilitatorClientManager > MachinePayApp::facilitatorClientManager() const {
    CHECK_STATE( facilitatorClientManager_ );
    return facilitatorClientManager_;
}
ptr< EasyNetFacilitator > MachinePayApp::easyNetFacilitator() const {
    CHECK_STATE( easyNetFacilitator_);
    return easyNetFacilitator_;
}


static std::atomic<int> sigReceived{0};


static void machinepayTerminateSignalHandler(int sig)
{
    sigReceived = sig;
}

uint32_t MachinePayApp::runUntilExit()
{
    std::signal(SIGTERM, machinepayTerminateSignalHandler);
    std::signal(SIGINT, machinepayTerminateSignalHandler);


    try
    {
        spdlog::info("Creating and starting machinepay server");
        serverFactory_ = std::make_shared<ServerFactory>(*this);
        auto serverConfig = configManager_->latestConfig()->server();

        proxygenServer_ = serverFactory_->createServerInstance(*serverConfig);

        spdlog::info("Creating thread pool");
        auto ioExecutor = std::make_shared<folly::IOThreadPoolExecutor>(
            256,
            std::make_shared<folly::NamedThreadFactory>("x402Processor"));

        spdlog::info("Starting machinepay server");

        auto onSuccess = [this]()
        {
            spdlog::info("Machinepay server started successfully.");
            this->isStarted_ = true;
        };
        auto onError = [this](std::exception_ptr eptr)
        {
            try
            {
                if (eptr) std::rethrow_exception(eptr);
            }
            catch (const std::exception& ex)
            {
                spdlog::error("Machinepay server failed to start: {}", ex.what());
                this->setExited(1, ex.what());
                return;
            }
            catch (...)
            {
            }
            spdlog::error("Machinepay server failed to start: unknown error");
            this->setExited(1, "Machinepay server failed to start: unknown error");
        };

        std::thread serverThread([this, ioExecutor, onSuccess, onError]()
        {
            try
            {
                proxygenServer_->start(onSuccess, onError, nullptr, ioExecutor);
                setExited();
            }
            catch (...)
            {
                spdlog::error("Proxygen server failed to start: unknown error");
                setExited(1, "Unknown error starting machinepay server");
            }
        });

        uint64_t counter = 0;

        while (!isExited() && !sigReceived)
        {
            counter++;
            if (counter == 10) {
                // do a self test of the facilitator one second after start
                try {
                    this->configManager()->latestConfig()->facilitatorClient()->selfTest();
                } catch (const std::exception& ex) {
                    spdlog::error("Facilitator self test failed: {}", ex.what());
                } catch (...) {
                    spdlog::error("Facilitator self test failed: unknown error");
                }
            }
            usleep(100 * 1000);
        }
        if (sigReceived)
        {
            if (sigReceived == SIGINT)
                spdlog::info("SIGINT (Ctrl-C) received, stopping server.");
            else if (sigReceived == SIGTERM)
                spdlog::info("SIGTERM received, stopping server.");
            else
            {
                CHECK_STATE2(false, std::string("Unexpected signal {}") + to_string(sigReceived.load()));
            }
            stopServer();
        }
        // Wait for server to exit after stopServer is called
        while (!isExited())
        {
            usleep(100 * 1000);
        }


        serverThread.join();
        if (exitCode_ != 0)
        {
            spdlog::error("Error running machinepay server: {}. Server exited.", exitErrorMessage_);
            return exitCode_;
        }
        spdlog::info("Machinepay server exited normally.");
        return 0;
    }
    catch (const std::exception& ex)
    {
        spdlog::critical("Fatal error running machinepay server: {}. Server exited", ex.what());
        printNestedException(ex);
        return 1;
    }
}

void MachinePayApp::stopServer()
{
    if (serverStopCalled_.exchange(true))
        return;

    proxygenServer_->stop();
}

std::weak_ptr<MachinePayApp> MachinePayApp::sLatestInstance;