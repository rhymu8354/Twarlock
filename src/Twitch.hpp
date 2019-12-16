#pragma once

/**
 * @file Twitch.hpp
 *
 * This module declares the Twarlock::Twitch class.
 *
 * © 2019 by Richard Walters
 */

#include <functional>
#include <Http/TimeKeeper.hpp>
#include <Json/Value.hpp>
#include <memory>
#include <string>
#include <SystemAbstractions/DiagnosticsSender.hpp>

namespace Twarlock {

    /**
     * This is used to access Twitch APIs.
     */
    class Twitch {
        // Types
    public:
        enum class Api {
            Kraken,
            Helix,
        };

        // Lifecycle Methods
    public:
        ~Twitch() noexcept;
        Twitch(const Twitch&) = delete;
        Twitch(Twitch&&) noexcept;
        Twitch& operator=(const Twitch&) = delete;
        Twitch& operator=(Twitch&&) noexcept;

        // Public Methods
    public:
        /**
         * This is the constructor of the class.
         */
        Twitch();

        /**
         * This method forms a new subscription to diagnostic
         * messages published by this class.
         *
         * @param[in] delegate
         *     This is the function to call to deliver messages
         *     to this subscriber.
         *
         * @param[in] minLevel
         *     This is the minimum level of message that this subscriber
         *     desires to receive.
         *
         * @return
         *     A function is returned which may be called
         *     to terminate the subscription.
         */
        SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        );

        void Mobilize(
            Json::Value configuration,
            std::string caCerts,
            std::shared_ptr< Http::TimeKeeper > timeKeeper
        );

        void Demobilize();

        void PostApiCall(
            Api api,
            const std::string& resource,
            std::function< void(Json::Value&& response) > onSuccess,
            std::function< void(unsigned int statusCode) > onFailure
        );

        // Private properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance.  It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This contains the private properties of the instance.
         */
        std::shared_ptr< Impl > impl_;
    };

}
