/**
 * @file Info.cpp
 *
 * This module defines the Twarlock::Info command.
 *
 * © 2019 by Richard Walters
 */

#include "Commands.hpp"
#include "Environment.hpp"

#include <future>
#include <inttypes.h>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>

namespace Twarlock {

    bool Info(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        if (environment.args.empty()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "channel name expected"
            );
            return false;
        }
        const auto done = std::make_shared< std::promise< void > >();
        twitch.PostApiCall(
            Twitch::Api::Kraken,
            StringExtensions::sprintf(
                "users?login=%s",
                environment.args[0].c_str()
            ),
            [&](Json::Value&& response){
                intmax_t userid;
                if (
                    sscanf(
                        ((std::string)response["users"][0]["_id"]).c_str(), "%" SCNdMAX,
                        &userid
                    ) == 1
                ) {
                    printf(
                        "User '%s' has id: %" PRIdMAX "\n",
                        environment.args[0].c_str(),
                        userid
                    );
                    twitch.PostApiCall(
                        Twitch::Api::Kraken,
                        StringExtensions::sprintf(
                            "channels/%" PRIdMAX,
                            userid
                        ),
                        [&](Json::Value&& response){
                            const intmax_t views = response["views"];
                            const intmax_t followers = response["followers"];
                            printf(
                                "Channel '%s' has %" PRIdMAX " followers and %" PRIdMAX " views.\n",
                                environment.args[0].c_str(),
                                followers,
                                views
                            );
                            done->set_value();
                        },
                        [&](unsigned int statusCode){
                            done->set_value();
                        }
                    );
                } else {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                        "Twitch API returned invalid ID"
                    );
                    done->set_value();
                }
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
            command.cmdSummary = "Query channel and user information";
            command.cmdDetails = (
                "Look up general information about a Twitch channel."
            );
            command.argSummary = "<CHANNEL>";
            command.argDetails = {
                {"CHANNEL", "Name of the channel for which to return information"},
            };
            command.execute = Info;
            Commands::Add("info", std::move(command));
        }
    } registerInfo;

}
