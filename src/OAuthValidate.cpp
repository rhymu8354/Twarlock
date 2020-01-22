/**
 * @file OAuthValidate.cpp
 *
 * This module defines the Twarlock::OAuthValidate command.
 *
 * © 2019 by Richard Walters
 */

#include "Commands.hpp"
#include "Environment.hpp"

#include <future>
#include <inttypes.h>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>

using namespace Twarlock;

namespace {

    bool OAuthValidate(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        const auto done = std::make_shared< std::promise< void > >();
        twitch.PostApiCall(
            Twitch::Api::OAuth2,
            "validate",
            [&](Json::Value&& response){
                const std::string login = response["login"];
                printf("Login: %s\n", login.c_str());
                const intmax_t expiresIn = response["expires_in"];
                printf("Expires in: %" PRIdMAX "\n", expiresIn);
                const auto& scopes = response["scopes"];
                printf("Scopes:\n");
                for (size_t i = 0; i < scopes.GetSize(); ++i) {
                    const std::string scope = scopes[i];
                    printf("  %s\n", scope.c_str());
                }
                done->set_value();
            },
            [&](unsigned int statusCode){
                done->set_value();
            }
        );
        done->get_future().get();
        return true;
    };

    struct RegisterInfo {
        RegisterInfo() {
            Command command;
            command.cmdSummary = "Validate OAuth token";
            command.cmdDetails = (
                "Validate configured OAuth token."
            );
            command.argSummary = "";
            command.argDetails = {
            };
            command.execute = OAuthValidate;
            Commands::Add("oauth-validate", std::move(command));
        }
    } registerInfo;

}
