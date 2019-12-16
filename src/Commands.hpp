#pragma once

/**
 * @file Commands.hpp
 *
 * This module declares the Twarlock::Commands structure.
 *
 * © 2019 by Richard Walters
 */

#include "Command.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace Twarlock {

    struct Commands {
        using Table = std::unordered_map< std::string, Command >;
        static void Add(std::string&& name, Command&& command);
        static Table Build();
    };

}
