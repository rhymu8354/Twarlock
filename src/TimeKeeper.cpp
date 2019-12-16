/**
 * @file TimeKeeper.cpp
 *
 * This module contains the implementations of the TimeKeeper class.
 *
 * © 2019 by Richard Walters
 */

#include "TimeKeeper.hpp"

#include <O9KClock/Clock.hpp>
#include <time.h>

namespace Twarlock {

    /**
     * This contains the private properties of a TimeKeeper class instance.
     */
    struct TimeKeeper::Impl {
        /**
         * This is used to measure real time.
         */
        O9K::Clock clock;
    };

    TimeKeeper::~TimeKeeper() noexcept = default;

    TimeKeeper::TimeKeeper()
        : impl_(new Impl())
    {
    }

    double TimeKeeper::GetCurrentTime() {
        return impl_->clock.GetTime();
    }

}
