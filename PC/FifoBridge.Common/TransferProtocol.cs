using System;
using System.IO;
using System.Text;

namespace FifoBridge.Common;

/// <summary>
/// Shared file-transfer protocol used by the Sender and Receiver apps.
///
/// Wire format (all integers are little-endian):
///
///   Header:
///     [4]  Magic          = 0x46494642  ("FIFB")
///     [2]  Version        = 1
///     [2]  FilenameLength (bytes, UTF-8, no NUL)
///     [N]  Filename       (UTF-8 bytes)
///     [8]  FileSize       (uint64)
///     [4]  HeaderCRC32    (CRC32 of all preceding header bytes)
///
///   Payload:
///     [FileSize] raw file bytes
///
///   Trailer:
///     [4]  PayloadCRC32   (CRC32 of all payload bytes)
/// </summary>
public static class TransferProtocol
{
    public const uint  Magic         = 0x46494642u; // "FIFB"
    public const ushort Version      = 1;
    public const int   ChunkSize     = 65536;       // read/write chunk (bytes)

    // -----------------------------------------------------------------------
    // CRC-32 (ISO 3309 / ITU-T V.42 – same polynomial as zlib/zip)
    // -----------------------------------------------------------------------
    private static readonly uint[] CrcTable = BuildCrcTable();

    private static uint[] BuildCrcTable()
    {
        var table = new uint[256];
        for (uint i = 0; i < 256; i++)
        {
            uint c = i;
            for (int j = 0; j < 8; j++)
                c = (c & 1) != 0 ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        return table;
    }

    public static uint Crc32(ReadOnlySpan<byte> data, uint crc = 0xFFFFFFFFu)
    {
        foreach (byte b in data)
            crc = CrcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFFu;
    }

    public static uint Crc32Update(uint crc, ReadOnlySpan<byte> data)
    {
        // Accept running (un-finalised) CRC; caller finalises with XOR 0xFFFFFFFF
        foreach (byte b in data)
            crc = CrcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
        return crc;
    }

    // -----------------------------------------------------------------------
    // Header helpers
    // -----------------------------------------------------------------------

    /// <summary>Build the binary header for a file transfer.</summary>
    public static byte[] BuildHeader(string filename, long fileSize)
    {
        byte[] nameBytes = Encoding.UTF8.GetBytes(Path.GetFileName(filename));
        if (nameBytes.Length > ushort.MaxValue)
            throw new ArgumentException("Filename too long.");

        using var ms = new MemoryStream(4 + 2 + 2 + nameBytes.Length + 8 + 4);
        using var bw = new BinaryWriter(ms, Encoding.UTF8, leaveOpen: true);

        bw.Write(Magic);                        // 4
        bw.Write(Version);                      // 2
        bw.Write((ushort)nameBytes.Length);     // 2
        bw.Write(nameBytes);                    // N
        bw.Write((ulong)fileSize);              // 8
        bw.Flush();

        byte[] hdrWithoutCrc = ms.ToArray();
        uint crc = Crc32(hdrWithoutCrc);
        bw.Write(crc);                          // 4

        return ms.ToArray();
    }

    // -----------------------------------------------------------------------
    // Parsed header
    // -----------------------------------------------------------------------

    public record FileHeader(string Filename, long FileSize);

    /// <summary>
    /// Read and validate a transfer header from <paramref name="reader"/>.
    /// Throws <see cref="InvalidDataException"/> on format or CRC mismatch.
    /// </summary>
    public static FileHeader ReadHeader(BinaryReader reader)
    {
        // We need to capture all header bytes for CRC check.
        using var capture = new MemoryStream();
        using var capWriter = new BinaryWriter(capture, Encoding.UTF8, leaveOpen: true);

        uint magic = reader.ReadUInt32(); capWriter.Write(magic);
        if (magic != Magic)
            throw new InvalidDataException($"Bad magic: 0x{magic:X8}");

        ushort version = reader.ReadUInt16(); capWriter.Write(version);
        if (version != Version)
            throw new InvalidDataException($"Unknown protocol version {version}");

        ushort nameLen = reader.ReadUInt16(); capWriter.Write(nameLen);
        byte[] nameBytes = reader.ReadBytes(nameLen); capWriter.Write(nameBytes);

        ulong fileSize = reader.ReadUInt64(); capWriter.Write(fileSize);
        capWriter.Flush();

        byte[] hdrBytes = capture.ToArray();
        uint expectedCrc = Crc32(hdrBytes);
        uint actualCrc   = reader.ReadUInt32();
        if (actualCrc != expectedCrc)
            throw new InvalidDataException(
                $"Header CRC mismatch: expected 0x{expectedCrc:X8}, got 0x{actualCrc:X8}");

        string filename = Encoding.UTF8.GetString(nameBytes);
        return new FileHeader(filename, (long)fileSize);
    }
}
