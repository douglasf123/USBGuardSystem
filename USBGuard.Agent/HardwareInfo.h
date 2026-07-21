#pragma once
#include <string>

// Retorna o Serial Number físico do dispositivo associado à letra da unidade (ex: 'E')
// Retorna o Serial Number fisico do dispositivo associado a letra da unidade (ex: 'E')
std::string GetUsbSerialNumber(char driveLetter);

// Retorna true se o dispositivo por tras da letra for conectado via barramento USB
bool IsUsbBus(char driveLetter);
