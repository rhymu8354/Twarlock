/**
 * @file OAuthRevoke.cpp
 *
 * This module defines the Twarlock::OAuthRevoke command.
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

    bool OAuthRevoke(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        const auto done = std::make_shared< std::promise< void > >();
        twitch.PostApiCall(
            Twitch::Api::RawPost,
            StringExtensions::sprintf(
                "id.twitch.tv/oauth2/revoke?client_id=%s&token=%s",
                ((std::string)environment.configuration["clientId"]).c_str(),
                ((std::string)environment.configuration["oauthToken"]).c_str()
            ),
            [&](Json::Value&& response){
                printf("OAuth token revoked.\n");
                done->set_value();
            },
            [&](unsigned int statusCode){
                printf("OAuth token invalid.\n");
                done->set_value();
            }
        );
        done->get_future().get();
        return true;
    };

    struct RegisterInfo {
        RegisterInfo() {
            Command command;
            command.cmdSummary = "Revoke OAuth token";
            command.cmdDetails = (
                "Revoke configured OAuth token."
            );
            command.argSummary = "";
            command.argDetails = {
            };
            command.execute = OAuthRevoke;
            Commands::Add("oauth-revoke", std::move(command));
        }
    } registerInfo;

}
