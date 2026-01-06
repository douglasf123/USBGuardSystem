#include <windows.h>
#include "GlobalConfig.h"
#include <string>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <wincrypt.h>
#include <cstring>

// Estrutura de 512 bytes para garantir compatibilidade com o C#
#pragma pack(push, 1)
struct USBAuthTicket {
    unsigned int Magic;    // 4 bytes (0x53425355)
    char AppID[64];        // 64 bytes
    char Serial[256];      // 256 bytes
    char Reserved[188];    // 188 bytes para fechar 512
};
#pragma pack(pop)

std::string GetInstalledAppID() {
    char value[64] = { 0 };
    DWORD size = sizeof(value);
    HKEY hKey = NULL;
    LONG res = ERROR_FILE_NOT_FOUND;

    // Try 64-bit view first (if available), then fallback to default
#if defined(KEY_WOW64_64KEY)
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\USBGuard", 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
#endif
    if (res != ERROR_SUCCESS) {
        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\USBGuard", 0, KEY_READ, &hKey);
    }

    if (res == ERROR_SUCCESS && hKey != NULL) {
        DWORD type = 0;
        DWORD cb = size;
        if (RegQueryValueExA(hKey, "AppID", NULL, &type, (LPBYTE)value, &cb) == ERROR_SUCCESS) {
            // ensure NUL termination
            if (cb >= sizeof(value)) cb = sizeof(value) - 1;
            value[cb] = '\0';
        }
        RegCloseKey(hKey);
    }

    // Trim whitespace
    std::string s(value);
    auto l = s.find_last_not_of(" \t\r\n");
    if (l == std::string::npos) return std::string();
    auto f = s.find_first_not_of(" \t\r\n");
    return s.substr(f, l - f + 1);
}

bool IsAuthorized(char driveLetter, std::string hardwareSerial, std::string installedAppID) {
    char path[MAX_PATH];
    sprintf_s(path, "%c:\\device.auth", driveLetter);

    FILE* file = NULL;
    errno_t ferr = fopen_s(&file, path, "rb");
    if (ferr != 0 || !file) return false;

    USBAuthTicket auth;
    size_t read = fread(&auth, 1, sizeof(USBAuthTicket), file);
    fclose(file);

    if (read < sizeof(USBAuthTicket)) return false;

    // Extract C strings safely (ensure null-termination)
    std::string fileAppID;
    std::string fileSerial;

    // AppID
    {
        size_t n = 0;
        for (; n < sizeof(auth.AppID) && auth.AppID[n] != '\0'; ++n) ;
        fileAppID.assign(auth.AppID, n);
    }

    // Serial
    {
        size_t n = 0;
        for (; n < sizeof(auth.Serial) && auth.Serial[n] != '\0'; ++n) ;
        fileSerial.assign(auth.Serial, n);
    }

    // Trim both
    auto trim = [](std::string &t) {
        auto l = t.find_last_not_of(" \t\r\n");
        if (l == std::string::npos) { t.clear(); return; }
        auto f = t.find_first_not_of(" \t\r\n");
        t = t.substr(f, l - f + 1);
    };
    trim(fileAppID);
    trim(fileSerial);
    trim(installedAppID);
    trim(hardwareSerial);

    // Case-insensitive compare for robustness
    auto toUpper = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
        return s;
    };

    // Verify magic first
    bool magicOk = (auth.Magic == 0x53425355);
    if (!magicOk) return false;

    // Verify SHA-256 of first 324 bytes matches Reserved[0..31]
    unsigned char computed[32] = {0};
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    bool hashOk = false;
    if (CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            // hash the first 324 bytes of the struct
            CryptHashData(hHash, reinterpret_cast<BYTE*>(&auth), 324, 0);
            DWORD cbHash = sizeof(computed);
            if (CryptGetHashParam(hHash, HP_HASHVAL, computed, &cbHash, 0)) {
                // compare with reserved
                if (cbHash == 32) {
                    if (std::memcmp(computed, auth.Reserved, 32) == 0) {
                        hashOk = true;
                    }
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }

    if (!hashOk) return false;

    bool appIdOk = (toUpper(installedAppID) == toUpper(fileAppID));
    bool serialOk = (toUpper(hardwareSerial) == toUpper(fileSerial));

    return (appIdOk && serialOk);
}