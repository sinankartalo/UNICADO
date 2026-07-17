/*
 * UNICADO - UNIversity Conceptual Aircraft Design and Optimization
 *
 * Copyright (C) 2025 UNICADO consortium
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 * This file is part of UNICADO.
 */


#ifndef LILIINTERFACE_LILIINTERFACE_H_
#define LILIINTERFACE_LILIINTERFACE_H_

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <atmosphere/atmosphere.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <windows.h>
#include "lifting_line_geometry_interface.h"
#include "aerodynamics/aerodynamics_v2.h"

namespace lifting_line
{
    namespace exe_interface
    {
        inline void exec_lifting_line(const std::string& input_file_name, const std::string& exe_directory, const std::string& exe_filename)
        {
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = NULL;

            HANDLE hRead, hWrite;

            // Create a pipe for the child process's STDIN
            if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
                std::cerr << "Failed to create pipe" << std::endl;
            }

            // Ensure the write handle is inheritable
            SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0);

            PROCESS_INFORMATION pi;
            STARTUPINFO si;
            ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
            ZeroMemory(&si, sizeof(STARTUPINFO));

            si.cb = sizeof(STARTUPINFO);
            si.hStdInput = hRead;  // Redirect stdin
            si.dwFlags |= STARTF_USESTDHANDLES;
            si.dwFlags |= STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;  // Hide the window

            // Start the new command prompt
            if (!CreateProcess(
                NULL,
                (LPSTR)"cmd.exe /K",  // /K keeps the window open
                NULL,
                NULL,
                TRUE,  // Inherit handles
                CREATE_NO_WINDOW,
                // CREATE_NEW_CONSOLE,
                NULL,
                NULL,
                &si,
                &pi)) {
                std::cerr << "Failed to create process" << std::endl;
            }

            // Send commands to the new cmd process
            DWORD bytesWritten;

            std::string command = "cd " + exe_directory + "\n";
            WriteFile(hWrite, command.c_str(), strlen(command.c_str()), &bytesWritten, NULL);
            command = exe_filename + ".exe -om:overwrite -op:" + input_file_name + " -tp:false -xo:true -pj:" + input_file_name + ".inp\n";
            WriteFile(hWrite, command.c_str(), strlen(command.c_str()), &bytesWritten, NULL);

            // Close handles
            CloseHandle(hWrite);
            CloseHandle(hRead);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        };

        inline auto init_input_file(std::string& surface_ID, std::filesystem::path path) -> std::fstream
        {
            std::fstream input_file;
            std::string file_name = path.generic_string() + "/" + surface_ID + ".inp";
            input_file.open(file_name, std::ios::out);
            input_file << "LIFTING_LINE INPUTFILE\n" << "VERSION V3.1\n";
            input_file << "|                                                           NAME_OF_DATASET                                                            |\n";
            input_file << surface_ID + "\n";
            return input_file;
        }

        inline void write_partial_wings(const node& case_node, const std::vector<std::vector<Panel>>& segments, std::fstream& file)
        {
            int count = 1;

            for (int i = 0; i < segments[0].size(); i++)
            {
                for (int j = 0; j < segments.size(); j++)
                {
                    //   INDEX_PW   |                   N_PANELS                   |    PANEL_DISTR   |  FACT_CL_ALPHA   |  FUSELAGE_INF  |     ELLIPT_CHORD                                               |
                    file << count << " " << case_node.at("n_spanwise_panel/value") << " " << int(0) << " " << double(1.) << " " << int(0) << " " << int(0) << "\n";

                    //             X_PW_1             |               Y_PW_1              |                Z_PW_1 
                    file << segments[j][i].inboard_point.x() << " " << segments[j][i].inboard_point.y() << " " << segments[j][i].inboard_point.z() << " ";

                    //             CHORD_PW_1             |        TWIST_PW_1       |        COUPL_COND_1       |   FORKING_NO_1 
                    file << segments[j][i].inboard_chord << " " << segments[j][i].inboard_twist << " " << segments[j][i].inboard_coupling << " " << int(0) << "\n";

                    //             X_PW_2             |               Y_PW_2              |                Z_PW_2 
                    file << segments[j][i].outboard_point.x() << " " << segments[j][i].outboard_point.y() << " " << segments[j][i].outboard_point.z() << " ";

                    //             CHORD_PW_2             |        TWIST_PW_2       |         COUPL_COND_2       |   FORKING_NO_2 
                    file << segments[j][i].outboard_chord << " " << segments[j][i].outboard_twist << " " << segments[j][i].outboard_coupling << " " << int(0) << "\n";

                    count++;
                }
            }
        };

        inline void write_ac_info(const std::shared_ptr<node>& aircraft_xml, const std::shared_ptr<Aircraft>& aircraft_geometry, std::fstream& file)
        {
            auto ac_cog = get_ac_cog(aircraft_xml);

            auto Sref = get_ref_area(aircraft_geometry);
            auto Cref = get_root_chord(aircraft_geometry);
            auto Bref = get_half_span(aircraft_geometry);

            int is_symmetric = 1;

            //      SYMMETRY      |      GEO_SCALING     |     TWIST_DISTR    |    GEO_TWIST   |   QSI_STDY_ROT |  N_PROPELLERS |   N_ADD_LINES   |   XML_DATA
            file << is_symmetric << " " << double(1.000) << " " << int(1) << " " << int(1) << " " << int(0) << " " << int(0) << " " << int(0) << " " << int(1) << "\n";

            //      REF_AREA   |   REF_SPAN    |  REF_LEN_CMX  |   REF_LEN_CMY   |    REF_LEN_CMZ      |     MOM_REF_X      |   MOM_REF_Y     |  MOM_REF_Z 
            file << Sref << " " << Bref << " " << Cref << " " << double(0.) << " " << double(0.) << " " << ac_cog[0] << " " << ac_cog[1] << " " << ac_cog[2] << "\n";
            // todo change this to the origin of the wing
        };

        inline void write_wing_info(const geom2::MultisectionSurface<geom2::AirfoilSection>& sections, std::fstream& file)
        {
            auto Sref = geom2::measure::reference_area(sections);
            auto Cref = geom2::measure::chord(sections, 0.0);
            auto Bref = geom2::measure::span(sections);

            int is_symmetric = 1;

            //      SYMMETRY      |      GEO_SCALING     |     TWIST_DISTR    |    GEO_TWIST   |   QSI_STDY_ROT |  N_PROPELLERS |   N_ADD_LINES   |   XML_DATA
            file << is_symmetric << " " << double(1.000) << " " << int(1) << " " << int(1) << " " << int(0) << " " << int(0) << " " << int(0) << " " << int(1) << "\n";

            //      REF_AREA   |   REF_SPAN    |  REF_LEN_CMX  |   REF_LEN_CMY   |    REF_LEN_CMZ      |     MOM_REF_X      |   MOM_REF_Y     |  MOM_REF_Z 
            file << Sref << " " << Bref << " " << Cref << " " << double(0.) << " " << double(0.) << " " << 0. << " " << 0. << " " << 0. << "\n";
            // todo change this to the origin of the wing
        };

        inline void initialize_wing(const node& case_node, std::fstream& file)
        {
            //   N_WINGS    |--> Equals to 1 as there is a single wing element that is being analyzed
            file << int(1) << "\n";

            //   N_PW_SPAN  |  N_PW_CHORD  |
            file << case_node.at("n_spanwise_section/value") << " " << case_node.at("n_chordwise_panel/value") << "\n";

            if (case_node.at("independent_parameter").getStringAttrib("type") == "alpha")
            {
                //   N_ALPHA    |    ALPHA     |...
                file << case_node.at("independent_parameter/n_inputs/value") << " ";
                std::string indep_params = case_node.at("independent_parameter/inputs/value");
                std::replace(indep_params.begin(), indep_params.end(), ',', ' ');
                file << indep_params << "\n";

                // N_TARGET_CFZ | CFZ | ...
                file << int(0) << "\n";
            }
            else
            {
                //   N_ALPHA    |    ALPHA     |...
                file << int(0) << "\n";

                // N_TARGET_CFZ | CFZ | ...
                file << case_node.at("independent_parameter/n_inputs/value") << " ";
                std::string indep_params = case_node.at("independent_parameter/inputs/value");
                std::replace(indep_params.begin(), indep_params.end(), ',', ' ');
                file << indep_params << "\n";
            }

            //  COMP_MA_NO  |     BETA
            file << case_node.at("M_correction/value") << " " << double(0.000) << "\n";

            // XML_MACH_NO  |  XML_RE_NO   
            file << case_node.at("freestream_M/value") << " " << case_node.at("Re/value") << "\n";

            //  N_AIRFOILS  |
            file << int(1) << "\n" << "FLAT_PLATE\n";

            // N_ETA_COORD  |   ETA_COORD  |...     
            file << int(1) << " " << double(0.000) << "\n";
        }

        inline void create_input_for_lifting_surface(
            const std::shared_ptr<node>& aircraft_xml,
            const std::shared_ptr<Wing>& lifting_surface,
            const std::filesystem::path path,
            const node& case_node)
        {
            auto mesh = create_mesh(case_node, (*lifting_surface));
            std::string file_name = (*lifting_surface).name;
            mesh = set_boundary_conditions(mesh);
            auto file = init_input_file(file_name, path);
            write_wing_info((*lifting_surface), file);
            initialize_wing(case_node, file);
            write_partial_wings(case_node, mesh, file);
            file.flush();
            file.close();
        }
    }; 

    inline auto read_lifting_line_output(const std::shared_ptr<node>& export_xml) -> aerodynamics::Polar
    {
        aerodynamics::Polar polar;

        double Mach = export_xml->at("lifting_line-exchange-file/global_data/flow_parameters/mach_number");

        double RE = export_xml->at("lifting_line-exchange-file/global_data/flow_parameters/reynolds_number");

        polar.conditions = aerodynamics::Flight_Condition(Mach, RE, 1.);

        auto results = export_xml->getVector("lifting_line-exchange-file/result_data/result");
        for (auto result : results)
        {
            std::string cfz = result->at("cfz");
            polar.CL.push_back(std::stod(cfz));
            std::string cfx_from_cdi = result->at("cfx_from_cdi");
            polar.CD.push_back(aerodynamics::Drag(0., 0., std::stod(cfx_from_cdi)));
            std::string cmy = result->at("cmy");
            polar.CM_y.push_back(std::stod(cmy));
            std::string angle_of_attack = result->at("angle_of_attack");
            polar.alpha.push_back(std::stod(angle_of_attack));
        }

        return polar;
    };
};

#endif