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

class ConfigManager;
class AudioDeviceManager;
class HotkeyManager;
class TrayIcon;
struct DeviceEntry;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(ConfigManager *config, AudioDeviceManager *audio, HotkeyManager *hotkey,
               QWidget *parent = nullptr);
    ~MainWindow();

    void setTrayIcon(TrayIcon *tray);
    void applyCurrentTheme();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onAddDevice();
    void onRemoveDevice();
    void onSave();
    void onRefreshDevices();
    void onThemeChanged(int index);

private:
    void setupUi();
    void loadFromConfig();
    void rebindAllHotkeys();
    QVector<DeviceEntry> collectEntriesFromTable() const;

    ConfigManager *m_configManager;
    AudioDeviceManager *m_audioManager;
    HotkeyManager *m_hotkeyManager;
    TrayIcon *m_trayIcon = nullptr;

    QWidget *m_centralWidget;
    QTableWidget *m_table;
    QComboBox *m_deviceCombo;
    QComboBox *m_themeCombo;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_refreshBtn;
    QLabel *m_statusLabel;
};

#endif // MAINWINDOW_H
