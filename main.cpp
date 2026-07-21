#include <windows.h>
#include "GlobalConfig.h"
#include <shellapi.h>
#include <string>
#include "HardwareInfo.h"
#include "VolumeManager.h"
#include <unordered_map>
#include <cstdint>
#include "Logger.h"
#include <unordered_set>
#include <cstdio>

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

enum class BlockStage {
    None,
    DismountTried,
    EjectTried,
    LetterRemoved
};

struct BlockState {
    BlockStage stage = BlockStage::None;
    unsigned long long retryAfter = 0;
    char lastDriveLetter = '\0';
};

static const char* BlockStageName(BlockStage stage) {
    switch (stage) {
    case BlockStage::None: return "None";
    case BlockStage::DismountTried: return "DismountTried";
    case BlockStage::EjectTried: return "EjectTried";
    case BlockStage::LetterRemoved: return "LetterRemoved";
    default: return "Unknown";
    }
}

static void LogBlockState(char driveLetter, const std::string& serial, UINT driveType, bool allowed, bool accessible, BlockStage stage) {
    char msg[512];
    sprintf_s(
        msg,
        "Drive %c type=%u usb=1 allowed=%d accessible=%d stage=%s serial=%s",
        driveLetter,
        static_cast<unsigned>(driveType),
        allowed ? 1 : 0,
        accessible ? 1 : 0,
        BlockStageName(stage),
        serial.c_str()
    );
    LogDebug(msg);
}

void MonitorLoop() {
    std::string myID = GetInstalledAppID();
    std::unordered_map<char, bool> lastAllowed;
    std::unordered_map<char, std::string> lastSerial;
    std::unordered_map<std::string, BlockState> blockStates;
    const unsigned long long retryAfterDismountMs = 3000ull;
    const unsigned long long retryAfterEjectMs = 3000ull;
    const unsigned long long retryAfterRemoveLetterMs = 10000ull;

    while (true) {
        std::unordered_set<std::string> presentSerials;

        for (char c = 'D'; c <= 'Z'; c++) {
            char root[] = { c, ':', '\\', '\0' };
            UINT driveType = GetDriveTypeA(root);

            if (driveType != DRIVE_REMOVABLE && driveType != DRIVE_FIXED) {
                lastAllowed.erase(c);
                lastSerial.erase(c);
                continue;
            }

            if (!IsUsbBus(c)) {
                lastAllowed.erase(c);
                lastSerial.erase(c);
                continue;
            }

            std::string hSerial = GetUsbSerialNumber(c);
            if (hSerial.empty() || hSerial == "N/A") {
                continue;
            }

            presentSerials.insert(hSerial);

            bool allowed = IsAuthorized(c, hSerial, myID);

            auto itStatus = lastAllowed.find(c);
            bool serialChanged = false;
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

            if (allowed) {
                auto itState = blockStates.find(hSerial);
                if (itState != blockStates.end()) {
                    char dbg[256];
                    sprintf_s(dbg, "Clearing block state for serial=%s on drive %c (authorized)", hSerial.c_str(), c);
                    LogDebug(dbg);
                    blockStates.erase(itState);
                }
                continue;
            }

            auto& state = blockStates[hSerial];
            state.lastDriveLetter = c;

            unsigned long long now = GetTickCount64();
            bool accessible = IsDriveAccessible(c);
            LogBlockState(c, hSerial, driveType, allowed, accessible, state.stage);

            if (now < state.retryAfter) {
                continue;
            }

            switch (state.stage) {
            case BlockStage::None:
                if (TryDismountVolume(c)) {
                    state.stage = BlockStage::DismountTried;
                }
                state.retryAfter = now + retryAfterDismountMs;
                break;

            case BlockStage::DismountTried:
                if (!accessible) {
                    state.retryAfter = now + retryAfterDismountMs;
                    break;
                }
                if (TryEjectMedia(c)) {
                    state.stage = BlockStage::EjectTried;
                }
                state.retryAfter = now + retryAfterEjectMs;
                break;

            case BlockStage::EjectTried:
                if (!accessible) {
                    state.retryAfter = now + retryAfterEjectMs;
                    break;
                }
                if (TryRemoveDriveLetter(c)) {
                    state.stage = BlockStage::LetterRemoved;
                }
                state.retryAfter = now + retryAfterRemoveLetterMs;
                break;

            case BlockStage::LetterRemoved:
                if (accessible) {
                    TryRemoveDriveLetter(c);
                }
                state.retryAfter = now + retryAfterRemoveLetterMs;
                break;
            }
        }

        for (auto it = blockStates.begin(); it != blockStates.end(); ) {
            if (presentSerials.find(it->first) == presentSerials.end()) {
                it = blockStates.erase(it);
            }
            else {
                ++it;
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