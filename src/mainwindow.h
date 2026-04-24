#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QCloseEvent>
#include <QPoint>
#include <QSet>

class ConfigManager;
class AudioDeviceManager;
class HotkeyManager;
class BluetoothManager;
class TrayIcon;
struct DeviceEntry;
class QMouseEvent;

// A custom-painted window control button so the minimize / maximize / close
// glyphs share an identical visual language regardless of platform fonts.
class WindowControlButton : public QPushButton {
    Q_OBJECT
public:
    enum class Kind { Minimize, Maximize, Restore, Close };

    explicit WindowControlButton(Kind kind, QWidget *parent = nullptr);
    void setKind(Kind kind);
    void setHoverColor(const QColor &bg, const QColor &fg);
    void setBaseColor(const QColor &fg);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Kind m_kind;
    QColor m_hoverBg{0, 0, 0, 0};
    QColor m_hoverFg{255, 255, 255};
    QColor m_baseFg{220, 220, 220};
    bool m_hovered = false;
};

// Custom dark title bar embedded in this header to keep MOC simple.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
    void setTitle(const QString &title);
    void setMaximizedState(bool maximized);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    WindowControlButton *m_minBtn = nullptr;
    WindowControlButton *m_maxBtn = nullptr;
    WindowControlButton *m_closeBtn = nullptr;
    QPoint m_dragPos;
    bool m_dragging = false;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(ConfigManager *config, AudioDeviceManager *audio, HotkeyManager *hotkey,
               BluetoothManager *bluetooth, QWidget *parent = nullptr);
    ~MainWindow();

    void setTrayIcon(TrayIcon *tray);
    void applyCurrentTheme();

    // Apply hotkey bindings from current saved config
    void rebindAllHotkeys();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onAddDevice();
    void onRemoveDevice();
    void onSave();
    void onRefreshDevices();
    void onThemeChanged(int index);

    void onTitleBarMinimize();
    void onTitleBarMaximize();
    void onTitleBarClose();

    void onAutostartToggled(bool checked);

private:
    void setupUi();
    void loadFromConfig();
    QVector<DeviceEntry> collectEntriesFromTable() const;

    void handleHotkeyShortPress(const DeviceEntry &entry);
    void handleHotkeyLongPress(const DeviceEntry &entry);

    ConfigManager *m_configManager;
    AudioDeviceManager *m_audioManager;
    HotkeyManager *m_hotkeyManager;
    BluetoothManager *m_bluetoothManager;
    TrayIcon *m_trayIcon = nullptr;

    QWidget *m_centralWidget;
    QWidget *m_contentWidget;
    TitleBar *m_titleBar;
    QTableWidget *m_table;
    QComboBox *m_deviceCombo;
    QComboBox *m_themeCombo;
    QCheckBox *m_autostartCheck = nullptr;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_refreshBtn;
    QLabel *m_statusLabel;

    // Toggle-mute state: tracks the device most recently activated by hotkey
    QString m_lastActivatedDeviceId;
    bool m_lastActivatedIsOutput = true;

    // Track devices the user disconnected via long-press, so a second
    // long-press reconnects them.
    QSet<QString> m_disconnectedDevices;
};

#endif // MAINWINDOW_H
