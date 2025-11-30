# QR204 ESC/POS Command Cheat Sheet
Based on the QR204 Thermal Control Unit User Manual.

---

## 1. Movement, Line Feed & Positioning

| Command | Hex | Args | Description |
|--------|-----|------|-------------|
| **LF** | `0A` | — | Print buffer + line feed |
| **CR** | `0D` | — | Carriage return (same as LF in serial) |
| **HT** | `09` | — | Horizontal tab → next TAB stop |
| **ESC D** | `1B 44` | `n1…nk 00` | Set TAB stops |
| **ESC J** | `1B 4A` | `n` | Print + feed `n × 0.125 mm` |
| **ESC d** | `1B 64` | `n` | Print + feed `n` lines |
| **ESC $** | `1B 24` | `nL nH` | Set absolute print position |
| **GS L** | `1D 4C` | `nL nH` | Set left margin |

---

## 2. Line Spacing

| Command | Hex | Args | Description |
|--------|-----|------|-------------|
| **ESC 2** | `1B 32` | — | Default line spacing |
| **ESC 3** | `1B 33` | `n` | Set spacing (`n × 0.125 mm`) |

---

## 3. Alignment

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **ESC a** | `1B 61` | `0/1/2` | Left / Center / Right |

---

## 4. Character Formatting

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **ESC !** | `1B 21` | `n` | Style bitmask (bold, width, height, invert…) |
| **GS !** | `1D 21` | `n` | Height/width multipliers (1–8×) |
| **ESC E** | `1B 45` | `n` | Bold on/off |
| **ESC G** | `1B 47` | `n` | Double-strike |
| **ESC –** | `1B 2D` | `n` | Underline off / 1-dot / 2-dot |
| **ESC SP** | `1B 20` | `n` | Right character spacing |
| **ESC {** | `1B 7B` | `n` | Upside-down printing |
| **ESC V** | `1B 56` | `n` | 90° rotated text |
| **GS B** | `1D 42` | `n` | Invert printing |
| **ESC SO** | `0E` | — | Double-width ON |
| **ESC DC4** | `14` | — | Double-width OFF |

---

## 5. International & Code Pages

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **ESC R** | `1B 52` | `n` | International charset |
| **ESC t** | `1B 74` | `n` | Select code page |

---

## 6. User-Defined Characters

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **ESC %** | `1B 25` | `n` | Enable/disable user-defined glyphs |
| **ESC &** | `1B 26` | `y c1 c2 …` | Define glyph(s) (font cell sized) |
| **ESC ?** | `1B 3F` | `n` | Delete user-defined character |

---

## 7. Bitmaps & Raster Graphics

### Standard Raster Graphics

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **ESC \*** | `1B 2A` | `m nL nH data` | Print raster bitmap (8/24-dot) |
| **GS \*** | `1D 2A` | `x y data` | Define downloaded bitmap |
| **GS /** | `1D 2F` | `m` | Print downloaded bitmap |
| **GS v 0** | `1D 76 30` | `m xL xH yL yH data` | Print bitmap (arbitrary size) |

### NV (Non-Volatile) Bitmaps

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **FS q** | `1C 71` | `n [bitmaps...]` | Store NV bitmap(s) |
| **FS p** | `1C 70` | `n m` | Print NV bitmap |

---

## 8. Initialization & Hardware Control

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **ESC @** | `1B 40` | — | Soft reset / initialize |
| **ESC 7** | `1B 37` | `n1 n2 n3` | Control parameters |
| **ESC 8** | `1B 38` | `n1 n2` | Sleep settings |
| **DC2 T** | `12 54` | — | Print self-test page |
| **ESC c 5 n** | `1B 63 35 n` | Enable/disable panel buttons |

---

## 9. Status & Sensors

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **ESC v** | `1B 76` | `n` | Query basic status |
| **GS r** | `1D 72` | `n` | Request sensor state |
| **GS a** | `1D 61` | `n` | Auto-status-back (ASB) config |

---

## 10. Barcodes (1D)

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **GS k** | `1D 6B` | `m …` | Print barcode (UPC/EAN/ITF/CODE39/C93/C128) |
| **GS h** | `1D 68` | `n` | Barcode height |
| **GS w** | `1D 77` | `n` | Module width |
| **GS H** | `1D 48` | `n` | HRI text position |
| **GS x** | `1D 78` | `n` | Barcode left margin |

---

## 11. 2D Codes (QR, PDF417)

All 2D code commands use:

GS ( k pL pH cn fn …)


### QR Code Sub-Functions

| `fn` | Purpose |
|------|---------|
| **65** | Select QR model |
| **67** | Module size |
| **69** | Error correction level |
| **80** | Store QR data |
| **81** | Print PDF417 |
| **82** | Select QR data type |

---

## 12. Chinese / Double-Byte Mode

| Command | Hex | Args | Description |
|--------|------|------|-------------|
| **FS &** | `1C 26` | — | Enter Chinese mode |
| **FS .** | `1C 2E` | — | Exit Chinese mode |
| **FS !** | `1C 21` | `n` | Chinese font mode |

---

## 13. Control Characters (Reference)

| Char | Hex | Meaning |
|------|------|---------|
| NUL | `00` | Terminator |
| SOH | `01` | Start of heading |
| STX | `02` | Start text |
| ETX | `03` | End text |
| EOT | `04` | End transmission |
| ENQ | `05` | Status request |
| ACK | `06` | Acknowledge |
| BEL | `07` | Bell (if supported) |
| BS | `08` | Backspace |
| FF | `0C` | Form feed (usually ignored) |
| SO | `0E` | Double-width ON |
| SI | `0F` | Double-width OFF |

---

## 14. Notes for Implementers

- Always explicitly set code page using `ESC t`.
- Serial flow control: DTR/RTS may be used for BUSY state.
- NV memory writes trigger a hardware reset — avoid frequent writes.
- Custom glyphs are cleared on reset (unless stored as NV bitmap).
- ESC `*` uses modes: `0/1 = 8-dot`, `32/33 = 24-dot`.

---

