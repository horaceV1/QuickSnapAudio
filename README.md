# ⚡ QuickSnapAudio

A lightweight, cross-platform audio device switcher with global hotkey support for **Windows** and **Linux**.

Assign keyboard shortcuts to your audio devices and switch between them instantly — no more clicking through system settings!

![License](https://img.shields.io/github/license/your-username/QuickSnapAudio)
![Release](https://img.shields.io/github/v/release/your-username/QuickSnapAudio)

---

## ✨ Features

- 🎧 **List all audio input/output devices** on your system
- ⌨️ **Assign global hotkeys** to any audio device
- 🔄 **Instantly switch** your default audio device with a keypress
- 🖥️ **System tray** — runs quietly in the background
- 💾 **Persistent config** — your settings are saved automatically
- 🐧🪟 **Cross-platform** — works on Windows and Linux

## 📦 Installation

### Windows
1. Download `QuickSnapAudio-Setup.exe` from the [Releases](../../releases) page
2. Run the installer
3. Launch from the Start Menu or Desktop shortcut

### Linux (Debian/Ubuntu)
```bash
# Download the .deb from Releases, then:
sudo dpkg -i quicksnapaudio_*.deb
```

### Build from Source

**Prerequisites:**
- CMake 3.16+
- Qt 6.x (Widgets, Core, Gui, Multimedia)
- C++17 compiler

**Windows (MSVC):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

**Linux:**
```bash
sudo apt install build-essential cmake libpulse-dev libx11-dev libxtst-dev qt6-base-dev qt6-multimedia-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## 🚀 Usage

1. **Launch** QuickSnapAudio — it appears in your system tray
2. **Open Settings** by double-clicking the tray icon
3. **Select a device** from the dropdown and click **"+ Add Device"**
4. **Click the Hotkey field** and press your desired key combination (e.g., `Ctrl+Shift+1`)
5. Click **"💾 Save & Apply"**
6. Press your hotkey anywhere to instantly switch audio devices!

## 🏗️ Architecture

```
src/
├── main.cpp              # Application entry point
├── mainwindow.cpp/h      # Settings UI
├── trayicon.cpp/h        # System tray integration
├── audiodevicemanager.*   # Cross-platform audio device API
├── hotkeymanager.*        # Cross-platform global hotkey API
├── configmanager.*        # JSON config persistence
├── deviceentry.h          # Data model
└── platform/
    ├── windows_audio.*    # Windows MMDevice/PolicyConfig
    ├── windows_hotkey.*   # Windows RegisterHotKey
    ├── linux_audio.*      # PulseAudio API
    └── linux_hotkey.*     # X11 XGrabKey
```

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.
