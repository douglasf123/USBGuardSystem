#pragma once

// Etapas individuais de bloqueio.
bool TryDismountVolume(char driveLetter);
bool TryEjectMedia(char driveLetter);
bool TryRemoveDriveLetter(char driveLetter);
bool IsDriveAccessible(char driveLetter);

// Mantido por compatibilidade com chamadas antigas.
void DismountUSB(char driveLetter);