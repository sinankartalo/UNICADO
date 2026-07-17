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

#include "runtimeInfo/runtimeInfo.h"
#include <chrono> //NOLINT [build/c++11] Just because Google doesn't like it, doesn't mean it's bad!
#include <iostream>
#include <iomanip>

runtimeInfo *myRuntimeInfo;

runtimeInfo::runtimeInfo(const unsigned int consoleOn, const unsigned int logOn,
                         const std::string& logfilename, const std::string& programname)
    :
    comments_on(consoleOn),
    logfile_on(logOn),
    logFilename(logfilename),
    _programName(programname),
    out(std::cout, logFile, comments_on, logfile_on),
    err(std::cerr, logFile, comments_on, logfile_on, false),
    warn(std::cout, logFile, comments_on, logfile_on, false),
    info(std::cout, logFile, comments_on, logfile_on, false),
    debug(std::cout, logFile, comments_on, logfile_on, false) {
    logFile.open(logFilename.c_str(), std::ios::app);
    out << "*******************************************************************************" << std::endl;
    out << "Start " << _programName << std::endl;
}

runtimeInfo::~runtimeInfo() {
    out << "Finish " << _programName << "\n";
    logFile.close();
    //dtor
}
outStream::outStream(std::ostream& os1, std::ostream& os2, unsigned int comments_on, unsigned int logfile_on, bool cleanFirstLine)
    :
    os1(os1),
    os2(os2),
    comments_on(comments_on),
    logfile_on(logfile_on),
    newLine(!cleanFirstLine) {
}
outStream::~outStream() {
}
#ifdef __APPLE__
std::string outStream::getTimeString() {
    // Get system time in UTC
    const auto now = std::chrono::system_clock::now();
    const auto now_time_t = std::chrono::system_clock::to_time_t(now);

    // Convert to local time
    std::tm local_tm = *std::localtime(&now_time_t);

    // Format the time string
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%d.%m.%Y %H:%M:%S - ");
    return oss.str();
}
#else
std::string outStream::getTimeString() {
    // Get system time in UTC
    const auto now = std::chrono::system_clock::now();

    // Convert to local time
    const auto local_time = std::chrono::zoned_time{std::chrono::current_zone(), now};

    // Return the time string with format "dd.mm.yyyy hh:mm:ss - "
    return std::format("{0:%d}.{0:%m}.{0:%Y} {0:%H}:{0:%M}:{0:%OS} - ", local_time.get_local_time());
}
#endif // __APPLE__
