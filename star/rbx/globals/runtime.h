#pragma once

#include "globals.h"

#include <algorithm>
#include <chrono>
#include <stop_token>
#include <thread>

namespace Globals::Runtime
{
    inline bool ShouldStop(const std::stop_token& stopToken)
    {
        return stopToken.stop_requested() || StopRequested.load(std::memory_order_relaxed);
    }

    inline bool Sleep(const std::stop_token& stopToken, std::chrono::milliseconds duration)
    {
        using namespace std::chrono;
        const auto deadline = steady_clock::now() + duration;
        while (!ShouldStop(stopToken))
        {
            const auto remaining = duration_cast<milliseconds>(deadline - steady_clock::now());
            if (remaining <= milliseconds::zero())
                return true;
            std::this_thread::sleep_for((std::min)(remaining, milliseconds(25)));
        }
        return false;
    }
}
