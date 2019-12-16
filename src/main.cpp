/**
 * @file main.cpp
 *
 * This module holds the main() function, which is the entrypoint
 * to the program.
 *
 * © 2019 by Richard Walters
 */

#include "Commands.hpp"
#include "Environment.hpp"

#include <algorithm>
#include <functional>
#include <Json/Value.hpp>
#include <map>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <SystemAbstractions/DiagnosticsStreamReporter.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <crtdbg.h>
#endif /* _WIN32 */

namespace {

    std::string Wrap(
        const std::string& text,
        const std::string& preface,
        size_t indent
    ) {
        std::ostringstream output;
        bool firstLine = true;
        size_t pos = 0;
        const auto textLength = text.length();
        const auto prefaceLength = preface.length();
        const size_t lineMaxLength = 78;
        while (pos < textLength) {
            output << std::string(indent, ' ');
            auto field = lineMaxLength - prefaceLength - indent;
            if (firstLine) {
                output << preface;
                firstLine = false;
            } else {
                output << std::string(prefaceLength, ' ');
            }
            bool firstWord = true;
            bool endLine = false;
            while (
                !endLine
                && (pos < textLength)
                && (field > 0)
            ) {
                auto delimiter = text.find_first_of(" \n", pos);
                endLine = (
                    (delimiter == std::string::npos)
                    || (text[delimiter] == '\n')
                );
                if (delimiter == std::string::npos) {
                    delimiter = textLength;
                }
                const auto wordLength = delimiter - pos;
                if (
                    firstWord
                    || (wordLength < field)
                ) {
                    if (!firstWord) {
                        output << ' ';
                    }
                    output << text.substr(pos, wordLength);
                    pos += wordLength + 1;
                    if (wordLength < field) {
                        field -= wordLength + 1;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
                firstWord = false;
            }
            output << '\n';
        }
        return output.str();
    }

    std::string Pad(
        const std::string& text,
        size_t field
    ) {
        std::string pad = text;
        const auto length = pad.length();
        if (length < field) {
            pad.insert(pad.end(), field - length, ' ');
        }
        return pad;
    }

    void PrintUsageInformation(
        const std::string& argSummary,
        const std::string& cmdDetails,
        const std::map< std::string, std::string>& argDetails
    ) {
        printf(
            (
                "\n"
                "Usage: Twarlock %s\n"
                "\n"
                "%s"
                "\n"
            ),
            argSummary.c_str(),
            Wrap(cmdDetails, "", 0).c_str()
        );
        size_t longestArgLength = 0;
        for (const auto& argDetailsEntry: argDetails) {
            longestArgLength = std::max(
                longestArgLength,
                argDetailsEntry.first.length()
            );
        }
        for (const auto& argDetailsEntry: argDetails) {
            printf(
                "%s\n",
                Wrap(
                    argDetailsEntry.second,
                    Pad(argDetailsEntry.first, longestArgLength + 2),
                    4
                ).c_str()
            );
        }
    }

    const std::string cfgArgSummary = "[-c <CFG>]";

    const std::string cfgArgDetails = (
        "Path to file containing the program configuration"
        " If not specified, Twarlock searches for a configuration"
        " file named 'Twarlock.json' in the current working directory,"
        " and then 'Twarlock.json' in directory containing the program,"
        " and then '.twarlock' the current user's home directory."
    );

    /**
     * This function prints to the standard error stream information
     * about how to use this program.
     */
    void PrintUsageInformation(const Twarlock::Commands::Table& commands) {
        std::ostringstream cmdSummaries;
        cmdSummaries << "Name of command to execute:\n";
        size_t longestCmdLength = 0;
        for (const auto& commandsEntry: commands) {
            longestCmdLength = std::max(
                longestCmdLength,
                commandsEntry.first.length()
            );
        }
        for (const auto& commandsEntry: commands) {
            cmdSummaries << Pad(commandsEntry.first, longestCmdLength + 2);
            cmdSummaries << commandsEntry.second.cmdSummary;
            cmdSummaries << '\n';
        }
        PrintUsageInformation(
            cfgArgSummary + " <CMD> [ARG]..",
            "Execute the given command.",
            {
                {"CFG", cfgArgDetails},
                {"CMD", cmdSummaries.str()},
            }
        );
        PrintUsageInformation(
            "-h <CMD>",
            "Print usage information about a specific command and exit.",
            {
                {"CMD", "Name of command for which to get more information"},
            }
        );
    }

    /**
     * This flag indicates whether or not the application should shut down.
     */
    bool shutDown = false;

    /**
     * This function is set up to be called when the SIGINT signal is
     * received by the program.  It just sets the "shutDown" flag
     * and relies on the program to be polling the flag to detect
     * when it's been set.
     *
     * @param[in] sig
     *     This is the signal for which this function was called.
     */
    void InterruptHandler(int) {
        shutDown = true;
    }

    /**
     * This function updates the program environment to incorporate
     * any applicable command-line arguments.
     *
     * @param[in] argc
     *     This is the number of command-line arguments given to the program.
     *
     * @param[in] argv
     *     This is the array of command-line arguments given to the program.
     *
     * @param[in,out] environment
     *     This is the environment to update.
     *
     * @param[in] diagnosticsSender
     *     This is the object to use to publish any diagnostic messages.
     *
     * @return
     *     An indication of whether or not the function succeeded is returned.
     */
    bool ProcessCommandLineArguments(
        int argc,
        char* argv[],
        Twarlock::Environment& environment,
        SystemAbstractions::DiagnosticsSender& diagnosticsSender
    ) {
        enum class State {
            FirstArgument,
            ConfigFile,
            Help,
            CommandToExecute,
            CommandArguments,
            ExtraArguments,
        } state = State::FirstArgument;
        int i = 1;
        while (i < argc) {
            const std::string arg(argv[i]);
            switch (state) {
                case State::FirstArgument: {
                    if (arg == "-c") {
                        environment.mode = Twarlock::Environment::Mode::Execute;
                        ++i;
                        state = State::ConfigFile;
                    } else if (arg == "-h") {
                        ++i;
                        state = State::Help;
                    } else {
                        environment.mode = Twarlock::Environment::Mode::Execute;
                        state = State::CommandToExecute;
                    }
                } break;

                case State::ConfigFile: {
                    environment.configurationFilePath = arg;
                    ++i;
                    state = State::CommandToExecute;
                } break;

                case State::Help: {
                    environment.mode = Twarlock::Environment::Mode::CommandHelp;
                    environment.command = arg;
                    ++i;
                    state = State::ExtraArguments;
                } break;

                case State::CommandToExecute: {
                    environment.command = arg;
                    ++i;
                    state = State::CommandArguments;
                } break;

                case State::CommandArguments: {
                    environment.args.push_back(arg);
                    ++i;
                } break;

                case State::ExtraArguments:
                default: {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                        "extra argument '%s' ignored",
                        arg.c_str()
                    );
                    ++i;
                } break;
            }
        }
        switch (state) {
            case State::FirstArgument: {
                environment.mode = Twarlock::Environment::Mode::OverallHelp;
            } break;

            case State::ConfigFile: {
                diagnosticsSender.SendDiagnosticInformationString(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "configuration file path expected"
                );
                return false;
            } break;

            case State::Help: {
                environment.mode = Twarlock::Environment::Mode::OverallHelp;
            } break;

            case State::CommandToExecute: {
                diagnosticsSender.SendDiagnosticInformationString(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "command expected"
                );
                return false;
            } break;

            case State::CommandArguments:
            case State::ExtraArguments:
            default: {
            } break;
        }
        return true;
    }

}

/**
 * This function is the entrypoint of the program.
 *
 * @param[in] argc
 *     This is the number of command-line arguments given to the program.
 *
 * @param[in] argv
 *     This is the array of command-line arguments given to the program.
 */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    //_crtBreakAlloc = 18;
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif /* _WIN32 */
    const auto previousInterruptHandler = signal(SIGINT, InterruptHandler);
    Twarlock::Environment environment;
    (void)setbuf(stdout, NULL);
    const auto diagnosticsPublisher = SystemAbstractions::DiagnosticsStreamReporter(stderr, stderr);
    SystemAbstractions::DiagnosticsSender diagnosticsSender("Twarlock");
    (void)diagnosticsSender.SubscribeToDiagnostics(diagnosticsPublisher);
    const auto commands = Twarlock::Commands::Build();
    if (!ProcessCommandLineArguments(argc, argv, environment, diagnosticsSender)) {
        PrintUsageInformation(commands);
        return EXIT_FAILURE;
    }
    int exitStatus = EXIT_SUCCESS;
    switch (environment.mode) {
        case Twarlock::Environment::Mode::OverallHelp: {
            PrintUsageInformation(commands);
        } break;

        case Twarlock::Environment::Mode::CommandHelp: {
            const auto command = commands.find(environment.command);
            if (command == commands.end()) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "No such command '%s'",
                    environment.command.c_str()
                );
                exitStatus = EXIT_FAILURE;
            } else {
                auto argDetails = command->second.argDetails;
                argDetails["CFG"] = cfgArgDetails;
                PrintUsageInformation(
                    cfgArgSummary + " " + environment.command + " " + command->second.argSummary,
                    command->second.cmdSummary,
                    argDetails
                );
            }
        } break;

        case Twarlock::Environment::Mode::Execute: {
            const auto command = commands.find(environment.command);
            if (command == commands.end()) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "No such command '%s'",
                    environment.command.c_str()
                );
                exitStatus = EXIT_FAILURE;
            } else {
                if (
                    !command->second.execute(
                        environment,
                        diagnosticsSender,
                        shutDown
                    )
                ) {
                    exitStatus = EXIT_FAILURE;
                }
            }
        } break;

        case Twarlock::Environment::Mode::Unknown:
        default: {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "I'm confused!"
            );
            exitStatus = EXIT_FAILURE;
        } break;
    }
    (void)signal(SIGINT, previousInterruptHandler);
    return exitStatus;
}
