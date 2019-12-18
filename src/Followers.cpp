/**
 * @file Followers.cpp
 *
 * This module defines the Twarlock::Followers command.
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

    bool Followers(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        if (environment.args.empty()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "user name expected"
            );
            return false;
        }
        const auto userid = twitch.GetUserIdByName(environment.args[0]);
        if (userid == 0) {
            return false;
        }
        std::string cursor;
        printf("--------------------------------------------------\n");
        do {
            const auto done = std::make_shared< std::promise< void > >();
            auto uri = StringExtensions::sprintf(
                "users/follows?to_id=%" PRIdMAX "&first=100",
                userid
            );
            if (!cursor.empty()) {
                uri += StringExtensions::sprintf(
                    "&after=%s",
                    cursor.c_str()
                );
            }
            twitch.PostApiCall(
                Twitch::Api::Helix,
                uri,
                [&](Json::Value&& response){
                    const intmax_t total = response["total"];
                    cursor = response["pagination"]["cursor"];
                    for (auto dataEntry: response["data"]) {
                        const auto& follower = dataEntry.value();
                        printf(
                            "%s - %s\n",
                            ((std::string)follower["followed_at"]).c_str(),
                            ((std::string)follower["from_name"]).c_str()
                        );
                    }
                    if (cursor.empty()) {
                        printf("--------------------------------------------------\n");
                        printf(
                            "User '%s' has %" PRIdMAX " total followers.\n",
                            environment.args[0].c_str(),
                            total
                        );
                    }
                    done->set_value();
                },
                [&](unsigned int statusCode){
                    done->set_value();
                }
            );
            done->get_future().get();
        } while (!cursor.empty());
        return true;
    };

    struct RegisterInfo {
        RegisterInfo() {
            Command command;
            command.cmdSummary = "Download follower list";
            command.cmdDetails = (
                "Download complete follower list."
            );
            command.argSummary = "<USER>";
            command.argDetails = {
                {"USER", "Name of the user for which to download follower information"},
            };
            command.execute = Followers;
            Commands::Add("followers", std::move(command));
        }
    } registerInfo;

}
