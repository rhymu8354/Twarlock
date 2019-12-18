/**
 * @file Twitch.cpp
 *
 * This module contains the implementation of the Twarlock::Twitch class.
 *
 * © 2019 by Richard Walters
 */

#include "Twitch.hpp"

#include <AsyncData/MultiProducerSingleConsumerQueue.hpp>
#include <condition_variable>
#include <future>
#include <Http/Client.hpp>
#include <HttpNetworkTransport/HttpClientNetworkTransport.hpp>
#include <inttypes.h>
#include <map>
#include <memory>
#include <mutex>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/NetworkConnection.hpp>
#include <thread>
#include <TlsDecorator/TlsDecorator.hpp>

namespace {

    constexpr double twitchApiLookupCooldown = 1.0;

    template< typename T > void WithoutLock(
        T& lock,
        std::function< void() > f
    ) {
        lock.unlock();
        f();
        lock.lock();
    }

}

namespace Twarlock {

    /**
     * This contains the private properties of a Twitch class instance.
     */
    struct Twitch::Impl {
        // Properties

        bool apiCallInProgress = false;
        AsyncData::MultiProducerSingleConsumerQueue< std::function< void() > > apiCalls;
        std::string caCerts;
        Json::Value configuration;
        SystemAbstractions::DiagnosticsSender diagnosticsSender;
        std::shared_ptr< Http::Client > httpClient = std::make_shared< Http::Client >();

        /**
         * This holds onto pending HTTP request transactions being made.
         *
         * The keys are unique identifiers.  The values are the transactions.
         */
        std::map< int, std::shared_ptr< Http::IClient::Transaction > > httpClientTransactions;

        std::recursive_mutex mutex;
        double nextApiCallTime = 0.0;

        /**
         * This is used to select unique identifiers as keys for the
         * httpClientTransactions collection.  It is incremented each
         * time a key is selected.
         */
        int nextHttpClientTransactionId = 1;

        std::weak_ptr< Impl > selfWeak;
        bool stopWorker = false;
        std::shared_ptr< Http::TimeKeeper > timeKeeper;
        std::condition_variable_any wakeWorker;
        std::thread worker;

        // Lifecycle

        ~Impl() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            Demobilize(lock);
        }
        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;

        // Methods

        Impl()
            : diagnosticsSender("Twitch")
        {
        }

        void Demobilize(std::unique_lock< decltype(mutex) >& lock) {
            if (!worker.joinable()) {
                return;
            }
            stopWorker = true;
            wakeWorker.notify_one();
            if (std::this_thread::get_id() == worker.get_id()) {
                worker.detach();
            } else {
                WithoutLock(lock, [this]{ worker.join(); });
            }
        }

        void Mobilize(
            Json::Value&& configuration,
            std::string&& caCerts,
            std::shared_ptr< Http::TimeKeeper >&& timeKeeper
        ) {
            if (worker.joinable()) {
                return;
            }
            this->configuration = std::move(configuration);
            this->caCerts = std::move(caCerts);
            this->timeKeeper = timeKeeper;
            stopWorker = false;
            worker = std::thread(&Impl::Worker, this);
        }

        void NextApiCall() {
            if (apiCalls.IsEmpty()) {
                nextApiCallTime = 0.0;
            } else {
                const auto apiCall = apiCalls.Remove();
                apiCall();
            }
        }

        void PostApiCall(
            Api api,
            const std::string& resource,
            std::function< void(Json::Value&& response) > onSuccess,
            std::function< void(unsigned int statusCode) > onFailure
        ) {
            apiCalls.Add(
                [
                    api,
                    resource,
                    onSuccess,
                    onFailure,
                    this
                ]{
                    Http::Request request;
                    std::string targetUriString;
                    switch (api) {
                        case Api::Kraken: {
                            targetUriString = std::string("https://api.twitch.tv/kraken/") + resource;
                            request.headers.SetHeader("Accept", "application/vnd.twitchtv.v5+json");
                        } break;

                        case Api::Helix: {
                            targetUriString = std::string("https://api.twitch.tv/helix/") + resource;
                        } break;

                        default: {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                                "Unknown API requested for: %s",
                                resource.c_str()
                            );
                            onFailure(400);
                            return;
                        } break;
                    }
                    apiCallInProgress = true;
                    const auto id = nextHttpClientTransactionId++;
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        0,
                        "Twitch API call %d request: %s",
                        id,
                        targetUriString.c_str()
                    );
                    request.method = "GET";
                    request.target.ParseFromString(targetUriString);
                    request.target.SetPort(443);
                    request.headers.SetHeader("Client-ID", configuration["clientId"]);
                    auto& httpClientTransaction = httpClientTransactions[id];
                    httpClientTransaction = httpClient->Request(request);
                    auto selfWeakCopy(selfWeak);
                    httpClientTransaction->SetCompletionDelegate(
                        [
                            id,
                            onSuccess,
                            onFailure,
                            targetUriString,
                            selfWeakCopy
                        ]{
                            auto impl = selfWeakCopy.lock();
                            if (impl == nullptr) {
                                return;
                            }
                            std::lock_guard< decltype(impl->mutex) > lock(impl->mutex);
                            impl->apiCallInProgress = false;
                            impl->nextApiCallTime = impl->timeKeeper->GetCurrentTime() + twitchApiLookupCooldown;
                            impl->wakeWorker.notify_one();
                            auto httpClientTransactionsEntry = impl->httpClientTransactions.find(id);
                            if (httpClientTransactionsEntry == impl->httpClientTransactions.end()) {
                                return;
                            }
                            const auto& httpClientTransaction = httpClientTransactionsEntry->second;
                            if (httpClientTransaction->response.statusCode == 200) {
                                impl->diagnosticsSender.SendDiagnosticInformationFormatted(
                                    0,
                                    "Twitch API call %d success: %s",
                                    id,
                                    httpClientTransaction->response.body.c_str()
                                );
                                onSuccess(Json::Value::FromEncoding(httpClientTransaction->response.body));
                            } else {
                                impl->diagnosticsSender.SendDiagnosticInformationFormatted(
                                    SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                                    "Twitch API call %d (%s) failure: %u",
                                    id,
                                    targetUriString.c_str(),
                                    httpClientTransaction->response.statusCode
                                );
                                onFailure(httpClientTransaction->response.statusCode);
                            }
                            (void)impl->httpClientTransactions.erase(httpClientTransactionsEntry);
                        }
                    );
                }
            );
            wakeWorker.notify_one();
        }

        void Worker() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            diagnosticsSender.SendDiagnosticInformationString(
                3,
                "Starting"
            );
            WorkerBody(lock);
            diagnosticsSender.SendDiagnosticInformationString(
                3,
                "Stopping"
            );
        }

        void WorkerBody(std::unique_lock< decltype(mutex) >& lock) {
            const auto diagnosticsPublisher = diagnosticsSender.Chain();
            (void)httpClient->SubscribeToDiagnostics(diagnosticsPublisher);
            Http::Client::MobilizationDependencies httpClientDeps;
            httpClientDeps.timeKeeper = timeKeeper;
            const auto transport = std::make_shared< HttpNetworkTransport::HttpClientNetworkTransport >();
            transport->SubscribeToDiagnostics(diagnosticsPublisher);
            transport->SetConnectionFactory(
                [
                    diagnosticsPublisher,
                    this
                ](
                    const std::string& scheme,
                    const std::string& serverName
                ) -> std::shared_ptr< SystemAbstractions::INetworkConnection > {
                    const auto decorator = std::make_shared< TlsDecorator::TlsDecorator >();
                    const auto connection = std::make_shared< SystemAbstractions::NetworkConnection >();
                    decorator->ConfigureAsClient(connection, caCerts, serverName);
                    return decorator;
                }
            );
            httpClientDeps.transport = transport;
            httpClient->Mobilize(httpClientDeps);
            while (!stopWorker) {
                auto now = timeKeeper->GetCurrentTime();
                if (
                    !apiCallInProgress
                    && (now >= nextApiCallTime)
                ) {
                    NextApiCall();
                }
                if (
                    !apiCallInProgress
                    && (nextApiCallTime != 0.0)
                ) {
                    const auto nowClock = std::chrono::system_clock::now();
                    now = timeKeeper->GetCurrentTime();
                    if (nextApiCallTime > now) {
                        const auto timeoutMilliseconds = (int)ceil(
                            (nextApiCallTime - now)
                            * 1000.0
                        );
                        wakeWorker.wait_until(
                            lock,
                            nowClock + std::chrono::milliseconds(timeoutMilliseconds)
                        );
                    }
                } else {
                    wakeWorker.wait(lock);
                }
            }
            httpClient->Demobilize();
        }
    };

    Twitch::~Twitch() noexcept = default;
    Twitch::Twitch(Twitch&&) noexcept = default;
    Twitch& Twitch::operator=(Twitch&&) noexcept = default;

    Twitch::Twitch()
        : impl_(new Impl())
    {
        impl_->selfWeak = impl_;
    }

    SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate Twitch::SubscribeToDiagnostics(
        SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
        size_t minLevel
    ) {
        return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    void Twitch::Mobilize(
        Json::Value configuration,
        std::string caCerts,
        std::shared_ptr< Http::TimeKeeper > timeKeeper
    ) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->Mobilize(
            std::move(configuration),
            std::move(caCerts),
            std::move(timeKeeper)
        );
    }

    void Twitch::Demobilize() {
        std::unique_lock< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->Demobilize(lock);
    }

    void Twitch::PostApiCall(
        Api api,
        const std::string& targetUriString,
        std::function< void(Json::Value&& response) > onSuccess,
        std::function< void(unsigned int statusCode) > onFailure
    ) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->PostApiCall(api, targetUriString, onSuccess, onFailure);
    }

    intmax_t Twitch::GetUserIdByName(const std::string& name) {
        const auto done = std::make_shared< std::promise< intmax_t > >();
        PostApiCall(
            Twitch::Api::Kraken,
            StringExtensions::sprintf("users?login=%s", name.c_str()),
            [&](Json::Value&& response){
                intmax_t userid;
                if (
                    sscanf(
                        ((std::string)response["users"][0]["_id"]).c_str(), "%" SCNdMAX,
                        &userid
                    ) == 1
                ) {
                    done->set_value(userid);
                } else {
                    impl_->diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                        "Twitch API returned invalid ID for user '%s'",
                        name.c_str()
                    );
                    done->set_value(0);
                }
            },
            [&](unsigned int statusCode){
                done->set_value(0);
            }
        );
        return done->get_future().get();
    }

}
