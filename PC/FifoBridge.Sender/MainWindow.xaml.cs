using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using FifoBridge.Common;
using Microsoft.Win32;

namespace FifoBridge.Sender;

public partial class MainWindow : Window
{
    private string?                  _filePath;
    private CancellationTokenSource? _cts;

    public MainWindow() => InitializeComponent();

    // -----------------------------------------------------------------------
    // Browse
    // -----------------------------------------------------------------------
    private void BrowseClick(object sender, RoutedEventArgs e)
    {
        var dlg = new OpenFileDialog { Title = "Select file to send" };
        if (dlg.ShowDialog() != true) return;

        _filePath          = dlg.FileName;
        FilePathBox.Text   = _filePath;
        SendButton.IsEnabled = true;
        StatusLabel.Text   = "Ready to send.";
        StatusLabel.Foreground = System.Windows.Media.Brushes.Gray;
    }

    // -----------------------------------------------------------------------
    // Send
    // -----------------------------------------------------------------------
    private async void SendClick(object sender, RoutedEventArgs e)
    {
        if (_filePath is null) return;

        string serial = SerialBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(serial))
        {
            MessageBox.Show("Enter the sender device serial number.", "Validation",
                            MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        SetBusy(true);
        _cts = new CancellationTokenSource();
        var token = _cts.Token;

        try
        {
            await Task.Run(() => DoSend(_filePath, serial, token), token);
            SetStatus("Transfer complete.", success: true);
        }
        catch (OperationCanceledException)
        {
            SetStatus("Cancelled.", success: false);
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
    private void CancelClick(object sender, RoutedEventArgs e)
    {
        _cts?.Cancel();
    }

    // -----------------------------------------------------------------------
    // Core send logic (runs on thread-pool thread)
    // -----------------------------------------------------------------------
    private void DoSend(string filePath, string serial, CancellationToken ct)
    {
        using var ft   = D2xx.OpenBySerial(serial);
        using var file = File.OpenRead(filePath);

        long fileSize = file.Length;

        // Build and send header
        byte[] header = TransferProtocol.BuildHeader(filePath, fileSize);
        ft.Write(header, 0, header.Length);

        // Send payload and compute CRC simultaneously
        var    buf       = new byte[TransferProtocol.ChunkSize];
        long   sent      = 0;
        uint   runningCrc = 0xFFFFFFFFu; // un-finalised
        var    sw        = Stopwatch.StartNew();
        long   lastBytes = 0;
        long   lastMs    = 0;

        while (sent < fileSize)
        {
            ct.ThrowIfCancellationRequested();

            int toRead = (int)Math.Min(buf.Length, fileSize - sent);
            int read   = file.Read(buf, 0, toRead);
            if (read == 0) break;

            ft.Write(buf, 0, read);

            runningCrc = TransferProtocol.Crc32Update(runningCrc, buf.AsSpan(0, read));
            sent      += read;

            // Report progress at most every 50 ms to avoid UI flooding
            long nowMs = sw.ElapsedMilliseconds;
            if (nowMs - lastMs >= 50 || sent == fileSize)
            {
                double pct      = fileSize > 0 ? sent * 100.0 / fileSize : 100.0;
                double elapsed  = (nowMs - lastMs) / 1000.0;
                double speedMbs = elapsed > 0
                    ? (sent - lastBytes) / elapsed / 1_048_576.0
                    : 0;
                lastBytes = sent;
                lastMs    = nowMs;

                Dispatcher.InvokeAsync(() =>
                {
                    Progress.Value     = pct;
                    StatusLabel.Text   = $"{pct:F1}%  –  {speedMbs:F2} MB/s";
                    StatusLabel.Foreground = System.Windows.Media.Brushes.DarkBlue;
                });
            }
        }

        // Finalise CRC and send trailer
        uint finalCrc = runningCrc ^ 0xFFFFFFFFu;
        var  trailer  = new byte[4];
        trailer[0] = (byte)(finalCrc & 0xFF);
        trailer[1] = (byte)((finalCrc >> 8)  & 0xFF);
        trailer[2] = (byte)((finalCrc >> 16) & 0xFF);
        trailer[3] = (byte)((finalCrc >> 24) & 0xFF);
        ft.Write(trailer, 0, 4);
    }

    // -----------------------------------------------------------------------
    // UI helpers
    // -----------------------------------------------------------------------
    private void SetBusy(bool busy)
    {
        Dispatcher.InvokeAsync(() =>
        {
            SendButton.IsEnabled   = !busy && _filePath is not null;
            CancelButton.IsEnabled = busy;
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
