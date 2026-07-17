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

#include <standardFiles/functions.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
#else
#include <inttypes.h>
#include <signal.h>
#include <spawn.h>
#include <unistd.h>
#ifdef __unix__
#include <errno.h>
#include <linux/limits.h>
#include <wait.h>
#else
#include <sys/wait.h>
#endif
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#endif
#include <unitConversion/unitConversion.h>

#define MAX_FILE_NAME_LEN 256
#define BUFSIZE 4096

#ifdef __APPLE__
/**
 * \brief Declare global variable environ
 */
extern char** environ;
#endif // __APPLE__

#ifndef _WIN32
/** \brief Switch that shows if child process ran out of time
 *   \return True if timeout occurred
 */
static bool timeout = false;

/** \brief Switch that shows if child process was successful
 *   \return True if process was successful
 */
static bool child_finished = false;
#endif // none __unix__

/********************************************************************************************************/
#ifdef _WIN32
int handleChildProcess(const std::string& processName, const std::string& relExecDir, double maxRuntime) {
  // s.a.: "http://msdn.microsoft.com/en-us/library/ms682512.aspx"
  /* definition of process variables */
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  /* define execution command for process call */
  std::string tmpCommand = relExecDir + processName;
  char* processNameCh = const_cast<char*>(tmpCommand.c_str());
  /* definition of absolute path for folder from which the process is started */
  TCHAR absExecDir[MAX_PATH]; // absolutes Verzeichnis
  TCHAR currentDir[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, currentDir);
  snprintf(absExecDir, sizeof absExecDir, "%s\\%s", currentDir, relExecDir.c_str());
  /* start child process */
  DWORD exitValue(0);
  if (!CreateProcess(NULL,          // No module name (use command line)
                     processNameCh, // Command line
                     NULL,          // Process handle not inheritable
                     NULL,          // Thread handle not inheritable
                     FALSE,         // Set handle inheritance to FALSE
                     0,             // No creation flags
                     NULL,          // Use parent's environment block
                     absExecDir,    // Use parent's starting directory
                     &si,           // Pointer to STARTUPINFO structure
                     &pi)) {        // Pointer to PROCESS_INFORMATION structure
    myRuntimeInfo->err << "CreateProcess failed (ErrorCode: " << GetLastError() << ") on process: " << processName << std::endl;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 2;
  }
  /* check status of child process */
  DWORD processStatus(WaitForSingleObject(pi.hProcess, maxRuntime * 60000.)); // timeout in ms
  if (processStatus) {
    if (processStatus == 0x00000102L) {
      myRuntimeInfo->err << processName << " mit Zeitueberschreitung abgebrochen!" << std::endl;
      TerminateProcess(pi.hProcess, processStatus);
      exitValue = 1;
    } else if (processStatus == 0xFFFFFFFF) {
      myRuntimeInfo->err << "WaitForSingleObject failed (ErrorCode: " << GetLastError() << ") on process: " << processName << std::endl;
      exitValue = 1; // return;
    } else {
      myRuntimeInfo->err << processName << " mit unbekanntem Fehler abgebrochen!" << std::endl;
      exitValue = 1;
    }
  } else {
    GetExitCodeProcess(pi.hProcess, &exitValue);
    //        if (exitValue)
    //        {
    //            myRuntimeInfo->err << processName << " mit Fehlercode "<<exitValue<<" abgebrochen!" << std::endl;
    //            exit(1);
    //        }
  }
  //    if (mySettings.comments_on)
  //    {
  //        cout << processName << " erfolgreich ausgefuehrt!";
  //    }
  /* close process and thread handles */
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return static_cast<int>(exitValue);
}
#else
void handle_timeout(int sig) {
  timeout = true;
}

void handle_child(int sig) {
  pid_t pid = wait(NULL);
  if (pid == -1) {
    child_finished = false;
    perror("wait");
  } else {
    child_finished = true;
  }
}

int handleChildProcess(const std::string& processName, const std::string& relExecDir, double maxRuntime) {
  timeout = false;        // Reset timeout switch
  child_finished = false; // Reset child process switch

  pid_t processID;
  int status;
  maxRuntime *= 60; // Elapsed time measured in seconds

  // Defining relative command for process execution
  std::string tmpCommand = relExecDir + processName;
  char* processNameCh = const_cast<char*>(tmpCommand.c_str());
  char* argv[] = {const_cast<char*>("sh"), const_cast<char*>("-c"), processNameCh, NULL};

  // Starting child process
  status = posix_spawn(&processID, "/bin/sh", NULL, NULL, argv, environ);

  signal(SIGALRM, handle_timeout);
  signal(SIGCHLD, handle_child);

  // Checking status of child process
  if (status == 0) {
    alarm(maxRuntime);
    pause();

    if (timeout) {
      myRuntimeInfo->err << "Process exited with timeout" << std::endl;
      killpg(processID, 0);
      status = 1;
    } else if (child_finished) {
      // myRuntimeInfo->out << "Process exited successfully" << std::endl;
      status = 0;
    } else {
      myRuntimeInfo->err << "Process exited with unknown error" << std::endl;
      status = 1;
    }
  } else {
    myRuntimeInfo->err << "Process exited with error: " << status << std::endl;
    return status;
  }
  return status;
}
#endif // none WIN32
/********************************************************************************************************/
#ifdef _WIN32
int handleChildProcessOtherDirectory(const std::string& processName, const std::string& relExecDir, // cppcheck-suppress unusedFunction
                                     const std::string& workingDir, double maxRuntime) {
  // s.a.: "http://msdn.microsoft.com/en-us/library/ms682512.aspx"
  /* definition of process variables */
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  /* define execution command for process call */
  std::string tmpCommand = relExecDir + processName;
  char* processNameCh = const_cast<char*>(tmpCommand.c_str());
  /* definition of absolute path for folder from which the process is started */
  TCHAR absExecDir[MAX_PATH];
  absExecDir[workingDir.size()] = 0;
  std::copy(workingDir.begin(), workingDir.end(), absExecDir);
  /* start child process */
  DWORD exitValue(0);
  if (!CreateProcess(NULL,          // No module name (use command line)
                     processNameCh, // argv[1], // Command line
                     NULL,          // Process handle not inheritable
                     NULL,          // Thread handle not inheritable
                     TRUE,          // Set handle inheritance to FALSE
                     0,             // No creation flags
                     NULL,          // Use parent's environment block
                     absExecDir,    // Use parent's starting directory
                     &si,           // Pointer to STARTUPINFO structure
                     &pi)) {        // Pointer to PROCESS_INFORMATION structure
    myRuntimeInfo->err << "CreateProcess failed (ErrorCode: " << GetLastError() << ") on process: " << processName << std::endl;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 2;
  }
  /* check status of child process */
  DWORD processStatus(WaitForSingleObject(pi.hProcess, maxRuntime * 60000.)); // timeout in ms
  if (processStatus) {
    if (processStatus == 0x00000102L) {
      myRuntimeInfo->err << processName << " mit Zeitueberschreitung abgebrochen!" << std::endl;
      TerminateProcess(pi.hProcess, processStatus);
      exitValue = 1;
    } else if (processStatus == 0xFFFFFFFF) {
      myRuntimeInfo->err << "WaitForSingleObject failed (ErrorCode: " << GetLastError() << ") on process: " << processName << std::endl;
      exitValue = 1; // return;
    } else {
      myRuntimeInfo->err << processName << " mit unbekanntem Fehler abgebrochen!" << std::endl;
      exitValue = 1;
    }
  } else {
    GetExitCodeProcess(pi.hProcess, &exitValue);
    if (exitValue) {
      myRuntimeInfo->err << processName << " mit Fehlercode " << exitValue << " abgebrochen!" << std::endl;
      // exit(1);
    }
  }
  //    if (mySettings.comments_on)
  //    {
  //        cout << processName << " erfolgreich ausgefuehrt!";
  //    }
  /* close process and thread handles */
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return static_cast<int>(exitValue);
}
#else
int handleChildProcessOtherDirectory(const std::string& processName, const std::string& relExecDir, const std::string& workingDir, double maxRuntime) {
  timeout = false;        // Reset timeout switch
  child_finished = false; // Reset child process switch
  pid_t processID;
  int status;
  maxRuntime *= 60; // Elapsed time measured in seconds

  // Defining relative command for process execution
  std::string tmpCommand = "cd " + workingDir + "/\n" + relExecDir + processName;
  char* processNameCh = const_cast<char*>(tmpCommand.c_str());
  char* argv2[] = {const_cast<char*>("sh"), const_cast<char*>("-c"), processNameCh, NULL};

  // Starting child process
  status = posix_spawn(&processID, "/bin/sh", NULL, NULL, argv2, environ);

  signal(SIGALRM, handle_timeout);
  signal(SIGCHLD, handle_child);

  // Checking status of child process
  if (status == 0) {
    alarm(maxRuntime);
    pause();

    if (timeout) {
      myRuntimeInfo->err << "Process exited with timeout" << std::endl;
      killpg(processID, 0);
      status = 1;
    } else if (child_finished) {
      // myRuntimeInfo->out << "Process exited successfully" << std::endl;
      status = 0;
    } else {
      myRuntimeInfo->err << "Process exited with unknown error" << std::endl;
      status = 1;
    }
  } else {
    myRuntimeInfo->err << "Error: " << status << std::endl;
    return status;
  }
  alarm(0);

  return status;
}
#endif // none WIN32
/********************************************************************************************************/
void deleteObsoleteFiles(const std::string& filename) {
  std::filesystem::path filepath = std::filesystem::path(filename).lexically_normal();
  if (std::filesystem::remove_all(filepath) < 1) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Error deleting " << filepath.string() << ". Program exit!" << std::endl;
    } else {
      std::cerr << "Error deleting " << filepath.string() << ". Program exit!" << std::endl;
    }
    // ToDo -> should throw instead of exiting;
    exit(1);
  }
}

/* Helper method for wildCardDelete */
std::string wildcard_to_regex(const std::string& wildcard) {
  std::string regex_pattern;
  for (char ch : wildcard) {
    switch (ch) {
      case '*':
        regex_pattern += ".*"; // `*` matches any sequence of characters
        break;
      case '?':
        regex_pattern += "."; // `?` matches any single character
        break;
      case '.':
        regex_pattern += "\\."; // `.` matches a literal dot
        break;
      default:
        regex_pattern += ch; // Any other character matches itself
    }
  }
  return regex_pattern;
}
/********************************************************************************************************/
void wildCardDeleteFiles(const std::string& filesPath, const std::string& filesDir) {
  // Convert inputs to std::filesystem::path
  std::filesystem::path directory = std::filesystem::path(filesDir).lexically_normal();
  std::filesystem::path patternPath = std::filesystem::path(filesPath).lexically_normal();

  // Convert the wildcard pattern to regex
  std::string regex_pattern = wildcard_to_regex(patternPath.string());
  std::regex pattern(regex_pattern, std::regex::icase); // Case-insensitive matching

  try {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), pattern)) {
        std::filesystem::remove(entry.path());
        std::cout << "Deleted: " << entry.path() << std::endl;
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "General exception: " << e.what() << std::endl;
  }
}
/********************************************************************************************************/

std::vector<std::string> listDirFiles(const std::string& filesDir, const std::string& fileName) {
  // Normalize the directory path
  std::filesystem::path dirPath = std::filesystem::path(filesDir).lexically_normal();
  std::vector<std::string> listFileNames;

  // Check if we are using a wildcard
  bool wildCard = (fileName == "*");

  // Convert the wildcard pattern to a regex pattern if not a universal wildcard
  std::regex pattern;
  if (!wildCard) {
    pattern = std::regex(wildcard_to_regex(fileName), std::regex::icase);
  }

  try {
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
      // Get the filename as a string
      std::string fileNameInDir = entry.path().filename().string();

      // Skip "." and ".." entries
      if (fileNameInDir == "." || fileNameInDir == "..") continue;

      // Check if file matches the pattern or if wildcard is enabled
      bool matchesPattern = wildCard || std::regex_match(fileNameInDir, pattern);

      if (matchesPattern) {
        // Check if entry is a directory or file and append appropriately
        if (entry.is_directory()) {
          listFileNames.push_back(fileNameInDir + "/");
        } else {
          listFileNames.push_back(fileNameInDir);
        }
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  }

  return listFileNames;
}

void renameFiles(const std::string& oldname, const std::string& newname, const std::string& newfiletype, bool backup) {
  try {
    // Convert input strings to filesystem paths
    std::filesystem::path oldfilename = std::filesystem::path(oldname).lexically_normal();
    std::filesystem::path newfilename = std::filesystem::path(newname).lexically_normal();

    // Append new file extension if specified
    if (!newfiletype.empty()) {
      newfilename.replace_extension(newfiletype);
    }

    // Check if the target new file already exists
    if (std::filesystem::exists(newfilename)) {
      if (backup) {
        // Define backup filename
        std::filesystem::path newfile_backup = newfilename;
        newfile_backup.replace_filename(newname + "_backup");
        if (!newfiletype.empty()) {
          newfile_backup.replace_extension(newfiletype);
        }

        // Delete backup file or directory if it already exists
        if (std::filesystem::exists(newfile_backup)) {
          if (newfile_backup.extension().empty()) {
            std::filesystem::remove_all(newfile_backup); // Delete directory
          } else {
            std::filesystem::remove(newfile_backup); // Delete file
          }
        }

        // Rename newfilename to newfile_backup
        std::filesystem::rename(newfilename, newfile_backup);
      } else {
        // If no backup is needed, delete the existing file or directory
        if (newfilename.extension().empty()) {
          std::filesystem::remove_all(newfilename); // Delete directory
        } else {
          std::filesystem::remove(newfilename); // Delete file
        }
      }
    }

    // Rename the old file to the new file name
    std::filesystem::rename(oldfilename, newfilename);
  } catch (const std::filesystem::filesystem_error& e) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Filesystem error: " << e.what() << std::endl;
    } else {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    exit(1);
  } catch (const std::exception& e) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "General error: " << e.what() << std::endl;
    } else {
      std::cerr << "General error: " << e.what() << std::endl;
    }
    exit(1);
  }
}
/********************************************************************************************************/
bool copyFiles(const std::string& from, const std::string& to, bool overwriteExistingFile) {
  std::filesystem::path fromPath = std::filesystem::path(from).lexically_normal();
  std::filesystem::path toPath = std::filesystem::path(to).lexically_normal();

  try {
    // Check if source exists
    if (!std::filesystem::exists(fromPath)) {
      if (myRuntimeInfo != nullptr) {
        myRuntimeInfo->err << "Cannot copy " << from << ". File or directory not found." << std::endl;
      } else {
        std::cerr << "Cannot copy " << from << ". File or directory not found." << std::endl;
      }
      return false;
    }

    // Determine if the source is a directory or a file
    if (std::filesystem::is_directory(fromPath)) {
      // Ensure the destination directory exists
      if (!std::filesystem::exists(toPath)) {
        std::filesystem::create_directories(toPath);
      }

      // Copy each file or directory within `fromPath` to `toPath` recursively
      for (const auto& entry : std::filesystem::recursive_directory_iterator(fromPath)) {
        const auto& relPath = std::filesystem::relative(entry.path(), fromPath); // Relative path within `fromPath`
        std::filesystem::path destinationPath = toPath / relPath;

        if (std::filesystem::is_directory(entry.status())) {
          // Create subdirectory in destination if it does not exist
          std::filesystem::create_directories(destinationPath);
        } else {
          // Copy file, respecting the overwrite flag
          if (!overwriteExistingFile && std::filesystem::exists(destinationPath)) {
            continue; // Skip file if it exists and overwrite is not allowed
          }
          std::filesystem::copy_file(entry.path(), destinationPath, std::filesystem::copy_options::overwrite_existing);
        }
      }
      return true;
    } else {
      // Source is a file; copy the file directly
      if (!overwriteExistingFile && std::filesystem::exists(toPath)) {
        if (myRuntimeInfo != nullptr) {
          myRuntimeInfo->err << "Cannot copy to " << to << ". File already exists and overwrite is disabled." << std::endl;
        } else {
          std::cerr << "Cannot copy to " << to << ". File already exists and overwrite is disabled." << std::endl;
        }
        return false;
      }
      std::filesystem::copy_file(fromPath, toPath, overwriteExistingFile ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none);
      return true;
    }
  } catch (const std::filesystem::filesystem_error& e) {
    // Handle filesystem errors with specific messages
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Filesystem error: " << e.what() << std::endl;
    } else {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    return false;
  } catch (const std::exception& e) {
    // Handle any other exceptions
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "General error: " << e.what() << std::endl;
    } else {
      std::cerr << "General error: " << e.what() << std::endl;
    }
    return false;
  }
}

bool fileExists(const std::filesystem::path& filename) {
  return std::filesystem::exists(filename.lexically_normal());
}

bool fileExists(const std::string& filename) {
  // Convert to normal path
  std::filesystem::path filepath = std::filesystem::path(filename).lexically_normal();
  // Check existence
  return std::filesystem::exists(filepath);
}
/********************************************************************************************************/
std::string getFullPathString(const std::string& filename) {
  try {
    // Get the canonical (absolute) path
    std::filesystem::path fullPath = std::filesystem::canonical(filename);

    // Check if the path is a directory and append a separator if it is
    if (std::filesystem::is_directory(fullPath)) {
      fullPath += std::filesystem::path::preferred_separator; // Append separator if it's a directory
    }

    return fullPath.string();
  } catch (const std::filesystem::filesystem_error& e) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "getFullPathString failed: " << e.what() << std::endl;
    } else {
      std::cerr << "getFullPathString failed: " << e.what() << std::endl;
    }
    return "";
  }
}

/********************************************************************************************************/
std::vector<std::string> readFile(const std::string& filename, const bool& specialCase) {
  std::ifstream resultFile;
  resultFile.open(filename.c_str(), std::ios::in);
  std::vector<std::string> lineVector;
  if (!resultFile) {
    myRuntimeInfo->err << filename << " could not be opened." << std::endl;
    exit(1);
  } else {
    /* Read file into vector */
    std::string line;
    while (!resultFile.eof()) {
      getline(resultFile, line);
      if (specialCase && line.size() >= 1) {
        line.erase(line.end() - 1);
      }
      lineVector.push_back(line);
    }
    resultFile.close();
  }
  return lineVector;
}
std::vector<std::string> readFile(const std::string& filename, bool specialCase) {
  std::vector<std::string> lineVector;
  std::ifstream resultFile(filename);

  if (!resultFile) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << filename << " could not be opened." << std::endl;
    } else {
      std::cerr << filename << " could not be opened." << std::endl;
    }
    exit(1);
  }

  std::string line;
  while (std::getline(resultFile, line)) {
    // Apply the special case if needed (remove last character of each line)
    if (specialCase && !line.empty()) {
      line.pop_back();
    }
    lineVector.push_back(line);
  }

  return lineVector;
}

/********************************************************************************************************/
size_t findInFile(const std::string& filename, const std::string& searchWord, bool specialCase) {
  std::filesystem::path filePath = std::filesystem::path(filename).lexically_normal();
  std::ifstream file(filePath);

  if (!file) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Could not open file: " << filename << std::endl;
    } else {
      std::cerr << "Could not open file: " << filename << std::endl;
    }
    return 0; // Return 0 to indicate that the file could not be opened or word not found
  }

  std::string line;
  size_t lineNumber = 0;

  while (std::getline(file, line)) {
    ++lineNumber;

    // Apply the special case (remove last character if needed)
    if (specialCase && !line.empty()) {
      line.pop_back();
    }

    // Check if the line contains the search word
    if (line.find(searchWord) != std::string::npos) {
      return lineNumber; // Return the 1-based line number where the word was found
    }
  }

  // If not found and `specialCase` is false, try again with `specialCase = true`
  if (!specialCase) {
    return findInFile(filename, searchWord, true);
  }

  // Return 0 if the word was not found in either pass
  return 0;
}

/********************************************************************************************************/
bool compareStrings(const std::string& str1, const std::string& str2, bool exact) {
  if (exact) {
    // Check for exact match
    return str1 == str2;
  } else {
    // Check if str2 is a substring of str1 (partial match)
    return str1.find(str2) != std::string::npos;
  }
}

/********************************************************************************************************/
std::string replaceAll(std::string word, const std::string& oldCharacter, const std::string& newCharacter) {
  size_t pos = 0;
  while ((pos = word.find(oldCharacter, pos)) != std::string::npos) {
    word.replace(pos, oldCharacter.size(), newCharacter);
    pos += newCharacter.size(); // Move past the newCharacter to avoid infinite loop
  }
  return word;
}

/********************************************************************************************************/
std::string stringForTex(std::string theData) { // cppcheck-suppress unusedFunction
  // Definition der Tex-Sonderzeichen
  std::vector<std::string> specialChars;
  specialChars.push_back("_");
  specialChars.push_back("%");
  specialChars.push_back("&");
  specialChars.push_back("$");
  for (size_t i = 0; i <= (specialChars.size() - 1); i++) {
    theData = replaceAll(theData, specialChars.at(i), "\\" + specialChars.at(i));
  }
  size_t pos = 0;
  while ((pos = theData.find('_', pos)) != std::string::npos) {
    theData.replace(pos, 1, "_{");
    size_t next_pos = theData.find('_', pos + 2);
    if (next_pos == std::string::npos) {
      theData.append("}");
      break;
    }
    theData.insert(next_pos, "}");
    pos = next_pos + 1;
  }
  return theData;
}
/********************************************************************************************************/
std::string win2lin(const std::string& path) {
  return std::filesystem::path(path).make_preferred().string();
}

/********************************************************************************************************/
std::string relativePath(const std::string& path, const std::string& basePath, bool useUpperLevel) {
  std::filesystem::path targetPath = std::filesystem::path(path).lexically_normal();
  std::filesystem::path base = std::filesystem::path(basePath).lexically_normal();

  try {
    // Compute the relative path
    std::filesystem::path relPath = std::filesystem::relative(targetPath, base);

    // Handle the useUpperLevel case if the result needs to start with "../"
    if (useUpperLevel) {
      relPath = std::filesystem::path("..") / relPath;
    }

    return relPath.string();
  } catch (const std::filesystem::filesystem_error& e) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Error computing relative path: " << e.what() << std::endl;
    } else {
      std::cerr << "Error computing relative path: " << e.what() << std::endl;
    }
    return ""; // Return an empty string in case of error
  }
}

/********************************************************************************************************/
bool deleteDirectory(const std::string& dir, bool recycleBin) {
  std::filesystem::path dirPath = std::filesystem::path(dir).lexically_normal();
  recycleBin = recycleBin; // unused parameter
  try {
    // Remove the directory and its contents
    if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
      std::filesystem::remove_all(dirPath);
      return true;
    } else {
      if (myRuntimeInfo != nullptr) {
        myRuntimeInfo->err << "Path does not exist or is not a directory: " << dir << std::endl;
      } else {
        std::cerr << "Path does not exist or is not a directory: " << dir << std::endl;
      }
      return false;
    }
  } catch (const std::filesystem::filesystem_error& e) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Filesystem error: " << e.what() << std::endl;
    } else {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    return false;
  }
}
/********************************************************************************************************/
double Rounding(double number, int digits) { // cppcheck-suppress unusedFunction
  number *= pow(10, digits);
  if (number >= 0) {
    number = floor(number + 0.5);
  } else {
    number = ceil(number - 0.5);
  }
  number /= pow(10, digits);
  return number;
}
/********************************************************************************************************/
double RoundUp(double number, int digits) { // cppcheck-suppress unusedFunction
  number *= pow(10, digits);
  if (number >= 0) {
    number = ceil(number);
  } else {
    number = floor(number);
  }
  number /= pow(10, digits);
  return number;
}
/********************************************************************************************************/
double RoundDown(double number, int digits) { // cppcheck-suppress unusedFunction
  number *= pow(10, digits);
  if (number >= 0) {
    number = floor(number);
  } else {
    number = ceil(number);
  }
  number /= pow(10, digits);
  return number;
}
/********************************************************************************************************/
bool accuracyCheck(double value, double targetValue, double accuracy) { // cppcheck-suppress unusedFunction
  if ((value < (targetValue + fabs(accuracy))) && (value > (targetValue - fabs(accuracy)))) {
    return true;
  }
  return false;
}
/********************************************************************************************************/
double linearInterpolation(const double& pos, const double& posA, const double& valA, const double& posB, const double& valB, const bool& allowExtrapolation) {
  if ((pos < std::min(posA, posB) || pos > std::max(posA, posB)) && !allowExtrapolation) throw "Extrapolation not allowed.";
  if (posA < posB)
    return (valA + (pos - posA) / (posB - posA) * (valB - valA));
  else if (fabs(posA - posB) < ACCURACY_HIGH)
    return valB;
  else
    return (valB + (pos - posB) / (posA - posB) * (valA - valB));
}
/********************************************************************************************************/
double trilinearInterpolation(const std::vector<std::vector<std::vector<double>>>& output, const std::vector<double>& input1, // cppcheck-suppress unusedFunction
                              const std::vector<double>& input2, const std::vector<double>& input3, const double& input1_tmp, const double& input2_tmp, const double& input3_tmp) {
  /*get vector bounds for all input vectors*/
  std::vector<double>::const_iterator upperBound1, lowerBound1;
  getVectorBounds(input1, input1_tmp, &upperBound1, &lowerBound1);

  std::vector<double>::const_iterator upperBound2, lowerBound2;
  getVectorBounds(input2, input2_tmp, &upperBound2, &lowerBound2);

  std::vector<double>::const_iterator upperBound3, lowerBound3;
  getVectorBounds(input3, input3_tmp, &upperBound3, &lowerBound3);

  /*calculate differences between neighboring coordinates*/
  double xdif = (input1_tmp - input1.at(lowerBound1 - input1.begin())) / (input1.at(upperBound1 - input1.begin()) - input1.at(lowerBound1 - input1.begin()));
  double ydif = (input2_tmp - input2.at(lowerBound2 - input2.begin())) / (input2.at(upperBound2 - input2.begin()) - input2.at(lowerBound2 - input2.begin()));
  double zdif = (input3_tmp - input3.at(lowerBound3 - input3.begin())) / (input3.at(upperBound3 - input3.begin()) - input3.at(lowerBound3 - input3.begin()));

  /*get pre interpolation values 3D: indices: 0-->lowerBound, 1-->upperBound*/
  double preinterpolation000 = output[lowerBound3 - input3.begin()][lowerBound2 - input2.begin()][lowerBound1 - input1.begin()];
  double preinterpolation100 = output[lowerBound3 - input3.begin()][lowerBound2 - input2.begin()][upperBound1 - input1.begin()];
  double preinterpolation001 = output[upperBound3 - input3.begin()][lowerBound2 - input2.begin()][lowerBound1 - input1.begin()];
  double preinterpolation101 = output[upperBound3 - input3.begin()][lowerBound2 - input2.begin()][upperBound1 - input1.begin()];
  double preinterpolation010 = output[lowerBound3 - input3.begin()][upperBound2 - input2.begin()][lowerBound1 - input1.begin()];
  double preinterpolation110 = output[lowerBound3 - input3.begin()][upperBound2 - input2.begin()][upperBound1 - input1.begin()];
  double preinterpolation011 = output[upperBound3 - input3.begin()][upperBound2 - input2.begin()][lowerBound1 - input1.begin()];
  double preinterpolation111 = output[upperBound3 - input3.begin()][upperBound2 - input2.begin()][upperBound1 - input1.begin()];

  /*get pre interpolation values 2D*/
  double preinterpolation00 = preinterpolation000 * (1.0 - xdif) + preinterpolation100 * xdif;
  double preinterpolation01 = preinterpolation001 * (1.0 - xdif) + preinterpolation101 * xdif;
  double preinterpolation10 = preinterpolation010 * (1.0 - xdif) + preinterpolation110 * xdif;
  double preinterpolation11 = preinterpolation011 * (1.0 - xdif) + preinterpolation111 * xdif;

  /*get pre interpolation values 1D*/
  double preinterpolation0 = preinterpolation00 * (1 - ydif) + preinterpolation10 * ydif;
  double preinterpolation1 = preinterpolation01 * (1 - ydif) + preinterpolation11 * ydif;

  /*get final interpolation values*/
  return preinterpolation0 * (1 - zdif) + preinterpolation1 * zdif;
}
/********************************************************************************************************/
std::vector<double> calcRegressionCoefficientsUsingQRdecomp(std::vector<double> vecX, std::vector<double> vecY, uint16_t orderOfRegression) {
  std::vector<double> resultVector;

  if (vecX.size() != vecY.size()) {
    throwError<std::invalid_argument>(__FILE__, __func__, __LINE__, "Size of passed x- and y-values does not match.");
  }

  uint16_t numbOfPairs = vecX.size();

  // Convert vecY to an Eigen vector
  Eigen::VectorXd y_values(numbOfPairs);
  for (uint16_t i = 0; i < numbOfPairs; ++i) {
    y_values(i) = vecY.at(i);
  }

  // Create the coefficient matrix using Eigen
  Eigen::MatrixXd coefficientMatrix(numbOfPairs, orderOfRegression + 1);
  for (uint16_t i = 0; i < numbOfPairs; ++i) {
    for (uint16_t j = 0; j <= orderOfRegression; ++j) {
      coefficientMatrix(i, j) = std::pow(vecX[i], j);
    }
  }

  // Perform QR decomposition using Eigen
  Eigen::HouseholderQR<Eigen::MatrixXd> decomposition(coefficientMatrix);
  Eigen::VectorXd result = decomposition.solve(y_values);

  if (result.size() == orderOfRegression + 1) {
    // Copy results to resultVector
    resultVector.reserve(orderOfRegression + 1);
    for (uint16_t j = 0; j <= orderOfRegression; ++j) {
      resultVector.push_back(result(j));
    }
  } else {
    throwError<std::runtime_error>(__FILE__, __func__, __LINE__, "Equation system has no results.");
  }

  return resultVector;
}
/********************************************************************************************************/
std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters) {
  std::vector<std::string> tokens;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos = str.find_first_of(delimiters, lastPos);
  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
  return tokens;
}

/********************************************************************************************************/
void writeCircleCoordinates(const std::string& aFilename, const uint16_t& numberOfCirclePoints) { // cppcheck-suppress unusedFunction
  std::ofstream out;
  out.open(aFilename.c_str());
  if (!out) {
    myRuntimeInfo->err << "could not open " << aFilename << std::endl;
    exit(1);
  }
  out << "# " << aFilename << std::endl;
  out << "#Circular data" << std::endl << std::endl;
  double angleInc = 360.0 / static_cast<double>(numberOfCirclePoints);
  out.precision(5);
  out.setf(std::ios::fixed, std::ios::floatfield);
  for (uint16_t tmpPoint = 0; tmpPoint <= numberOfCirclePoints; ++tmpPoint) {
    double angle = convertUnit(DEGREE, RADIAN, static_cast<double>(tmpPoint) * angleInc);
    out << 0.5 * cos(angle) << "\t" << 0.5 * sin(angle) << std::endl;
  }
  out.close();
}
/********************************************************************************************************/
std::vector<Point> alignLeftAndRightVectorPoints(const std::vector<Point>& theFixedVector, const std::vector<Point>& theVectorToAlign) { // cppcheck-suppress unusedFunction
  std::vector<Point> theAlignedVector;
  // Interpolate Points
  for (const Point& aPoint : theFixedVector) {
    double targetXvalue(aPoint.xCoordinate);
    size_t idLeft, idRight;
    Point theInterpolatedPoint;
    size_t pos_x_lower = static_cast<size_t>(
        std::find_if(theVectorToAlign.begin(), theVectorToAlign.end(), [targetXvalue](const Point& currentPoint) { return (currentPoint.xCoordinate >= targetXvalue); }) -
        theVectorToAlign.begin());
    if (pos_x_lower == 0) {
      idLeft = pos_x_lower;
      idRight = pos_x_lower + 1;
    } else if (pos_x_lower == theVectorToAlign.size()) {
      idLeft = pos_x_lower;
      idRight = idLeft;
    } else {
      idLeft = pos_x_lower - 1;
      idRight = pos_x_lower;
    }
    theInterpolatedPoint.xCoordinate = aPoint.xCoordinate;
    theInterpolatedPoint.yCoordinate = linearInterpolation(aPoint.xCoordinate, theVectorToAlign.at(idLeft).xCoordinate, theVectorToAlign.at(idLeft).yCoordinate,
                                                           theVectorToAlign.at(idRight).xCoordinate, theVectorToAlign.at(idRight).yCoordinate);
    theInterpolatedPoint.zCoordinate = linearInterpolation(aPoint.xCoordinate, theVectorToAlign.at(idLeft).xCoordinate, theVectorToAlign.at(idLeft).zCoordinate,
                                                           theVectorToAlign.at(idRight).xCoordinate, theVectorToAlign.at(idRight).zCoordinate);
    theAlignedVector.push_back(theInterpolatedPoint);
  }
  return theAlignedVector;
}
/********************************************************************************************************/
std::uintmax_t getDirectorySize(const std::string& path) {
  std::filesystem::path dirPath = std::filesystem::path(path).lexically_normal();
  std::uintmax_t folderSize = 0;

  try {
    if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
      // Recursively iterate through the directory and accumulate file sizes
      for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
          folderSize += entry.file_size();
        }
      }
    } else {
      if (myRuntimeInfo != nullptr) {
        myRuntimeInfo->err << "Path does not exist or is not a directory: " << path << std::endl;
      } else {
        std::cerr << "Path does not exist or is not a directory: " << path << std::endl;
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Filesystem error: " << e.what() << std::endl;
    } else {
      std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
  }

  return folderSize;
}
/********************************************************************************************************/
std::string size2Str(std::int64_t size) {
  // Define size units similar to Windows Explorer with binary size calculation
  constexpr std::array<const char*, 9> UNITS = {"Bytes", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
  double temp = static_cast<double>(size);
  int unitIndex = 0;

  // Convert size to the largest appropriate unit
  while (temp >= 1024.0 && unitIndex < UNITS.size() - 1) {
    temp /= 1024.0;
    ++unitIndex;
  }

  // Format the result with 2 decimal places
  std::ostringstream result;
  result << std::fixed << std::setprecision(2) << temp << " " << UNITS[unitIndex];
  return result.str();
}

/********************************************************************************************************/
void createFolder(const std::string& directoryPath) {
  std::filesystem::path dirPath = std::filesystem::path(directoryPath).lexically_normal();

  try {
    // Attempt to create the directory (and any necessary parent directories)
    if (!std::filesystem::exists(dirPath)) {
      std::filesystem::create_directories(dirPath);
    } else {
      if (myRuntimeInfo != nullptr) {
        myRuntimeInfo->err << "Directory already exists: " << directoryPath << std::endl;
      } else {
        std::cerr << "Directory already exists: " << directoryPath << std::endl;
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->err << "Error creating directory: " << e.what() << std::endl;
    } else {
      std::cerr << "Error creating directory: " << e.what() << std::endl;
    }
    throwError(__FILE__, __func__, __LINE__, "Directory could not be created. Abort program!");
  }
}
