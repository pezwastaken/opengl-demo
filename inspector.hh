#pragma once

#include "utils.hh"

struct Inspector
{
    u32 currently_inspecting_idx;
    bool hidden;

    void reset()
    {
        hidden = true;
        currently_inspecting_idx = U32_MAX;
    }
};

