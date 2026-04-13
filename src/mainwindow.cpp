#include "mainwindow.h"
#include "configmanager.h"
#include "audiodevicemanager.h"
#include "hotkeymanager.h"
#include "deviceentry.h"
#include "trayicon.h"
#include "thememanager.h"

#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QUuid>
#include <QMessageBox>
#include <QGroupBox>
#include <QFont>
#include <QApplication>
#include <QStyle>

MainWindow::MainWindow(ConfigManager *config, AudioDeviceManager *audio, HotkeyManager *hotkey,
                       QWidget *parent)
    : QMainWindow(parent),
      m_configManager(config),
      m_audioManager(audio),
      m_hotkeyManager(hotkey)
{
    // Load saved theme
    QString savedTheme = m_configManager->loadTheme();
    ThemeManager::instance().setTheme(savedTheme);

    setupUi();
    loadFromConfig();
    setWindowTitle("QuickSnapAudio");
    setMinimumSize(700, 450);
    resize(750, 500);

    applyCurrentTheme();
}

MainWindow::~MainWindow() {}

void MainWindow::setTrayIcon(TrayIcon *tray)
{
    m_trayIcon = tray;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Minimize to tray instead of closing
    hide();
    event->ignore();
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto *mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    auto *titleLabel = new QLabel("QuickSnapAudio", this);
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("Assign hotkeys to your audio devices for instant switching.", this);
    subtitleLabel->setObjectName("subtitleLabel");
    mainLayout->addWidget(subtitleLabel);

    // Theme selector row
    auto *themeRow = new QHBoxLayout();
    auto *themeLabel = new QLabel("Theme:", this);
    m_themeCombo = new QComboBox(this);
    m_themeCombo->setObjectName("themeCombo");
    m_themeCombo->setMinimumWidth(180);
    m_themeCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto themes = ThemeManager::instance().availableThemes();
    m_themeCombo->addItems(themes);
    m_themeCombo->setCurrentText(ThemeManager::instance().currentTheme());

    themeRow->addWidget(themeLabel);
    themeRow->addWidget(m_themeCombo);
    themeRow->addStretch();
    mainLayout->addLayout(themeRow);

    // Add device row
    auto *addRow = new QHBoxLayout();
    m_deviceCombo = new QComboBox(this);
    m_deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setFixedWidth(100);

    m_addBtn = new QPushButton("+ Add Device", this);
    m_addBtn->setFixedWidth(120);

    addRow->addWidget(m_deviceCombo);
    addRow->addWidget(m_refreshBtn);
    addRow->addWidget(m_addBtn);
    mainLayout->addLayout(addRow);

    // Table
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"Device Name", "Type", "Hotkey", "Device ID"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_table->horizontalHeader()->resizeSection(2, 200);
    m_table->setColumnHidden(3, true); // Hide device ID column
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table);

    // Bottom buttons
    auto *bottomRow = new QHBoxLayout();

    m_removeBtn = new QPushButton("Remove Selected", this);
    m_removeBtn->setObjectName("removeBtn");

    m_saveBtn = new QPushButton("Save && Apply", this);

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setObjectName("statusLabel");

    bottomRow->addWidget(m_removeBtn);
    bottomRow->addStretch();
    bottomRow->addWidget(m_statusLabel);
    bottomRow->addWidget(m_saveBtn);
    mainLayout->addLayout(bottomRow);

    // Connections
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddDevice);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveDevice);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshDevices);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onThemeChanged);

    // Initial device list refresh
    onRefreshDevices();
}

void MainWindow::onRefreshDevices()
{
    m_deviceCombo->clear();
    auto devices = m_audioManager->enumerateDevices();
    for (const auto &dev : devices) {
        QString label = QString("[%1] %2").arg(dev.isOutput ? "Output" : "Input").arg(dev.name);
        m_deviceCombo->addItem(label, QVariant::fromValue(QStringList({dev.id, dev.name, dev.isOutput ? "1" : "0"})));
    }
    m_statusLabel->setText(QString("Found %1 device(s)").arg(devices.size()));
}

void MainWindow::onAddDevice()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0) return;

    QStringList data = m_deviceCombo->currentData().toStringList();
    QString deviceId = data[0];
    QString deviceName = data[1];
    bool isOutput = data[2] == "1";

    int row = m_table->rowCount();
    m_table->insertRow(row);

    m_table->setItem(row, 0, new QTableWidgetItem(deviceName));
    m_table->setItem(row, 1, new QTableWidgetItem(isOutput ? "Output" : "Input"));

    // Add a key sequence editor for the hotkey column
    auto *hotkeyEdit = new QKeySequenceEdit(this);
    m_table->setCellWidget(row, 2, hotkeyEdit);

    m_table->setItem(row, 3, new QTableWidgetItem(deviceId));

    m_statusLabel->setText(QString("Added: %1").arg(deviceName));
}

void MainWindow::onRemoveDevice()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Remove", "Please select a row to remove.");
        return;
    }
    QString name = m_table->item(row, 0)->text();
    m_table->removeRow(row);
    m_statusLabel->setText(QString("Removed: %1").arg(name));
}

void MainWindow::onSave()
{
    auto entries = collectEntriesFromTable();
    m_configManager->saveEntries(entries);
    rebindAllHotkeys();
    m_statusLabel->setText("Saved & hotkeys applied!");
}

QVector<DeviceEntry> MainWindow::collectEntriesFromTable() const
{
    QVector<DeviceEntry> entries;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        DeviceEntry entry;
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.deviceName = m_table->item(row, 0)->text();
        entry.isOutput = (m_table->item(row, 1)->text() == "Output");

        auto *hotkeyEdit = qobject_cast<QKeySequenceEdit *>(m_table->cellWidget(row, 2));
        entry.hotkey = hotkeyEdit ? hotkeyEdit->keySequence().toString(QKeySequence::PortableText) : "";

        entry.deviceId = m_table->item(row, 3)->text();
        entries.append(entry);
    }
    return entries;
}

void MainWindow::loadFromConfig()
{
    auto entries = m_configManager->loadEntries();
    for (const auto &entry : entries) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(entry.deviceName));
        m_table->setItem(row, 1, new QTableWidgetItem(entry.isOutput ? "Output" : "Input"));

        auto *hotkeyEdit = new QKeySequenceEdit(QKeySequence(entry.hotkey), this);
        m_table->setCellWidget(row, 2, hotkeyEdit);

        m_table->setItem(row, 3, new QTableWidgetItem(entry.deviceId));
    }
}

void MainWindow::rebindAllHotkeys()
{
    m_hotkeyManager->unregisterAll();

    auto entries = m_configManager->loadEntries();
    for (const auto &entry : entries) {
        if (!entry.hotkey.isEmpty()) {
            m_hotkeyManager->registerHotkey(entry.id, entry.hotkey,
                [this, entry]() {
                    m_audioManager->setDefaultDevice(entry.deviceId, entry.isOutput);
                    if (m_trayIcon) {
                        m_trayIcon->showSwitchedNotification(entry.deviceName);
                    }
                });
        }
    }
}

void MainWindow::onThemeChanged(int index)
{
    Q_UNUSED(index);
    QString themeName = m_themeCombo->currentText();
    ThemeManager::instance().setTheme(themeName);
    m_configManager->saveTheme(themeName);
    applyCurrentTheme();
    m_statusLabel->setText(QString("Theme: %1").arg(themeName));
}

void MainWindow::applyCurrentTheme()
{
    setStyleSheet(ThemeManager::instance().styleSheet());

    // Update subtitle label color from theme
    auto *subtitleLabel = findChild<QLabel *>("subtitleLabel");
    if (subtitleLabel) {
        ThemeColors c = ThemeManager::instance().colors();
        subtitleLabel->setStyleSheet(QString("color: %1; font-size: 12px; margin-bottom: 8px;").arg(c.textSecondary));
    }
}
