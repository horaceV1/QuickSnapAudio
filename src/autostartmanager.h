#ifndef AUTOSTARTMANAGER_H
#define AUTOSTARTMANAGER_H

#include <QString>

// Cross-platform "run at system startup" helper.
//   - Windows: HKCU\Software\Microsoft\Windows\CurrentVersion\Run
//   - Linux:   ~/.config/autostart/QuickSnapAudio.desktop
// All operations are best-effort and return whether the requested state is
// now in effect.
namespace AutostartManager {
    bool isEnabled();
    bool setEnabled(bool enabled, QString *errorOut = nullptr);
}

#endif // AUTOSTARTMANAGER_H
