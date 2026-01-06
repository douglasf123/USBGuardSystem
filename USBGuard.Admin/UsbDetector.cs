using System;
using System.Management;
using System.Text.RegularExpressions;

namespace USBGuard.Admin
{
    public class UsbDetector
    {
        public static string GetPhysicalSerialNumber(string driveLetter)
        {
            try
            {
                string letter = driveLetter.Substring(0, 1).ToUpper() + ":";
                var searcher = new ManagementObjectSearcher(
                    $"ASSOCIATORS OF {{Win32_LogicalDisk.DeviceID='{letter}'}} WHERE AssocClass = Win32_LogicalDiskToPartition");

                foreach (ManagementObject partition in searcher.Get())
                {
                    var driveSearcher = new ManagementObjectSearcher(
                        $"ASSOCIATORS OF {{Win32_DiskPartition.DeviceID='{partition["DeviceID"]}'}} WHERE AssocClass = Win32_DiskDriveToPartition");

                    foreach (ManagementObject drive in driveSearcher.Get())
                    {
                        // Pegamos o PnpDeviceID que vimos na sua imagem de diagnóstico
                        string pnpId = drive["PNPDeviceID"]?.ToString();

                        if (!string.IsNullOrEmpty(pnpId))
                        {
                            // Log the raw PnP ID to a simple text file next to the executable
                            try
                            {
                                string logDir = AppDomain.CurrentDomain.BaseDirectory;
                                string logPath = System.IO.Path.Combine(logDir, "UsbDetector.log");
                                string line = $"{DateTime.Now:yyyy-MM-dd HH:mm:ss} {driveLetter}: PNPID={pnpId}\n";
                                System.IO.File.AppendAllText(logPath, line);
                            }
                            catch { }

                            return TratarSerial(pnpId);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                return "Erro: " + ex.Message;
            }
            return "Serial não encontrado";
        }

        private static string TratarSerial(string pnpId)
        {
            // Split into components and prefer a long alphanumeric token (or one
            // starting with vendor prefix like 'MSFT'). Walk parts from end
            // to start looking for a good candidate.
            string[] partes = pnpId.Split('\\');
            string candidate = null;

            for (int i = partes.Length - 1; i >= 0; --i)
            {
                string part = partes[i];
                if (string.IsNullOrWhiteSpace(part)) continue;
                // strip suffix after '&'
                int amp = part.IndexOf('&');
                if (amp >= 0) part = part.Substring(0, amp);
                part = part.Trim();
                if (part.Length == 0) continue;

                // prefer tokens that look like real serials
                if (part.Length >= 8) return part;
                if (part.StartsWith("MSFT", StringComparison.OrdinalIgnoreCase) && part.Length >= 6) return part;

                // keep short token as fallback
                if (candidate == null) candidate = part;
            }

            // if none long found, search anywhere in the PnP string for a longer token
            try
            {
                var matches = Regex.Matches(pnpId, "[A-Za-z0-9]{6,}");
                string best = null;
                foreach (Match m in matches)
                {
                    var val = m.Value.Trim();
                    if (val.StartsWith("MSFT", StringComparison.OrdinalIgnoreCase)) return val;
                    if (best == null || val.Length > best.Length) best = val;
                }
                if (!string.IsNullOrEmpty(best)) return best;
            }
            catch { }

            return (candidate ?? string.Empty).Trim();
        }
    }
}