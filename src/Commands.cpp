/**
 * @file Commands.cpp
 *
 * This module contains the implementation of the Twarlock::Commands structure.
 *
 * © 2019 by Richard Walters
 */

#include "Commands.hpp"

namespace {

    struct CommandToAdd {
        std::string name;
        Twarlock::Command command;
        CommandToAdd* next;

        CommandToAdd(
            std::string&& name,
            Twarlock::Command&& command,
            CommandToAdd* next
        )
            : name(name)
            , command(command)
            , next(next)
        {
        }
    };

    CommandToAdd* nextCommand = nullptr;

}

namespace Twarlock {

    void Commands::Add(std::string&& name, Command&& command) {
        nextCommand = new CommandToAdd(
            std::move(name),
            std::move(command),
            nextCommand
        );
    }

    auto Commands::Build() -> Table {
        Table table;
        while (nextCommand != nullptr) {
            auto* command = nextCommand;
            nextCommand = command->next;
            (void)table.insert({
                std::move(command->name),
                std::move(command->command)
            });
            delete command;
        }
        return table;
    }

}
