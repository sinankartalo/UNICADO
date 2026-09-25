#include "constraint_analysis/ca_plotting.h"


// CSV output
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace constraint_analysis
{
    void constraint_output_writer::write_curve_to_csv(const constraint_curve& curve, const std::string& file_path)
    {
        std::ofstream file(file_path);

        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file for writing: " + file_path);
        }

        file << "x,y\n";

        for (const auto& point : curve.points)
        {
            file << point.x << "," << point.y << "\n";
        }
    }

    void constraint_output_writer::write_all_curves_to_csv(const constraint_output& output, const std::string& folder_path)
    {
        std::filesystem::create_directories(folder_path);

        for (const auto& curve : output.curves)
        {
            const std::string file_path = folder_path + "/" + curve.name + ".csv";
            write_curve_to_csv(curve, file_path);
        }
    }
}
