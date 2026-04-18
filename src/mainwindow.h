#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QCloseEvent>
#include <QPoint>

class ConfigManager;
class AudioDeviceManager;
class HotkeyManager;
class TrayIcon;
struct DeviceEntry;
class QMouseEvent;

// Custom dark title bar embedded in this header to keep MOC simple.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
    void setTitle(const QString &title);

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
    QPushButton *m_minBtn = nullptr;
    QPushButton *m_maxBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QPoint m_dragPos;
    bool m_dragging = false;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(ConfigManager *config, AudioDeviceManager *audio, HotkeyManager *hotkey,
               QWidget *parent = nullptr);
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

private:
    void setupUi();
    void loadFromConfig();
    QVector<DeviceEntry> collectEntriesFromTable() const;

    void handleHotkeyActivated(const DeviceEntry &entry);

    ConfigManager *m_configManager;
    AudioDeviceManager *m_audioManager;
    HotkeyManager *m_hotkeyManager;
    TrayIcon *m_trayIcon = nullptr;

    QWidget *m_centralWidget;
    QWidget *m_contentWidget;
    TitleBar *m_titleBar;
    QTableWidget *m_table;
    QComboBox *m_deviceCombo;
    QComboBox *m_themeCombo;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_refreshBtn;
    QLabel *m_statusLabel;

    // Toggle-mute state: tracks the device most recently activated by hotkey
    QString m_lastActivatedDeviceId;
    bool m_lastActivatedIsOutput = true;
};

#endif // MAINWINDOW_H
