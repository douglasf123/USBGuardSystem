#include "HardwareInfo.h"
#include "GlobalConfig.h"
#include <windows.h>
#include <winioctl.h>
#include <vector>
#include <algorithm>
#include <cctype>
#include <ctype.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <string>
#include <cfgmgr32.h>
#include <regex>
// Link against SetupAPI and Cfgmgr32 for device enumeration and CM_ calls
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Cfgmgr32.lib")

std::string GetUsbSerialNumber(char driveLetter) {
    char volumePath[] = "\\\\.\\X:";
    volumePath[4] = driveLetter;

    // Open handle for logical volume to get the device number
    HANDLE hVolume = CreateFileA(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (hVolume == INVALID_HANDLE_VALUE) return "";

    STORAGE_DEVICE_NUMBER volNum = {};
    DWORD bytesReturned = 0;
    if (!DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &volNum, sizeof(volNum), &bytesReturned, NULL)) {
        CloseHandle(hVolume);
        return "";
    }

    // Enumerate disk device interfaces and find matching device number
    GUID diskClass = GUID_DEVINTERFACE_DISK;
    HDEVINFO devs = SetupDiGetClassDevsA(&diskClass, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    std::string result;

    if (devs != INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA ifdata;
        ifdata.cbSize = sizeof(ifdata);
        for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devs, NULL, &diskClass, i, &ifdata); ++i) {
            DWORD needed = 0;
            SetupDiGetDeviceInterfaceDetailA(devs, &ifdata, NULL, 0, &needed, NULL);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) continue;

            std::vector<char> detail(needed);
            SP_DEVICE_INTERFACE_DETAIL_DATA_A* pd = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_A*>(detail.data());
            pd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
            SP_DEVINFO_DATA dinfo;
            dinfo.cbSize = sizeof(dinfo);

            if (!SetupDiGetDeviceInterfaceDetailA(devs, &ifdata, pd, needed, NULL, &dinfo)) continue;

            // Try open the device path and query its device number
            HANDLE hDev = CreateFileA(pd->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hDev == INVALID_HANDLE_VALUE) continue;

            STORAGE_DEVICE_NUMBER devNum = {};
            DWORD cb = 0;
            bool match = false;
            if (DeviceIoControl(hDev, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &devNum, sizeof(devNum), &cb, NULL)) {
                if (devNum.DeviceNumber == volNum.DeviceNumber) {
                    match = true;
                }
            }
            CloseHandle(hDev);

            if (match) {
                // fetch device instance id (PNPDeviceID)
                DWORD req = 0;
                SetupDiGetDeviceInstanceIdA(devs, &dinfo, NULL, 0, &req);
                std::vector<char> inst(req);
                if (SetupDiGetDeviceInstanceIdA(devs, &dinfo, inst.data(), req, NULL)) {
                    std::string devId = inst.data();
                    // Serial is usually after last '\\'
                    size_t pos = devId.find_last_of('\\');
                    if (pos != std::string::npos && pos + 1 < devId.size()) {
                        std::string candidate = devId.substr(pos + 1);
                        // strip after '&'
                        size_t amp = candidate.find('&');
                        if (amp != std::string::npos) candidate = candidate.substr(0, amp);
                        // trim
                        candidate.erase(std::remove_if(candidate.begin(), candidate.end(), [](unsigned char c){ return (c <= 32) || (c > 126); }), candidate.end());

                        // If candidate is short or numeric (e.g. "7"), try to find a
                        // longer alphanumeric token either in this devId or in parent
                        // device instance IDs (some vendors place the serial on parent).
                        auto find_long_token = [](const std::string &s) -> std::string {
                            std::string best;
                            size_t i = 0;
                            while (i < s.size()) {
                                // find alnum run
                                while (i < s.size() && !isalnum((unsigned char)s[i])) ++i;
                                size_t j = i;
                                while (j < s.size() && isalnum((unsigned char)s[j])) ++j;
                                size_t len = j - i;
                                if (len >= 8) {
                                    std::string tok = s.substr(i, len);
                                    if (tok.size() > best.size()) best = tok;
                                }
                                i = j + 1;
                            }
                            return best;
                        };

                        if (candidate.size() < 8) {
                            std::string best = find_long_token(devId);
                            // if not found in current devId, walk up parents
                            if (best.empty()) {
                                DEVINST cur = dinfo.DevInst;
                                for (int depth = 0; depth < 6 && cur != 0; ++depth) {
                                    DEVINST parent = 0;
                                    if (CR_SUCCESS != CM_Get_Parent(&parent, cur, 0) || parent == 0) break;
                                    // attempt to read parent device ID into a fixed buffer
                                    std::vector<char> buf(512);
                                    if (CR_SUCCESS == CM_Get_Device_IDA(parent, buf.data(), (ULONG)buf.size(), 0)) {
                                        std::string pid(buf.data());
                                        std::string tok = find_long_token(pid);
                                        if (!tok.empty()) { best = tok; break; }
                                    }
                                    cur = parent;
                                }
                            }
                            if (!best.empty()) candidate = best;
                        }

                        result = candidate;
                    }
                }
                break;
            }
        }
        SetupDiDestroyDeviceInfoList(devs);
    }

    CloseHandle(hVolume);

    if (!result.empty()) return result;

    // Fallback: use STORAGE_DESCRIPTOR serial
    HANDLE hDevice = CreateFileA(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) return "";

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    STORAGE_DESCRIPTOR_HEADER header = {};
    if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &header, sizeof(header), &bytesReturned, NULL)) {
        CloseHandle(hDevice);
        return "";
    }

    std::vector<char> buffer(header.Size);
    if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer.data(), (DWORD)buffer.size(), &bytesReturned, NULL)) {
        CloseHandle(hDevice);
        return "";
    }

    STORAGE_DEVICE_DESCRIPTOR* desc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer.data());
    std::string serial;
    if (desc->SerialNumberOffset != 0 && desc->SerialNumberOffset != -1) {
        serial = std::string(buffer.data() + desc->SerialNumberOffset);
    }
    CloseHandle(hDevice);

    // sanitize
    serial.erase(std::remove_if(serial.begin(), serial.end(), [](unsigned char c){ return (c <= 32) || (c > 126); }), serial.end());
    auto first = serial.find_first_not_of(' ');
    if (first == std::string::npos) return std::string();
    auto last = serial.find_last_not_of(' ');
    return serial.substr(first, last - first + 1);
}

// Determine if the drive's underlying device is attached to USB by querying
// the storage descriptor BusType field.
bool IsUsbBus(char driveLetter) {
    char volumePath[] = "\\\\.\\X:";
    volumePath[4] = driveLetter;

    HANDLE hDevice = CreateFileA(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) return false;

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    STORAGE_DESCRIPTOR_HEADER header = {};
    DWORD bytesReturned = 0;

    if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
        &header, sizeof(header), &bytesReturned, NULL)) {
        CloseHandle(hDevice);
        return false;
    }

    std::vector<char> buffer(header.Size);
    if (!DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
        buffer.data(), (DWORD)buffer.size(), &bytesReturned, NULL)) {
        CloseHandle(hDevice);
        return false;
    }

    STORAGE_DEVICE_DESCRIPTOR* desc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer.data());
    bool isUsb = (desc->BusType == BusTypeUsb);

    CloseHandle(hDevice);
    return isUsb;
}
