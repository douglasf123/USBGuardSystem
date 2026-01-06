#include "Logger.h"
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>

static std::mutex g_logMutex;

static std::string GetLogFilePath()
{
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0)
        return "USBGuard.Agent.log";

    std::string p(path);
    auto pos = p.find_last_of("\\/");
    if (pos == std::string::npos) return "USBGuard.Agent.log";
    return p.substr(0, pos + 1) + "USBGuard.Agent.log";
}

void LogUsbEvent(char driveLetter, const std::string& serial, bool allowed, const std::string& note)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(4) << st.wYear << "-"
       << std::setw(2) << st.wMonth << "-" << std::setw(2) << st.wDay << " "
       << std::setw(2) << st.wHour << ":" << std::setw(2) << st.wMinute << ":" << std::setw(2) << st.wSecond;

    ss << " " << driveLetter << ": " << serial << " " << (allowed ? "Liberado" : "Bloqueado");
    if (!note.empty()) ss << " [" << note << "]";
    ss << "\n";

    std::string file = GetLogFilePath();
    std::lock_guard<std::mutex> lock(g_logMutex);

    // Try primary location (exe folder)
    std::ofstream ofs(file, std::ios::app);
    if (!ofs.is_open()) {
        // fallback to ProgramData\USBGuard\USBGuard.Agent.log
        char progData[MAX_PATH] = {0};
        size_t len = 0;
        getenv_s(&len, progData, MAX_PATH, "PROGRAMDATA");
        std::string fallbackDir;
        if (len > 0 && progData[0] != '\0') {
            fallbackDir = std::string(progData) + "\\USBGuard";
        } else {
            // fallback to local temp
            char tempPath[MAX_PATH] = {0};
            if (GetTempPathA(MAX_PATH, tempPath) != 0) {
                fallbackDir = std::string(tempPath) + "USBGuard";
            } else {
                fallbackDir = ".";
            }
        }

        // create directory if needed
        CreateDirectoryA(fallbackDir.c_str(), NULL);
        std::string fallbackFile = fallbackDir + "\\USBGuard.Agent.log";
        ofs.open(fallbackFile, std::ios::app);
        if (!ofs.is_open()) {
            // give up but output debug message
            std::string dbg = "USBGuard: failed to open log file '" + file + "' and fallback '" + fallbackFile + "'\n";
            OutputDebugStringA(dbg.c_str());
            return;
        }
    }

    ofs << ss.str();
    ofs.flush();
}

void LogDebug(const std::string& text)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string file = GetLogFilePath();
    std::ofstream ofs(file, std::ios::app);
    if (ofs.is_open()) {
        ofs << text << "\n";
        ofs.flush();
    }
}
