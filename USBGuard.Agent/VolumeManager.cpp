#include "VolumeManager.h"
#include "GlobalConfig.h"
#include "Logger.h"
#include <windows.h>
#include <winioctl.h>
#include <cstdio>
#include <string>

static void BuildVolumePath(char driveLetter, char* buffer, size_t size)
{
    sprintf_s(buffer, size, "\\\\.\\%c:", driveLetter);
}

static void BuildRootPath(char driveLetter, char* buffer, size_t size)
{
    sprintf_s(buffer, size, "%c:\\", driveLetter);
}

static void LogVolumeAction(const char* action, char driveLetter, BOOL ok, DWORD errorCode)
{
    char msg[256];
    sprintf_s(msg, "%s(%c) ok=%d err=%lu", action, driveLetter, ok ? 1 : 0, static_cast<unsigned long>(errorCode));
    LogDebug(msg);
}

bool TryDismountVolume(char driveLetter)
{
    char volumePath[16] = {};
    BuildVolumePath(driveLetter, volumePath, sizeof(volumePath));

    HANDLE hVolume = CreateFileA(volumePath, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hVolume == INVALID_HANDLE_VALUE) {
        LogVolumeAction("OpenForDismount", driveLetter, FALSE, GetLastError());
        return false;
    }

    DWORD bytesReturned = 0;
    BOOL lockOk = DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
    DWORD lockErr = lockOk ? 0 : GetLastError();
    LogVolumeAction("FSCTL_LOCK_VOLUME", driveLetter, lockOk, lockErr);

    BOOL dismountOk = FALSE;
    DWORD dismountErr = 0;
    if (lockOk) {
        dismountOk = DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        dismountErr = dismountOk ? 0 : GetLastError();
        LogVolumeAction("FSCTL_DISMOUNT_VOLUME", driveLetter, dismountOk, dismountErr);
    }

    CloseHandle(hVolume);
    return (lockOk && dismountOk);
}

bool TryEjectMedia(char driveLetter)
{
    char volumePath[16] = {};
    BuildVolumePath(driveLetter, volumePath, sizeof(volumePath));

    HANDLE hVolume = CreateFileA(volumePath, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hVolume == INVALID_HANDLE_VALUE) {
        LogVolumeAction("OpenForEject", driveLetter, FALSE, GetLastError());
        return false;
    }

    DWORD bytesReturned = 0;
    BOOL lockOk = DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
    DWORD lockErr = lockOk ? 0 : GetLastError();
    LogVolumeAction("FSCTL_LOCK_VOLUME(before eject)", driveLetter, lockOk, lockErr);

    BOOL dismountOk = FALSE;
    DWORD dismountErr = 0;
    if (lockOk) {
        dismountOk = DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        dismountErr = dismountOk ? 0 : GetLastError();
        LogVolumeAction("FSCTL_DISMOUNT_VOLUME(before eject)", driveLetter, dismountOk, dismountErr);
    }

    BOOL ejectOk = DeviceIoControl(hVolume, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &bytesReturned, NULL);
    DWORD ejectErr = ejectOk ? 0 : GetLastError();
    LogVolumeAction("IOCTL_STORAGE_EJECT_MEDIA", driveLetter, ejectOk, ejectErr);

    CloseHandle(hVolume);
    return ejectOk == TRUE;
}

bool TryRemoveDriveLetter(char driveLetter)
{
    char rootPath[8] = {};
    BuildRootPath(driveLetter, rootPath, sizeof(rootPath));

    BOOL removeOk = DeleteVolumeMountPointA(rootPath);
    DWORD removeErr = removeOk ? 0 : GetLastError();
    LogVolumeAction("DeleteVolumeMountPointA", driveLetter, removeOk, removeErr);
    return removeOk == TRUE;
}

bool IsDriveAccessible(char driveLetter)
{
    char rootPath[8] = {};
    BuildRootPath(driveLetter, rootPath, sizeof(rootPath));

    DWORD serialNumber = 0;
    DWORD maxComponentLen = 0;
    DWORD fileSystemFlags = 0;
    char fileSystemName[MAX_PATH] = {};

    BOOL ok = GetVolumeInformationA(
        rootPath,
        NULL,
        0,
        &serialNumber,
        &maxComponentLen,
        &fileSystemFlags,
        fileSystemName,
        MAX_PATH
    );

    return ok == TRUE;
}

void DismountUSB(char driveLetter)
{
    TryDismountVolume(driveLetter);
    TryEjectMedia(driveLetter);
}