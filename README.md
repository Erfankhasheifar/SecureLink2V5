# FT2232HL Channel A 245 Sync FIFO File Bridge

End-to-end system for transferring files between two PCs via a pair of
**CJMCU-2232HL** (FT2232HL) modules connected to an **STM32H750 DevEBox**
running FreeRTOS.

```
PC (Sender)  ─── USB ───►  CJMCU-2232HL #1 (FTBA7CJ0)
                                │  Channel A 245 FIFO
                                ▼
                          STM32H750 DevEBox
                          (FreeRTOS bridge firmware)
                                │  Channel A 245 FIFO
                                ▼
                         CJMCU-2232HL #2 (FTBA7CIZ)  ◄─── USB ─── PC (Receiver)
```

---

## Repository Layout

```
FIFO-Docs/
├── Firmware/                   STM32H750 CubeIDE project
│   ├── FIFO_Bridge.ioc         CubeMX configuration
│   └── Core/
│       ├── Inc/
│       │   ├── fifo_bridge.h   GPIO macros & task prototypes
│       │   └── ring_buffer.h   Lock-free SPSC ring buffer
│       └── Src/
│           ├── main.c          Clock + GPIO init, FreeRTOS startup
│           └── fifo_bridge.c   ReaderTask + WriterTask
├── PC/                         .NET 8 WPF applications
│   ├── FifoBridge.sln
│   ├── FifoBridge.Common/      Shared D2XX wrapper & protocol
│   ├── FifoBridge.Sender/      WPF Sender app
│   └── FifoBridge.Receiver/    WPF Receiver app
└── README.md                   This file
```

---

## Hardware Requirements

| Item | Qty | Notes |
|------|-----|-------|
| STM32H750VB DevEBox | 1 | 25 MHz HSE crystal fitted |
| CJMCU-2232HL | 2 | FT2232HL USB-HS modules |
| USB-A to Mini-B cables | 2 | One per CJMCU-2232HL |
| ST-Link v2 / DAPLink | 1 | For flashing firmware |
| Breadboard + jumper wires | – | 26 signal wires total |

---

## FT_Prog Configuration (Must Do First)

Download **FT_Prog** from the [FTDI website](https://ftdichip.com/utilities/).

For **each** CJMCU-2232HL:

1. Plug the module into USB and open FT_Prog → **Scan and Parse**.
2. Navigate to **FT EEPROM → Device → Hardware Specific → Port A**.
3. Set **Hardware** = `245 FIFO`.
4. Set **Driver** = `D2XX Direct`.
5. Port B settings are unused; leave as default.
6. Click **Program** → confirm the warning.
7. Unplug and replug the module.
8. Repeat for the second module (they will get different serial numbers, e.g.
   **FTBA7CJ0** and **FTBA7CIZ** — note them down for use in the WPF apps).

> **Control pin mapping** (FT2232HL Channel A, 245 FIFO mode):
>
> | ACBUS pin | FT2232HL signal | Direction (from FTDI) |
> |-----------|----------------|-----------------------|
> | AC0       | RXF#           | Output (FTDI → MCU)   |
> | AC1       | TXE#           | Output (FTDI → MCU)   |
> | AC2       | RD#            | Input  (MCU → FTDI)   |
> | AC3       | WR#            | Input  (MCU → FTDI)   |
> | AC5       | CLKOUT         | Output (FTDI → MCU)   |
> | AC6       | OE#            | Input  (MCU → FTDI)   |
>
> *(Source: FT2232H Datasheet, Table 3.3)*

---

## Wiring Diagram

### FIFO#1 — CJMCU-2232HL #1 (Sender, serial FTBA7CJ0) → STM32H750

```
CJMCU-2232HL #1        STM32H750 DevEBox
─────────────────      ──────────────────
ADBUS0  (D0)    ──────► PE0  (FIFO1_D0)
ADBUS1  (D1)    ──────► PE1  (FIFO1_D1)
ADBUS2  (D2)    ──────► PE2  (FIFO1_D2)
ADBUS3  (D3)    ──────► PE3  (FIFO1_D3)
ADBUS4  (D4)    ──────► PE4  (FIFO1_D4)
ADBUS5  (D5)    ──────► PE5  (FIFO1_D5)
ADBUS6  (D6)    ──────► PE6  (FIFO1_D6)
ADBUS7  (D7)    ──────► PE7  (FIFO1_D7)

ACBUS0  (RXF#)  ──────► PC0  (FIFO1_RXF)  [input to MCU, active-low]
ACBUS1  (TXE#)  ──────► PC1  (FIFO1_TXE)  [input to MCU, active-low]
ACBUS2  (RD#)   ◄──────  PC2  (FIFO1_RD)   [output from MCU, active-low]
ACBUS3  (WR#)   ◄──────  PC3  (FIFO1_WR)   [tied high – read-only mode]
ACBUS5  (CLKOUT)──────► PC4  (FIFO1_CLK)  [60 MHz bus clock, input]
ACBUS6  (OE#)   ◄──────  PC5  (FIFO1_OE)   [output from MCU, active-low]

GND             ──────── GND
```

### FIFO#2 — STM32H750 → CJMCU-2232HL #2 (Receiver, serial FTBA7CIZ)

```
STM32H750 DevEBox      CJMCU-2232HL #2
──────────────────     ─────────────────
PF0  (FIFO2_D0) ──────► ADBUS0  (D0)
PF1  (FIFO2_D1) ──────► ADBUS1  (D1)
PF2  (FIFO2_D2) ──────► ADBUS2  (D2)
PF3  (FIFO2_D3) ──────► ADBUS3  (D3)
PF4  (FIFO2_D4) ──────► ADBUS4  (D4)
PF5  (FIFO2_D5) ──────► ADBUS5  (D5)
PF6  (FIFO2_D6) ──────► ADBUS6  (D6)
PF7  (FIFO2_D7) ──────► ADBUS7  (D7)

PD0  (FIFO2_RXF) ◄──────  ACBUS0  (RXF#)  [optional, input]
PD1  (FIFO2_TXE) ◄──────  ACBUS1  (TXE#)  [input to MCU, active-low]
PD2  (FIFO2_RD)  ──────►  ACBUS2  (RD#)   [optional output]
PD3  (FIFO2_WR)  ──────►  ACBUS3  (WR#)   [output from MCU, active-low]
PD4  (FIFO2_CLK) ◄──────  ACBUS5  (CLKOUT)[optional 60 MHz input]
PD5  (FIFO2_OE)  ──────►  ACBUS6  (OE#)   [optional output]

GND              ──────── GND
```

> **Important:** Share a common GND between both CJMCU-2232HL modules and the
> DevEBox. The 3.3 V I/O levels are compatible directly.

---

## Firmware Setup

### Prerequisites
- **STM32CubeIDE 1.14+** with STM32Cube FW_H7 V1.11.0 pack installed.
- ST-Link v2 or any SWD programmer.

### Steps

1. **Open project** in STM32CubeIDE:
   `File → Open Projects from File System → Firmware/`

2. **Regenerate code** (optional – only if you change the `.ioc`):
   Double-click `FIFO_Bridge.ioc` → **Generate Code**.

3. **Build**: `Project → Build All` (Ctrl+B).

4. **Flash**: `Run → Debug` with ST-Link connected to the DevEBox SWD header.

### Clock Configuration
The `.ioc` is set up for a **25 MHz HSE crystal** (fitted on the DevEBox):

| Domain | Frequency |
|--------|-----------|
| SYSCLK | 480 MHz   |
| AHB    | 240 MHz   |
| APB1   | 120 MHz   |
| APB2   | 120 MHz   |

### STM32H7 Cache Considerations

The Cortex-M7 D-cache is enabled by `SystemInit()`. The ring buffer lives in
**AXI-SRAM** (`0x24000000`), which is in the Write-Back/Read-Allocate MPU
region. Because only the CPU reads/writes the ring buffer (no DMA), cache
coherency is maintained automatically.

> **If you add DMA later:**
> - Either place DMA buffers in a dedicated **Non-Cacheable** MPU region, or
> - Call `SCB_InvalidateDCache_by_Addr()` before reading DMA-transferred data
>   and `SCB_CleanDCache_by_Addr()` before initiating a DMA write.

---

## PC Applications Setup

### Prerequisites
- **.NET 8 SDK** (x64) — [download](https://dotnet.microsoft.com/download)
- **FTDI D2XX drivers** installed on both PCs —
  [download](https://ftdichip.com/drivers/d2xx-drivers/)
- **FTD2XX.dll** (x64) — found in the D2XX driver package
  (`amd64/FTD2XX.dll`).

### DLL Deployment

Copy `FTD2XX.dll` (x64) into:
- `PC/FifoBridge.Sender/` (next to the `.csproj`)
- `PC/FifoBridge.Receiver/` (next to the `.csproj`)

The project file copies it to the output directory automatically.

Alternatively, copy `FTD2XX.dll` to `C:\Windows\System32\` (system-wide).

### Build

```powershell
cd PC
dotnet build FifoBridge.sln -c Release -r win-x64
```

Or open `PC/FifoBridge.sln` in **Visual Studio 2022** and build the solution
(platform = **x64**).

### Running the Sender

1. Launch `FifoBridge.Sender.exe` on the **sending PC**.
2. Verify the **Sender Device Serial** field shows the serial of CJMCU #1
   (e.g. `FTBA7CJ0A` — note the channel suffix `A`).
3. Click **Browse…** and select the file to transfer.
4. Click **Send**. Progress percentage and speed are shown in real time.
5. Click **Cancel** to abort at any time.

### Running the Receiver

1. Launch `FifoBridge.Receiver.exe` on the **receiving PC**.
2. Verify the **Receiver Device Serial** matches CJMCU #2 (e.g. `FTBA7CIZ`).
3. Click **Browse…** to choose the output folder.
4. Click **Receive**. The app waits for a transfer from the Sender.
5. After the transfer completes, the CRC32 is verified automatically.
   - ✅ Green status = file saved and verified.
   - ❌ Red status = CRC mismatch (file deleted automatically).

---

## Transfer Protocol Reference

All integers are **little-endian**.

### Header

| Offset | Size | Field          | Value |
|--------|------|----------------|-------|
| 0      | 4    | Magic          | `0x46494642` ("FIFB") |
| 4      | 2    | Version        | `1`   |
| 6      | 2    | FilenameLength | N (bytes, UTF-8) |
| 8      | N    | Filename       | UTF-8 bytes (no NUL) |
| 8+N    | 8    | FileSize       | uint64 |
| 16+N   | 4    | HeaderCRC32    | CRC32 of all preceding bytes |

### Payload
Raw file bytes, `FileSize` in total.

### Trailer

| Offset | Size | Field         | Value |
|--------|------|---------------|-------|
| 0      | 4    | PayloadCRC32  | CRC32 of the entire payload |

CRC32 uses the standard ISO 3309 polynomial (same as zlib/zip).

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `D2XX error FT_DEVICE_NOT_FOUND` | FT_Prog not applied, or wrong serial | Re-run FT_Prog; check serial in app |
| `D2XX error FT_DEVICE_NOT_OPENED` | VCP driver still bound | Switch to D2XX Direct in FT_Prog |
| Firmware hangs at first byte | OE# not asserted / wiring issue | Check PC5 → ACBUS6 and GND |
| CRC mismatch | Data corruption in GPIO wiring | Shorten wires; check common GND |
| Transfer very slow (< 1 MB/s) | USB HS not negotiated | Use USB 2.0 port; check cable |
| `InvalidOperationException` in Receiver | Header bytes not yet available | Sender must start first; retry |
| GPIO signals absent / firmware stuck | DevEBox variant does not expose Port G | This firmware uses **PC0–PC5** (FIFO1) and **PD0–PD5** (FIFO2) instead of PG. Verify wiring matches the table above; some early DevEBox silk-screens label Port G but the header pins are not connected. |

---

## References

- [FT2232H Datasheet](https://ftdichip.com/wp-content/uploads/2020/07/DS_FT2232H.pdf)
- [AN_130 – FT2232H Used In FT245 Synchronous FIFO Mode](https://ftdichip.com/wp-content/uploads/2020/08/AN_130_FT2232H_Used_In_FT245-Synchronous-FIFO-Mode.pdf) (included in this repo)
- [D2XX Programmer's Guide](https://ftdichip.com/wp-content/uploads/2023/09/D2XX_Programmers_Guide1.pdf)
- [STM32H750 Reference Manual (RM0433)](https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
