#include <windows.h>
#include "GlobalConfig.h"
#include <shellapi.h>
#include <string>
#include "HardwareInfo.h"
#include "VolumeManager.h"
#include <unordered_map>
#include <cstdint>
#include "Logger.h"

#pragma pack(push, 1) 
struct USBAuthTicket {
    uint32_t Magic;        // 4 bytes
    char AppID[64];        // 64 bytes
    char Serial[256];      // 256 bytes
    uint8_t Reserved[188]; // 188 bytes (Total: 512 bytes)
};
#pragma pack(pop)

// Funcoes do Security.cpp
std::string GetInstalledAppID();
bool IsAuthorized(char driveLetter, std::string hardwareSerial, std::string installedAppID);

void MonitorLoop() {
    std::string myID = GetInstalledAppID();

    // Track last ejection time per drive to avoid tight ejection loop when device
    // is re-enumerated quickly after being ejected.
    std::unordered_map<char, unsigned long long> lastEjectTime;
    const unsigned long long ejectCooldownMs = 60ull * 1000ull; // 60 seconds cooldown
    std::unordered_map<char, bool> lastAllowed;

    while (true) {
        for (char c = 'D'; c <= 'Z'; c++) {
            char root[] = { c, ':', '\\', '\0' };
            if (GetDriveTypeA(root) == DRIVE_REMOVABLE) {
                // check cooldown
                unsigned long long now = GetTickCount64();
                auto it = lastEjectTime.find(c);
                if (it != lastEjectTime.end()) {
                    if (now < it->second + ejectCooldownMs) {
                        // still in cooldown period, skip
                        continue;
                    }
                }

                std::string hSerial = GetUsbSerialNumber(c);
                if (hSerial.empty() || hSerial == "N/A") continue;

                bool allowed = IsAuthorized(c, hSerial, myID);
                if (!allowed) {
                    // Eject regardless of bus type (allow ejecting external HDDs and USB sticks)
                    DismountUSB(c);
                    // record ejection time to avoid re-eject loop
                    lastEjectTime[c] = GetTickCount64();
                }

                // Log only when authorization status changes or serial changed
                auto itStatus = lastAllowed.find(c);
                bool serialChanged = false;
                static std::unordered_map<char, std::string> lastSerial;
                auto itSerial = lastSerial.find(c);
                if (itSerial == lastSerial.end() || itSerial->second != hSerial) {
                    serialChanged = (itSerial != lastSerial.end());
                    lastSerial[c] = hSerial;
                }

                if (itStatus == lastAllowed.end() || itStatus->second != allowed || serialChanged) {
                    std::string note;
                    if (serialChanged) note = "SerialChanged";
                    LogUsbEvent(c, hSerial, allowed, note);
                    lastAllowed[c] = allowed;
                }
            } else {
                // If drive is not present anymore, clear any cooldown record so
                // a future genuine insertion can be checked immediately.
                lastEjectTime.erase(c);
            }
        }
        Sleep(2000);
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // Cria icone discreto na bandeja
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.hWnd = CreateWindowEx(0, L"STATIC", L"USBGuard", 0, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP;
    nid.hIcon = LoadIcon(NULL, IDI_SHIELD);
    lstrcpy(nid.szTip, L"USBGuard Ativo");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Roda o monitoramento em segundo plano
    MonitorLoop();
    return 0;
}