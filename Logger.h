#pragma once

#include <string>

#pragma once

#include <string>

// Registra eventos de acesso USB no arquivo de log ao lado do executavel.
// Ex: 2026-01-05 13:45:12 E: ABC123 Liberado
// note: optional free-text appended in brackets (e.g. "SerialChanged")
void LogUsbEvent(char driveLetter, const std::string& serial, bool allowed, const std::string& note = "");

// Registra mensagens de depuração no mesmo arquivo de log.
void LogDebug(const std::string& text);