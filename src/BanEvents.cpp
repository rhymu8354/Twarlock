/**
 * @file BanEvents.cpp
 *
 * This module defines the Twarlock::BanEvents command.
 *
 * © 2019 by Richard Walters
 */

#include "Commands.hpp"
#include "Environment.hpp"

#include <future>
#include <inttypes.h>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <unordered_set>

using namespace Twarlock;

namespace {

    bool BanEvents(
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
        const auto channelName = environment.args[0];
        const auto userid = twitch.GetUserIdByName(channelName);
        if (userid == 0) {
            return false;
        }
        std::string cursor;
        printf("--------------------------------------------------\n");
        size_t totalEvents = 0;
        do {
            const auto done = std::make_shared< std::promise< void > >();
            auto uri = StringExtensions::sprintf(
                "moderation/banned/events?broadcaster_id=%" PRIdMAX "&first=100",
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
                    cursor = response["pagination"]["cursor"];
                    for (auto dataEntry: response["data"]) {
                        const auto& event = dataEntry.value();
                        const auto& eventData = event["event_data"];
                        intmax_t eventUserid = 0;
                        if (
                            sscanf(
                                ((std::string)eventData["user_id"]).c_str(), "%" SCNdMAX,
                                &eventUserid
                            ) == 1
                        ) {
                            ++totalEvents;
                            printf(
                                "%s: %s for %s (%" PRIdMAX ")\n",
                                ((std::string)event["event_timestamp"]).c_str(),
                                ((std::string)event["event_type"]).c_str(),
                                ((std::string)eventData["user_name"]).c_str(),
                                eventUserid
                            );
                        }
                    }
                    done->set_value();
                },
                [&](unsigned int statusCode){
                    done->set_value();
                }
            );
            done->get_future().get();
        } while (!cursor.empty());
        printf("--------------------------------------------------\n");
        printf(
            "Channel '%s' has had %zu total ban/unban events.\n",
            channelName.c_str(),
            totalEvents
        );
        return true;
    };

    struct RegisterInfo {
        RegisterInfo() {
            Command command;
            command.cmdSummary = "List channel ban events";
            command.cmdDetails = (
                "List all channel ban/unban events."
            );
            command.argSummary = "<CHANNEL>";
            command.argDetails = {
                {"CHANNEL", "Name of the channel for which to list ban events"},
            };
            command.execute = BanEvents;
            Commands::Add("ban-events", std::move(command));
        }
    } registerInfo;

}
