# USBGuardSystem

USBGuard is a small Windows utility that monitors removable drives and enforces authorization via a binary `device.auth` ticket written to the drive. This repo contains two main projects:

- `USBGuard.Agent` - native C++ background agent that monitors drives and ejects unauthorized devices.
- `USBGuard.Admin` - WinForms admin utility to generate `device.auth` files.

Requirements
- Windows 10+ (some SDK/toolset features may not be supported on older OS versions)
- Visual Studio 2022
- .NET Framework 4.8 (for the Admin project)

How to build
1. Open the solution in Visual Studio 2022.
2. Restore workloads: "Desktop development with C++" and the Windows 10 SDK.
3. Build the solution.

Usage
- Run `USBGuard.Admin` to create an `device.auth` file on a target drive. Make sure the AppID in the registry (`HKEY_LOCAL_MACHINE\\SOFTWARE\\USBGuard\\AppID`) matches the AppID written into the ticket.
- Run `USBGuard.Agent` (as admin) to start monitoring.

Security
- `device.auth` includes a SHA-256 hash to resist casual tampering.

License
- Add your license here.
