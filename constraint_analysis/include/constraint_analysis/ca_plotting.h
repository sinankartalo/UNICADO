// This file merges the declarations that were previously split over multiple small headers.
#pragma once

#include "constraint_analysis/ca_functions.h"

// ============================================================
// merged from: constraint_output_writer.h
// ============================================================
#include <string>

namespace constraint_analysis
{
    class constraint_output_writer
    {
    public:
        static void write_curve_to_csv(const constraint_curve& curve, const std::string& file_path);
        static void write_all_curves_to_csv(const constraint_output& output, const std::string& folder_path);
    };
}
