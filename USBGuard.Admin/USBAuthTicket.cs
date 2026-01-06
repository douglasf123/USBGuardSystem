using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace USBGuard.Admin
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential, Pack = 1)] // O 'Pack = 1' é obrigatório aqui
    public struct USBAuthTicket
    {
        public uint Magic;
        // Use fixed-size byte arrays to guarantee exact layout and ASCII encoding
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)] public byte[] AppID;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)] public byte[] Serial;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 188)] public byte[] Reserved;
    }
}
