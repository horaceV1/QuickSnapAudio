#include "mainwindow.h"
#include "configmanager.h"
#include "audiodevicemanager.h"
#include "hotkeymanager.h"
#include "deviceentry.h"

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
    setupUi();
    loadFromConfig();
    setWindowTitle("QuickSnapAudio");
    setMinimumSize(700, 450);
    resize(750, 500);

    // Apply a clean modern style
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1e1e2e;
        }
        QWidget {
            color: #cdd6f4;
            font-size: 13px;
        }
        QTableWidget {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 6px;
            gridline-color: #45475a;
            selection-background-color: #585b70;
        }
        QTableWidget::item {
            padding: 6px;
        }
        QHeaderView::section {
            background-color: #45475a;
            color: #cdd6f4;
            padding: 8px;
            border: none;
            font-weight: bold;
        }
        QPushButton {
            background-color: #89b4fa;
            color: #1e1e2e;
            border: none;
            border-radius: 6px;
            padding: 8px 18px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #74c7ec;
        }
        QPushButton:pressed {
            background-color: #89dceb;
        }
        QPushButton#removeBtn {
            background-color: #f38ba8;
        }
        QPushButton#removeBtn:hover {
            background-color: #eba0ac;
        }
        QComboBox {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 6px;
            padding: 6px 12px;
            min-width: 250px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: #313244;
            border: 1px solid #45475a;
            selection-background-color: #585b70;
        }
        QLabel#statusLabel {
            color: #a6e3a1;
            font-size: 12px;
        }
        QLabel#titleLabel {
            font-size: 20px;
            font-weight: bold;
            color: #cba6f7;
        }
        QGroupBox {
            border: 1px solid #45475a;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 16px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
        }
        QKeySequenceEdit {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 4px;
            color: #f9e2af;
        }
    )");
}

MainWindow::~MainWindow() {}

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
    auto *titleLabel = new QLabel("⚡ QuickSnapAudio", this);
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("Assign hotkeys to your audio devices for instant switching.", this);
    subtitleLabel->setStyleSheet("color: #a6adc8; font-size: 12px; margin-bottom: 8px;");
    mainLayout->addWidget(subtitleLabel);

    // Add device row
    auto *addRow = new QHBoxLayout();
    m_deviceCombo = new QComboBox(this);
    m_deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_refreshBtn = new QPushButton("↻ Refresh", this);
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
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->setColumnHidden(3, true); // Hide device ID column
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table);

    // Bottom buttons
    auto *bottomRow = new QHBoxLayout();

    m_removeBtn = new QPushButton("Remove Selected", this);
    m_removeBtn->setObjectName("removeBtn");

    m_saveBtn = new QPushButton("💾 Save & Apply", this);

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
    hotkeyEdit->setStyleSheet("background-color: #313244; border: 1px solid #45475a; border-radius: 4px; padding: 4px; color: #f9e2af;");
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
        hotkeyEdit->setStyleSheet("background-color: #313244; border: 1px solid #45475a; border-radius: 4px; padding: 4px; color: #f9e2af;");
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
                });
        }
    }
}
