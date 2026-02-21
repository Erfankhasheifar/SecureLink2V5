using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using FifoBridge.Common;

namespace FifoBridge.Receiver;

public partial class MainWindow : Window
{
    private string?                  _outputFolder;
    private CancellationTokenSource? _cts;

    public MainWindow() => InitializeComponent();

    // -----------------------------------------------------------------------
    // Browse output folder
    // -----------------------------------------------------------------------
    private void BrowseClick(object sender, RoutedEventArgs e)
    {
        // Use a FolderBrowserDialog via WPF's OpenFileDialog workaround
        var dlg = new Microsoft.Win32.OpenFolderDialog
        {
            Title = "Select output folder"
        };
        if (dlg.ShowDialog() != true) return;

        _outputFolder  = dlg.FolderName;
        FolderBox.Text = _outputFolder;
    }

    // -----------------------------------------------------------------------
    // Receive
    // -----------------------------------------------------------------------
    private async void ReceiveClick(object sender, RoutedEventArgs e)
    {
        string serial = SerialBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(serial))
        {
            MessageBox.Show("Enter the receiver device serial number.", "Validation",
                            MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (string.IsNullOrWhiteSpace(_outputFolder) ||
            !Directory.Exists(_outputFolder))
        {
            MessageBox.Show("Choose a valid output folder.", "Validation",
                            MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        SetBusy(true);
        _cts = new CancellationTokenSource();
        var token    = _cts.Token;
        var outFolder = _outputFolder;

        try
        {
            string savedPath = await Task.Run(
                () => DoReceive(serial, outFolder, token), token);
            SetStatus($"Saved: {savedPath}", success: true);
        }
        catch (OperationCanceledException)
        {
            SetStatus("Cancelled.", success: false);
        }
        catch (InvalidDataException ex)
        {
            SetStatus($"Protocol error: {ex.Message}", success: false);
        }
        catch (Exception ex)
        {
            SetStatus($"Error: {ex.Message}", success: false);
        }
        finally
        {
            SetBusy(false);
        }
    }

    // -----------------------------------------------------------------------
    // Cancel
    // -----------------------------------------------------------------------
    private void CancelClick(object sender, RoutedEventArgs e) => _cts?.Cancel();

    // -----------------------------------------------------------------------
    // Core receive logic (thread-pool thread)
    // -----------------------------------------------------------------------
    private string DoReceive(string serial, string outputFolder,
                             CancellationToken ct)
    {
        using var ft = D2xx.OpenBySerial(serial);

        // ----- Wait for header -----
        // Poll until enough bytes for the minimum header arrive
        // Minimum header = 4+2+2+0+8+4 = 20 bytes
        WaitForBytes(ft, 20, ct);

        // Use a stream wrapper to read header bytes from the FIFO
        using var fifoStream = new FifoReadStream(ft, ct);
        using var br         = new BinaryReader(fifoStream,
                                System.Text.Encoding.UTF8, leaveOpen: true);

        var header = TransferProtocol.ReadHeader(br);

        // Sanitise filename (strip any path components from sender)
        string safeFilename = Path.GetFileName(header.Filename);
        if (string.IsNullOrWhiteSpace(safeFilename)) safeFilename = "received_file";
        string outputPath = Path.Combine(outputFolder, safeFilename);

        Dispatcher.InvokeAsync(() =>
            ReceivedFileLabel.Text = $"{safeFilename}  ({header.FileSize:N0} bytes)");

        // ----- Receive payload -----
        long   remaining   = header.FileSize;
        uint   runningCrc  = 0xFFFFFFFFu;
        var    buf         = new byte[TransferProtocol.ChunkSize];
        var    sw          = Stopwatch.StartNew();
        long   received    = 0;
        long   lastBytes   = 0;
        long   lastMs      = 0;

        using (var outFile = File.Create(outputPath))
        {
            while (remaining > 0)
            {
                ct.ThrowIfCancellationRequested();

                int toRead = (int)Math.Min(buf.Length, remaining);
                int got    = fifoStream.Read(buf, 0, toRead);
                if (got == 0)
                {
                    Thread.Sleep(1); // wait for more data
                    continue;
                }

                outFile.Write(buf, 0, got);
                runningCrc  = TransferProtocol.Crc32Update(runningCrc,
                                   buf.AsSpan(0, got));
                remaining  -= got;
                received   += got;

                long nowMs = sw.ElapsedMilliseconds;
                if (nowMs - lastMs >= 50 || remaining == 0)
                {
                    double pct     = header.FileSize > 0
                        ? received * 100.0 / header.FileSize : 100.0;
                    double elapsed = (nowMs - lastMs) / 1000.0;
                    double speed   = elapsed > 0
                        ? (received - lastBytes) / elapsed / 1_048_576.0 : 0;
                    lastBytes = received;
                    lastMs    = nowMs;

                    Dispatcher.InvokeAsync(() =>
                    {
                        Progress.Value     = pct;
                        StatusLabel.Text   = $"{pct:F1}%  –  {speed:F2} MB/s";
                        StatusLabel.Foreground = System.Windows.Media.Brushes.DarkBlue;
                    });
                }
            }
        }

        // ----- Verify CRC -----
        uint finalCrc = runningCrc ^ 0xFFFFFFFFu;

        // Read 4-byte trailer CRC
        WaitForBytes(ft, 4, ct);
        byte[] trailerBuf = new byte[4];
        int    trailerGot = 0;
        while (trailerGot < 4)
        {
            ct.ThrowIfCancellationRequested();
            trailerGot += fifoStream.Read(trailerBuf, trailerGot, 4 - trailerGot);
        }
        uint receivedCrc = BitConverter.ToUInt32(trailerBuf, 0);

        if (receivedCrc != finalCrc)
        {
            File.Delete(outputPath);
            throw new InvalidDataException(
                $"CRC mismatch: expected 0x{finalCrc:X8}, received 0x{receivedCrc:X8}. File deleted.");
        }

        return outputPath;
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// <summary>Block until at least <paramref name="count"/> bytes are in the RX queue.</summary>
    private static void WaitForBytes(D2xx.FtDevice ft, int count, CancellationToken ct)
    {
        while (ft.RxBytesAvailable < (uint)count)
        {
            ct.ThrowIfCancellationRequested();
            Thread.Sleep(1);
        }
    }

    /// <summary>
    /// Thin <see cref="Stream"/> wrapper that reads from an <see cref="D2xx.FtDevice"/>.
    /// </summary>
    private sealed class FifoReadStream(D2xx.FtDevice ft, CancellationToken ct)
        : Stream
    {
        public override bool CanRead  => true;
        public override bool CanSeek  => false;
        public override bool CanWrite => false;
        public override long Length   => throw new NotSupportedException();
        public override long Position
        {
            get => throw new NotSupportedException();
            set => throw new NotSupportedException();
        }
        public override void Flush() {}
        public override long Seek(long offset, SeekOrigin origin)
            => throw new NotSupportedException();
        public override void SetLength(long value)
            => throw new NotSupportedException();
        public override void Write(byte[] buffer, int offset, int count)
            => throw new NotSupportedException();

        public override int Read(byte[] buffer, int offset, int count)
        {
            // Poll until data is available
            while (ft.RxBytesAvailable == 0)
            {
                ct.ThrowIfCancellationRequested();
                Thread.Sleep(1);
            }
            int toRead = (int)Math.Min((uint)count, ft.RxBytesAvailable);
            return ft.Read(buffer, offset, toRead);
        }
    }

    private void SetBusy(bool busy)
    {
        Dispatcher.InvokeAsync(() =>
        {
            ReceiveButton.IsEnabled = !busy;
            CancelButton.IsEnabled  = busy;
        });
    }

    private void SetStatus(string msg, bool success)
    {
        Dispatcher.InvokeAsync(() =>
        {
            StatusLabel.Text       = msg;
            StatusLabel.Foreground = success
                ? System.Windows.Media.Brushes.DarkGreen
                : System.Windows.Media.Brushes.Red;
            Progress.Value = success ? 100 : Progress.Value;
        });
    }
}
