#include "VolumeManager.h"
#include "GlobalConfig.h"
#include <windows.h>
#include <winioctl.h>
#include <cstdio>

void DismountUSB(char driveLetter) {
    char volumePath[] = "\\\\.\\X:";
    volumePath[4] = driveLetter;

    // 1. Abrir o handle do volume com privilégios de escrita para poder ejetar
    HANDLE hVolume = CreateFileA(volumePath, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (hVolume != INVALID_HANDLE_VALUE) {
        DWORD bytesReturned;

        // 2. Tenta dar o LOCK no volume (impede que outros programas usem enquanto ejetamos)
        if (DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL)) {

            // 3. Comando de DISMOUNT (desmonta o sistema de arquivos)
            if (DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL)) {

                // 4. Opcional: Comando para ejetar fisicamente o dispositivo (mídia removível)
                DeviceIoControl(hVolume, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &bytesReturned, NULL);
            }
        }

        CloseHandle(hVolume);
    }
}