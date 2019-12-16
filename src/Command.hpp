#pragma once

/**
 * @file Command.hpp
 *
 * This module declares the Twarlock::Command structure.
 *
 * © 2019 by Richard Walters
 */

#include "Environment.hpp"

#include <functional>
#include <map>
#include <string>
#include <SystemAbstractions/DiagnosticsSender.hpp>

namespace Twarlock {

    /**
     * This contains information about a command that the Twarlock
     * program can execute.
     */
    struct Command {
        std::string argSummary;
        std::string cmdSummary;
        std::string cmdDetails;
        std::map< std::string, std::string > argDetails;
        std::function<
            bool(
                Twarlock::Environment& environment,
                SystemAbstractions::DiagnosticsSender& diagnosticsSender,
                const bool& shutDown
            )
        > execute;
    };

}
