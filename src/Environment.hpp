#pragma once

/**
 * @file Environment.hpp
 *
 * This module declares the Twarlock::Environment structure.
 *
 * © 2019 by Richard Walters
 */

#include <Json/Value.hpp>
#include <string>
#include <vector>

namespace Twarlock {

    /**
     * This contains variables set through the operating system environment
     * or the command-line arguments.
     */
    struct Environment {
        /**
         * This is the name of the command to execute.
         */
        std::string command;

        /**
         * This holds any additional arguments provided with the command.
         */
        std::vector< std::string > args;

        /**
         * This is the path to configuration file to consult for any
         * missing command arguments or environment variables.
         */
        std::string configurationFilePath;

        /**
         * This holds configuration items which direct or modify
         * the behavior of the program.
         */
        Json::Value configuration;

        /**
         * This indicates the general set of operations the program
         * should perform.
         */
        enum class Mode {
            Unknown,
            Execute,
            CommandHelp,
            OverallHelp
        } mode = Mode::Unknown;
    };

}
