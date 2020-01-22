/**
 * @file Bans.cpp
 *
 * This module defines the Twarlock::Bans command.
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

    bool Bans(
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
        std::unordered_set< intmax_t > bannedUserIds;
        std::string cursor;
        printf("--------------------------------------------------\n");
        size_t numNewBannedUserIds;
        do {
            numNewBannedUserIds = 0;
            const auto done = std::make_shared< std::promise< void > >();
            auto uri = StringExtensions::sprintf(
                "moderation/banned?broadcaster_id=%" PRIdMAX "&first=100",
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
                        const auto& banned = dataEntry.value();
                        intmax_t bannedUserid = 0;
                        if (
                            sscanf(
                                ((std::string)banned["user_id"]).c_str(), "%" SCNdMAX,
                                &bannedUserid
                            ) == 1
                        ) {
                            if (bannedUserIds.insert(bannedUserid).second) {
                                ++numNewBannedUserIds;
                                printf(
                                    "%s (%" PRIdMAX ")\n",
                                    ((std::string)banned["user_name"]).c_str(),
                                    bannedUserid
                                );
                            }
                        }
                    }
                    done->set_value();
                },
                [&](unsigned int statusCode){
                    done->set_value();
                }
            );
            done->get_future().get();
        } while (
            !cursor.empty()
            && (numNewBannedUserIds > 0)
        );
        printf("--------------------------------------------------\n");
        printf(
            "Channel '%s' has %zu total Bans.\n",
            channelName.c_str(),
            bannedUserIds.size()
        );
        return true;
    };

    struct RegisterInfo {
        RegisterInfo() {
            Command command;
            command.cmdSummary = "Download banned users list";
            command.cmdDetails = (
                "Download complete banned users list."
            );
            command.argSummary = "<CHANNEL>";
            command.argDetails = {
                {"CHANNEL", "Name of the channel for which to download banned user list"},
            };
            command.execute = Bans;
            Commands::Add("bans", std::move(command));
        }
    } registerInfo;

}
