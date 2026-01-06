USBGuard.Agent

Build and packaging notes:
- Native C++ project. Ensure the v142/v143 toolset and Windows 10 SDK are installed.
- The agent uses SetupAPI/CfgMgr/Crypt32; linking is handled in-source with pragmas.
- The logger writes to `USBGuard.Agent.log` next to the binary or to `C:\ProgramData\USBGuard` as fallback.
