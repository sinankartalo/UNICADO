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

#ifndef RUNTIMEINFO_RUNTIMEINFO_H_
#define RUNTIMEINFO_RUNTIMEINFO_H_

#include <fstream>
#include <ostream>
#include <string>

#ifdef BUILD_RUNTIMEINFO_SHARED
    #ifdef _WIN32
        #define RUNTIMEINFODLLEXPORT __declspec(dllexport)
    #else
        #define RUNTIMEINFODLLEXPORT __attribute__ ((visibility ("default")))
    #endif
#elif defined(IMPORT_RUNTIMEINFO_SHARED)
    #ifdef _WIN32
        #define RUNTIMEINFODLLEXPORT __declspec(dllimport)
    #else
        #define RUNTIMEINFODLLEXPORT
    #endif
#else
    #define RUNTIMEINFODLLEXPORT
#endif

class node;

class RUNTIMEINFODLLEXPORT outStream {
 public:
    outStream(std::ostream& os1, std::ostream& os2, unsigned int comments_on, unsigned int logfile_on, bool cleanFirstLine = true);
    virtual ~outStream();

    template<class T>
    outStream& operator<<(const T& x) {
        if (newLine) {
            if (comments_on) os1 << getTimeString();
            if (logfile_on) os2 << getTimeString();
            newLine = false;
        }
        if (comments_on) os1 << x;
        if (logfile_on) os2 << x;
        return *this;
    }

    typedef outStream& (*outStreamManipulator)(outStream&);  // function that takes a custom stream, and returns it
    outStream& operator<<(const outStreamManipulator& manip) {  // take in a function with the custom signature
        return manip(*this);    // call the function, and return it's value
    }
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;  // this is the type of std::cout
    typedef CoutType& (*StandardEndLine)(CoutType&);    // this is the function signature of std::endl
    outStream& operator<<(const StandardEndLine manip) {   // define an operator<< to take in std::endl
        // call the function, but we cannot return it's value
        if (comments_on) manip(os1);
        if (logfile_on) manip(os2);
        newLine = true;
        // *this << getTimeString();
        return *this;
    }
    std::string getTimeString();

 protected:
    std::ostream& os1;
    std::ostream& os2;
    unsigned int comments_on;
    unsigned int logfile_on;
    bool newLine;
};

class RUNTIMEINFODLLEXPORT errStream : public outStream {
 public:
    errStream(std::ostream& os1, std::ostream& os2, unsigned int comments_on, unsigned int logfile_on, bool cleanFirstLine = true)
        : outStream(os1, os2, comments_on, logfile_on, cleanFirstLine) {}
    template<class T>
    outStream& operator<<(const T& x) {
        if (newLine) {
            if (comments_on) os1 << getTimeString() << "ERROR: ";
            if (logfile_on) os2 << getTimeString() << "ERROR: ";
            newLine = false;
        }
        if (comments_on) os1 << x;
        if (logfile_on) os2 << x;
        return *this;
    }
};

class RUNTIMEINFODLLEXPORT warnStream : public outStream {
 public:
    warnStream(std::ostream& os1, std::ostream& os2, unsigned int comments_on, unsigned int logfile_on, bool cleanFirstLine = true)
        : outStream(os1, os2, comments_on, logfile_on, cleanFirstLine) {}
    template<class T>
    outStream& operator<<(const T& x) {
        if (newLine) {
            if (comments_on) os1 << getTimeString() << "WARNING: ";
            if (logfile_on) os2 << getTimeString() << "WARNING: ";
            newLine = false;
        }
        if (comments_on) os1 << x;
        if (logfile_on) os2 << x;
        return *this;
    }
};

class RUNTIMEINFODLLEXPORT infoStream : public outStream {
 public:
    infoStream(std::ostream& os1, std::ostream& os2, unsigned int  comments_on, unsigned int  logfile_on, bool cleanFirstLine = true)
        : outStream(os1, os2, comments_on, logfile_on, cleanFirstLine) {}
    template<class T>
    infoStream& operator<<(const T& x) {
        if (newLine) {
            if (comments_on >= 2) os1 << getTimeString() << "INFO: ";
            if (logfile_on >= 2) os2 << getTimeString() << "INFO: ";
            newLine = false;
        }
        if (comments_on >= 2) os1 << x;
        if (logfile_on >= 2) os2 << x;
        return *this;
    }

    typedef infoStream& (*infoStreamManipulator)(infoStream&);  // function that takes a custom stream, and returns it
    infoStream& operator<<(const infoStreamManipulator& manip) {  // take in a function with the custom signature
        if (comments_on >= 2) return manip(*this);  // call the function, and return it's value
        else
            return(*this);
    }

    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;  // this is the type of std::cout
    typedef CoutType& (*StandardEndLine)(CoutType&);    // this is the function signature of std::endl
    infoStream& operator<<(const StandardEndLine manip) {  // define an operator<< to take in std::endl
        // call the function, but we cannot return it's value
        if (comments_on >= 2) manip(os1);
        if (logfile_on >= 2) manip(os2);
        newLine = true;
        return *this;
    }
};

class RUNTIMEINFODLLEXPORT debugStream : public outStream {
 public:
    debugStream(std::ostream& os1, std::ostream& os2, unsigned int comments_on, unsigned int logfile_on, bool cleanFirstLine = true)
        : outStream(os1, os2, comments_on, logfile_on, cleanFirstLine) {}
    template<class T>
    debugStream& operator<<(const T& x) {
        if (newLine) {
            if (comments_on >= 3) os1 << getTimeString() << "DEBUG: ";
            if (logfile_on >= 3) os2 << getTimeString() << "DEBUG: ";
            newLine = false;
        }
        if (comments_on >= 3) os1 << x;
        if (logfile_on >= 3) os2 << x;
        return *this;
    }

    typedef debugStream& (*infoStreamManipulator)(debugStream&);  // function that takes a custom stream, and returns it
    debugStream& operator<<(const infoStreamManipulator& manip) {  // take in a function with the custom signature
        if (comments_on >= 3) return manip(*this);  // call the function, and return it's value
        else
            return(*this);
    }

    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;  // this is the type of std::cout
    typedef CoutType& (*StandardEndLine)(CoutType&);  // this is the function signature of std::endl
    debugStream& operator<<(const StandardEndLine manip) {  // define an operator<< to take in std::endl
        // call the function, but we cannot return it's value
        if (comments_on >= 3) manip(os1);
        if (logfile_on >= 3) manip(os2);
        newLine = true;
        return *this;
    }
};

class RUNTIMEINFODLLEXPORT runtimeInfo {
 protected:
    unsigned int comments_on;
    unsigned int logfile_on;
    std::string logFilename;
    std::string _programName;
 public:
    std::ofstream logFile;

    runtimeInfo(const unsigned int consoleOn, const unsigned int logOn,
                const std::string& logfilename, const std::string& programname);
    virtual ~runtimeInfo();

    outStream out;
    errStream err;
    warnStream warn;
    infoStream info;
    debugStream debug;
};

extern RUNTIMEINFODLLEXPORT runtimeInfo *myRuntimeInfo;

#endif  // RUNTIMEINFO_RUNTIMEINFO_H_
