/**
 * @file Following.cpp
 *
 * This module defines the Twarlock::Following command.
 *
 * © 2020 by Richard Walters
 */

#include "Commands.hpp"
#include "Environment.hpp"

#include <future>
#include <inttypes.h>
#include <string>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <unordered_map>
#include <unordered_set>

using namespace Twarlock;

namespace {

    bool Following(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        Twitch& twitch,
        const bool& shutDown
    ) {
        if (environment.args.size() < 2) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "at least two user names expected"
            );
            return false;
        }
        if (environment.args.size() > 100) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "too many user names provided (100 maximum)"
            );
            return false;
        }
        std::unordered_set< std::string > namesOfUserIdsNeeded;
        std::string uri = "users";
        for (const auto& arg: environment.args) {
            if (namesOfUserIdsNeeded.empty()) {
                uri += '?';
            } else {
                uri += '&';
            }
            uri += "login=";
            uri += arg;
            (void)namesOfUserIdsNeeded.insert(arg);
        }
        std::unordered_map< std::string, intmax_t > userIdsByLogin;
        auto done = std::make_shared< std::promise< void > >();
        twitch.PostApiCall(
            Twitch::Api::Helix,
            uri,
            [&](Json::Value&& response){
                const auto& data = response["data"];
                for (size_t i = 0; i < data.GetSize(); ++i) {
                    intmax_t userid;
                    if (
                        sscanf(
                            ((std::string)data[i]["id"]).c_str(), "%" SCNdMAX,
                            &userid
                        ) == 1
                    ) {
                        const std::string login = data[i]["login"];
                        userIdsByLogin[login] = userid;
                        (void)namesOfUserIdsNeeded.erase(login);
                    }
                }
                done->set_value();
            },
            [&](unsigned int statusCode){
                done->set_value();
            }
        );
        done->get_future().get();
        for (const auto& name: namesOfUserIdsNeeded) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                "Could not get ID of user '%s'",
                name.c_str()
            );
        }
        if (userIdsByLogin.size() < 2) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "at least two user IDs needed to compare followers"
            );
            return false;
        }
        printf("--------------------------------------------------\n");
        for (const auto& userIdsByLoginEntry: userIdsByLogin) {
            const auto& toName = userIdsByLoginEntry.first;
            const auto toUserId = userIdsByLoginEntry.second;
            for (const auto& userIdsByLoginEntry: userIdsByLogin) {
                const auto& fromName = userIdsByLoginEntry.first;
                const auto fromUserId = userIdsByLoginEntry.second;
                if (toUserId == fromUserId) {
                    continue;
                }
                const auto done = std::make_shared< std::promise< void > >();
                const auto uri = StringExtensions::sprintf(
                    "users/follows?to_id=%" PRIdMAX "&from_id=%" PRIdMAX,
                    toUserId, fromUserId
                );
                twitch.PostApiCall(
                    Twitch::Api::Helix,
                    uri,
                    [&](Json::Value&& response){
                        for (auto dataEntry: response["data"]) {
                            const auto& follower = dataEntry.value();
                            printf(
                                "%s followed %s at %s\n",
                                ((std::string)follower["from_name"]).c_str(),
                                ((std::string)follower["to_name"]).c_str(),
                                ((std::string)follower["followed_at"]).c_str()
                            );
                        }
                        done->set_value();
                    },
                    [&](unsigned int statusCode){
                        done->set_value();
                    }
                );
                done->get_future().get();
            }
        }
        printf("--------------------------------------------------\n");
        return true;
    };

    struct RegisterInfo {
        RegisterInfo() {
            Command command;
            command.cmdSummary = "Check if users are following each other";
            command.cmdDetails = (
                "For a given set of users, check which ones are following"
                " the others."
            );
            command.argSummary = "<USER>...";
            command.argDetails = {
                {"USER", "Name of one user to query (list at least two)"},
            };
            command.execute = Following;
            Commands::Add("following", std::move(command));
        }
    } registerInfo;

}
