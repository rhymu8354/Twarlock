/**
 * @file Info.cpp
 *
 * This module defines the Twarlock::Info command.
 *
 * © 2019 by Richard Walters
 */

#include "Commands.hpp"
#include "Environment.hpp"

#include <SystemAbstractions/DiagnosticsSender.hpp>

namespace Twarlock {

    bool Info(
        Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender,
        const bool& shutDown
    ) {
        if (environment.args.empty()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "channel name expected"
            );
            return false;
        }
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
