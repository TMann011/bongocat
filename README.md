# Bongo Cat - Memory Trainer & Reverse Engineering Suite

High-performance native C++ memory trainer, runtime introspection engine, and complete architectural reverse engineering suite for Bongo Cat.

---

## Features

- **Automated Memory Patching & Byte Injection**: Direct process memory manipulation via Win32 Virtual Memory APIs.
- **PawPass Season Max Unlocker**: Unlocks all battle pass tiers and cosmetics.
- **Auto-Clicker & Tap Acceleration**: High-speed input generation with configurable delay.
- **Item Exchange & Trade-up Automator**: Automates duplicate item recycling and inventory rewards.
- **Interactive Console UI**: Live status display, process attachment monitoring, and hotkey listeners.

---

## Build Instructions

### Prerequisites
- Windows 10 / 11 (x64)
- Visual Studio 2019 / 2022 / v18 or MSVC C++ Build Tools

### Compilation
Run the portable build script:
```cmd
build.bat
```
The script auto-detects your compiler environment and generates `BongoTrainer.exe`.

---

## Repository Contents

- `trainer.cpp`: Complete C++ source code for the memory trainer.
- `BongoTrainer.ini`: Configuration file for delay timings and hotkey bindings.
- `BongoTrainer.exe`: Pre-compiled 64-bit release binary.
- `TECHNICAL_NOTES_AND_HANDOVER.md`: Exhaustive architectural analysis across 578 decompiled game classes.
- `decomp_references.zip`: Complete reverse engineered C# source references from ILSpy/dnSpy.
