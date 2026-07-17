// This file merges the declarations that were previously split over multiple small headers.
#pragma once

#include "constraint_analysis/ca_functions.h"
#include "constraint_analysis/ca_minfinder.h"
#include "constraint_analysis/ca_plotting.h"
#include <atmosphere/atmosphere.h>


// ============================================================
// merged from: constraint_analysis_tool.h
// ============================================================
namespace constraint_analysis
{
    class constraint_analysis_tool
    {
    public:
        explicit constraint_analysis_tool(const atmosphere& atmosphere);

        constraint_output run(const constraint_input& input) const;

    private:
        const atmosphere& atmosphere_;
    };
}

