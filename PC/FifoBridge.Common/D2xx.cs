using System;
using System.Runtime.InteropServices;
using System.Text;

namespace FifoBridge.Common;

/// <summary>
/// Minimal P/Invoke wrapper around the FTDI D2XX (FTD2XX.dll) API.
/// Only the functions needed for 245 Sync FIFO operation are exposed.
///
/// Prerequisites:
///   - Place FTD2XX.dll (x64) next to the executable, or in System32.
///   - Build the calling project as x64 (Platform target = x64).
///   - The FT2232HL channel must already be configured for 245 FIFO mode
///     and D2XX Direct driver via FT_Prog.
/// </summary>
public static class D2xx
{
    private const string Dll = "FTD2XX.dll";

    // -----------------------------------------------------------------------
    // Status codes
    // -----------------------------------------------------------------------
    public enum FtStatus : uint
    {
        FT_OK                          = 0,
        FT_INVALID_HANDLE              = 1,
        FT_DEVICE_NOT_FOUND            = 2,
        FT_DEVICE_NOT_OPENED           = 3,
        FT_IO_ERROR                    = 4,
        FT_INSUFFICIENT_RESOURCES      = 5,
        FT_INVALID_PARAMETER           = 6,
        FT_INVALID_BAUD_RATE           = 7,
        FT_DEVICE_NOT_OPENED_FOR_ERASE = 8,
        FT_DEVICE_NOT_OPENED_FOR_WRITE = 9,
        FT_FAILED_TO_WRITE_DEVICE      = 10,
        FT_EEPROM_READ_FAILED          = 11,
        FT_EEPROM_WRITE_FAILED         = 12,
        FT_EEPROM_ERASE_FAILED         = 13,
        FT_EEPROM_NOT_PRESENT          = 14,
        FT_EEPROM_NOT_PROGRAMMED       = 15,
        FT_INVALID_ARGS                = 16,
        FT_NOT_SUPPORTED               = 17,
        FT_OTHER_ERROR                 = 18,
    }

    // -----------------------------------------------------------------------
    // Bit-mode flags
    // -----------------------------------------------------------------------
    [Flags]
    public enum FtBitMode : byte
    {
        Reset         = 0x00,
        AsyncBitbang  = 0x01,
        Mpsse         = 0x02,
        SyncBitbang   = 0x04,
        Mcu           = 0x08,
        Opto          = 0x10,
        Cbus          = 0x20,
        SyncFifo      = 0x40,
    }

    // -----------------------------------------------------------------------
    // Native imports
    // -----------------------------------------------------------------------
    [DllImport(Dll, EntryPoint = "FT_CreateDeviceInfoList")]
    private static extern FtStatus FT_CreateDeviceInfoList(out uint numDevs);

    [DllImport(Dll, EntryPoint = "FT_GetDeviceInfoList")]
    private static extern FtStatus FT_GetDeviceInfoList(
        [Out] FtDeviceInfoNode[] dest, ref uint count);

    [DllImport(Dll, EntryPoint = "FT_OpenEx")]
    private static extern FtStatus FT_OpenEx(
        string pvArg1, uint dwFlags, out IntPtr ftHandle);

    [DllImport(Dll, EntryPoint = "FT_Close")]
    private static extern FtStatus FT_Close(IntPtr ftHandle);

    [DllImport(Dll, EntryPoint = "FT_ResetDevice")]
    private static extern FtStatus FT_ResetDevice(IntPtr ftHandle);

    [DllImport(Dll, EntryPoint = "FT_SetBitMode")]
    private static extern FtStatus FT_SetBitMode(
        IntPtr ftHandle, byte ucMask, byte ucMode);

    [DllImport(Dll, EntryPoint = "FT_SetUSBParameters")]
    private static extern FtStatus FT_SetUSBParameters(
        IntPtr ftHandle, uint ulInTransferSize, uint ulOutTransferSize);

    [DllImport(Dll, EntryPoint = "FT_SetLatencyTimer")]
    private static extern FtStatus FT_SetLatencyTimer(
        IntPtr ftHandle, byte ucLatency);

    [DllImport(Dll, EntryPoint = "FT_SetFlowControl")]
    private static extern FtStatus FT_SetFlowControl(
        IntPtr ftHandle, ushort usFlowControl, byte uXon, byte uXoff);

    [DllImport(Dll, EntryPoint = "FT_Read")]
    private static extern FtStatus FT_Read(
        IntPtr ftHandle, byte[] lpBuffer, uint dwBytesToRead,
        out uint lpdwBytesReturned);

    [DllImport(Dll, EntryPoint = "FT_Write")]
    private static extern FtStatus FT_Write(
        IntPtr ftHandle, byte[] lpBuffer, uint dwBytesToWrite,
        out uint lpdwBytesWritten);

    [DllImport(Dll, EntryPoint = "FT_GetQueueStatus")]
    private static extern FtStatus FT_GetQueueStatus(
        IntPtr ftHandle, out uint dwRxBytes);

    [DllImport(Dll, EntryPoint = "FT_Purge")]
    private static extern FtStatus FT_Purge(IntPtr ftHandle, uint dwMask);

    // -----------------------------------------------------------------------
    // Device info node (must match D2XX SDK structure layout)
    // -----------------------------------------------------------------------
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct FtDeviceInfoNode
    {
        public uint Flags;
        public uint Type;
        public uint ID;
        public uint LocId;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string SerialNumber;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string Description;
        public IntPtr FtHandle;
    }

    // -----------------------------------------------------------------------
    // Open/close flags
    // -----------------------------------------------------------------------
    private const uint FT_OPEN_BY_SERIAL_NUMBER = 1;
    private const uint FT_PURGE_RX              = 1;
    private const uint FT_PURGE_TX              = 2;
    private const ushort FT_FLOW_NONE           = 0;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /// <summary>Enumerate all connected D2XX devices.</summary>
    public static FtDeviceInfoNode[] GetDeviceList()
    {
        Check(FT_CreateDeviceInfoList(out uint count));
        if (count == 0) return Array.Empty<FtDeviceInfoNode>();
        var nodes = new FtDeviceInfoNode[count];
        Check(FT_GetDeviceInfoList(nodes, ref count));
        return nodes;
    }

    /// <summary>
    /// Open the first device whose serial number starts with
    /// <paramref name="serial"/> and configure it for 245 Sync FIFO mode.
    /// </summary>
    /// <param name="serial">Serial number prefix (e.g. "FTBA7CIZ").</param>
    /// <param name="usbTransferSize">USB bulk transfer size in bytes (default 64 KB).</param>
    /// <returns>An open <see cref="FtDevice"/>.</returns>
    public static FtDevice OpenBySerial(string serial,
                                        uint usbTransferSize = 65536)
    {
        Check(FT_OpenEx(serial, FT_OPEN_BY_SERIAL_NUMBER, out IntPtr handle));

        var dev = new FtDevice(handle);
        try
        {
            dev.Configure245SyncFifo(usbTransferSize);
        }
        catch
        {
            dev.Close();
            throw;
        }
        return dev;
    }

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------
    internal static void Check(FtStatus st, string? context = null)
    {
        if (st != FtStatus.FT_OK)
        {
            throw new InvalidOperationException(
                $"D2XX error {st}" + (context is null ? "" : $" ({context})"));
        }
    }

    // -----------------------------------------------------------------------
    // Device handle wrapper
    // -----------------------------------------------------------------------

    /// <summary>
    /// Represents an open FT2232HL device handle.  Dispose to close.
    /// </summary>
    public sealed class FtDevice : IDisposable
    {
        private IntPtr _handle;
        private bool   _disposed;

        internal FtDevice(IntPtr handle) => _handle = handle;

        /// <summary>Configure for 245 Synchronous FIFO mode.</summary>
        internal void Configure245SyncFifo(uint usbTransferSize)
        {
            Check(FT_ResetDevice(_handle),          "ResetDevice");
            // No flow control in 245 FIFO mode
            Check(FT_SetFlowControl(_handle, FT_FLOW_NONE, 0, 0), "SetFlowControl");
            // Latency of 2 ms gives a good throughput/latency balance
            Check(FT_SetLatencyTimer(_handle, 2),   "SetLatencyTimer");
            Check(FT_SetUSBParameters(_handle, usbTransferSize, usbTransferSize),
                  "SetUSBParameters");
            // Purge buffers before switching mode
            Check(FT_Purge(_handle, FT_PURGE_RX | FT_PURGE_TX), "Purge");
            // Switch to 245 Sync FIFO – mask is 0xFF (all bits as FIFO)
            Check(FT_SetBitMode(_handle, 0xFF, (byte)FtBitMode.SyncFifo),
                  "SetBitMode");
        }

        /// <summary>Number of bytes waiting in the receive queue.</summary>
        public uint RxBytesAvailable
        {
            get
            {
                Check(FT_GetQueueStatus(_handle, out uint n));
                return n;
            }
        }

        /// <summary>
        /// Read up to <paramref name="count"/> bytes.
        /// Returns the number of bytes actually read.
        /// </summary>
        public int Read(byte[] buffer, int offset, int count)
        {
            if (count == 0) return 0;
            // D2XX API requires a contiguous array starting at index 0
            byte[] tmp = offset == 0 ? buffer : new byte[count];
            Check(FT_Read(_handle, tmp, (uint)count, out uint got));
            if (offset != 0 && got > 0)
                Buffer.BlockCopy(tmp, 0, buffer, offset, (int)got);
            return (int)got;
        }

        /// <summary>Write <paramref name="count"/> bytes.</summary>
        /// <returns>Number of bytes written.</returns>
        public int Write(byte[] buffer, int offset, int count)
        {
            if (count == 0) return 0;
            byte[] tmp;
            if (offset == 0 && buffer.Length == count)
            {
                tmp = buffer;
            }
            else
            {
                tmp = new byte[count];
                Buffer.BlockCopy(buffer, offset, tmp, 0, count);
            }
            Check(FT_Write(_handle, tmp, (uint)count, out uint sent));
            return (int)sent;
        }

        public void Close() => Dispose();

        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            if (_handle != IntPtr.Zero)
            {
                FT_Close(_handle);
                _handle = IntPtr.Zero;
            }
        }
    }
}
