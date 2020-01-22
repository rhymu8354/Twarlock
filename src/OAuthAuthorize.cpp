/**
 * @file OAuthAuthorize.cpp
 *
 * This module defines the Twarlock::OAuthAuthorize command.
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

    bool OAuthAuthorize(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        if (environment.args.size() < 1) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "Redirect URI required"
            );
            return false;
        }
        const auto redirectUri = environment.args[0];
        if (environment.args.size() < 2) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "At least one OAuth scope required"
            );
            return false;
        }
        auto url = StringExtensions::sprintf(
            "id.twitch.tv/oauth2/authorize?client_id=%s&redirect_uri=%s&response_type=token&scope=",
            ((std::string)environment.configuration["clientId"]).c_str(),
            redirectUri.c_str()
        );
        for (size_t i = 1; i < environment.args.size(); ++i) {
            if (i != 1) {
                url += "%20";
            }
            url += environment.args[i];
        }
        const auto done = std::make_shared< std::promise< void > >();
        twitch.PostApiCall(
            Twitch::Api::RawGet,
            url,
            [&](Json::Value&& response){
                printf("%s\n", response.ToEncoding().c_str());
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
            command.cmdSummary = "Get OAuth token";
            command.cmdDetails = (
                "Get OAuth token using OICE implicit code flow."
            );
            command.argSummary = "<REDIR> <SCOPE>...";
            command.argDetails = {
                {"REDIR", "Redirect URI"},
                {"SCOPE", "A scope to request for the new token"},
            };
            command.execute = OAuthAuthorize;
            Commands::Add("oauth-authorize", std::move(command));
        }
    } registerInfo;

}
