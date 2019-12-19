/**
 * @file Api.cpp
 *
 * This module defines the Twarlock::Api command.
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

    bool Kraken(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        if (environment.args.empty()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "resource expected"
            );
            return false;
        }
        const auto done = std::make_shared< std::promise< void > >();
        twitch.PostApiCall(
            Twitch::Api::Kraken,
            environment.args[0],
            [&](Json::Value&& response){
                Json::EncodingOptions encodingOptions;
                encodingOptions.reencode = true;
                encodingOptions.pretty = true;
                printf("%s\n", response.ToEncoding(encodingOptions).c_str());
                done->set_value();
            },
            [&](unsigned int statusCode){
                done->set_value();
            }
        );
        done->get_future().get();
        return true;
    };

    bool Helix(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        if (environment.args.empty()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "resource expected"
            );
            return false;
        }
        const auto done = std::make_shared< std::promise< void > >();
        twitch.PostApiCall(
            Twitch::Api::Helix,
            environment.args[0],
            [&](Json::Value&& response){
                Json::EncodingOptions encodingOptions;
                encodingOptions.reencode = true;
                encodingOptions.pretty = true;
                printf("%s\n", response.ToEncoding(encodingOptions).c_str());
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
            Command krakenCommand;
            krakenCommand.cmdSummary = "Issue a Kraken API request";
            krakenCommand.cmdDetails = (
                "Issue a Kraken API request."
            );
            krakenCommand.argSummary = "<RESOURCE>";
            krakenCommand.argDetails = {
                {"RESOURCE", "Name of the API endpoint resource to request"},
            };
            krakenCommand.execute = Kraken;
            Commands::Add("kraken", std::move(krakenCommand));
            Command helixCommand;
            helixCommand.cmdSummary = "Issue a Helix API request";
            helixCommand.cmdDetails = (
                "Issue a Helix API request."
            );
            helixCommand.argSummary = "<RESOURCE>";
            helixCommand.argDetails = {
                {"RESOURCE", "Name of the API endpoint resource to request"},
            };
            helixCommand.execute = Helix;
            Commands::Add("helix", std::move(helixCommand));
        }
    } registerInfo;

}
