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
#include <QSizeGrip>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QTimer>
#include <QMouseEvent>
#include <QPixmap>

// ============================================================================
// Custom dark title bar implementation
// ============================================================================
static const char *kTitleBarStyle = R"(
QWidget#TitleBarRoot {
    background-color: #15171c;
}
QLabel#TitleBarTitle {
    color: #e6e8ee;
    font-size: 13px;
    font-weight: 600;
    padding-left: 6px;
}
QPushButton#TitleBarBtn {
    background-color: transparent;
    color: #c9ccd3;
    border: none;
    font-size: 14px;
    font-weight: bold;
    min-width: 44px;
    min-height: 32px;
    max-height: 32px;
}
QPushButton#TitleBarBtn:hover {
    background-color: #2a2d36;
    color: #ffffff;
}
QPushButton#TitleBarBtn:pressed {
    background-color: #3a3e49;
}
QPushButton#TitleBarCloseBtn {
    background-color: transparent;
    color: #c9ccd3;
    border: none;
    font-size: 14px;
    font-weight: bold;
    min-width: 44px;
    min-height: 32px;
    max-height: 32px;
}
QPushButton#TitleBarCloseBtn:hover {
    background-color: #e81123;
    color: #ffffff;
}
QPushButton#TitleBarCloseBtn:pressed {
    background-color: #c10f1f;
}
)";

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("TitleBarRoot");
    setFixedHeight(36);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(kTitleBarStyle);

    m_iconLabel = new QLabel(this);
    QPixmap icon(":/icons/tray.png");
    if (!icon.isNull()) {
        m_iconLabel->setPixmap(icon.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel("QuickSnapAudio", this);
    m_titleLabel->setObjectName("TitleBarTitle");

    m_minBtn = new QPushButton(QString::fromUtf8("\u2014"), this);
    m_minBtn->setObjectName("TitleBarBtn");
    m_minBtn->setFocusPolicy(Qt::NoFocus);

    m_maxBtn = new QPushButton(QString::fromUtf8("\u25A1"), this);
    m_maxBtn->setObjectName("TitleBarBtn");
    m_maxBtn->setFocusPolicy(Qt::NoFocus);

    m_closeBtn = new QPushButton(QString::fromUtf8("\u2715"), this);
    m_closeBtn->setObjectName("TitleBarCloseBtn");
    m_closeBtn->setFocusPolicy(Qt::NoFocus);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_minBtn);
    layout->addWidget(m_maxBtn);
    layout->addWidget(m_closeBtn);

    connect(m_minBtn, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(m_maxBtn, &QPushButton::clicked, this, &TitleBar::maximizeClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        if (window()->isMaximized()) {
            window()->showNormal();
        }
        window()->move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeClicked();
        event->accept();
    }
}

void TitleBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), QColor("#15171c"));
    p.setPen(QColor(0, 0, 0, 80));
    p.drawLine(rect().left(), rect().bottom(), rect().right(), rect().bottom());
}

// ============================================================================
// MainWindow
// ============================================================================

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

    // Frameless window with custom dark title bar
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setupUi();
    loadFromConfig();
    setWindowTitle("QuickSnapAudio");
    setMinimumSize(720, 480);
    resize(780, 540);

    applyCurrentTheme();

    // Register hotkeys at startup so they work without opening the window first
    rebindAllHotkeys();
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

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (windowState() & Qt::WindowMinimized) {
            // Hide to tray on minimize
            QTimer::singleShot(0, this, [this]() {
                setWindowState(windowState() & ~Qt::WindowMinimized);
                hide();
            });
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // Paint dark rounded background outside content (subtle border around frameless window)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#0f1115"));
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto *outer = new QVBoxLayout(m_centralWidget);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Custom dark title bar
    m_titleBar = new TitleBar(m_centralWidget);
    m_titleBar->setTitle("QuickSnapAudio");
    outer->addWidget(m_titleBar);

    connect(m_titleBar, &TitleBar::minimizeClicked, this, &MainWindow::onTitleBarMinimize);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, &MainWindow::onTitleBarMaximize);
    connect(m_titleBar, &TitleBar::closeClicked, this, &MainWindow::onTitleBarClose);

    // Inner content widget so the theme stylesheet only affects content area
    m_contentWidget = new QWidget(m_centralWidget);
    m_contentWidget->setObjectName("contentRoot");
    outer->addWidget(m_contentWidget, 1);

    auto *mainLayout = new QVBoxLayout(m_contentWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 16, 20, 16);

    // Title
    auto *titleLabel = new QLabel("QuickSnapAudio", m_contentWidget);
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("Assign hotkeys to your audio devices for instant switching. Press the same hotkey twice to toggle mute.", m_contentWidget);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setWordWrap(true);
    mainLayout->addWidget(subtitleLabel);

    // Theme selector row
    auto *themeRow = new QHBoxLayout();
    auto *themeLabel = new QLabel("Theme:", m_contentWidget);
    m_themeCombo = new QComboBox(m_contentWidget);
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
    m_deviceCombo = new QComboBox(m_contentWidget);
    m_deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_refreshBtn = new QPushButton("Refresh", m_contentWidget);
    m_refreshBtn->setFixedWidth(100);

    m_addBtn = new QPushButton("+ Add Device", m_contentWidget);
    m_addBtn->setFixedWidth(120);

    addRow->addWidget(m_deviceCombo);
    addRow->addWidget(m_refreshBtn);
    addRow->addWidget(m_addBtn);
    mainLayout->addLayout(addRow);

    // Table
    m_table = new QTableWidget(0, 4, m_contentWidget);
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

    m_removeBtn = new QPushButton("Remove Selected", m_contentWidget);
    m_removeBtn->setObjectName("removeBtn");

    m_saveBtn = new QPushButton("Save && Apply", m_contentWidget);

    m_statusLabel = new QLabel("", m_contentWidget);
    m_statusLabel->setObjectName("statusLabel");

    bottomRow->addWidget(m_removeBtn);
    bottomRow->addStretch();
    bottomRow->addWidget(m_statusLabel);
    bottomRow->addWidget(m_saveBtn);

    // Size grip in corner so frameless window can still be resized
    auto *grip = new QSizeGrip(m_contentWidget);
    grip->setFixedSize(16, 16);
    bottomRow->addWidget(grip, 0, Qt::AlignBottom | Qt::AlignRight);

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

void MainWindow::onTitleBarMinimize()
{
    // Hide directly to tray
    hide();
}

void MainWindow::onTitleBarMaximize()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
}

void MainWindow::onTitleBarClose()
{
    hide();
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
                [this, entry]() { handleHotkeyActivated(entry); });
        }
    }
}

void MainWindow::handleHotkeyActivated(const DeviceEntry &entry)
{
    // Toggle-mute behavior:
    //  - Pressing the hotkey for the currently-active device toggles mute on/off
    //  - Pressing a hotkey for a different device switches to it (and unmutes it)
    if (m_lastActivatedDeviceId == entry.deviceId &&
        m_lastActivatedIsOutput == entry.isOutput)
    {
        bool newMuted = false;
        if (m_audioManager->toggleMute(entry.deviceId, entry.isOutput, &newMuted)) {
            if (m_trayIcon) {
                m_trayIcon->showSwitchedNotification(
                    QString("%1 %2").arg(entry.deviceName,
                                         newMuted ? "muted" : "unmuted"));
            }
        }
        return;
    }

    // Switch to this device, ensure unmuted
    m_audioManager->setDefaultDevice(entry.deviceId, entry.isOutput);
    m_audioManager->setMute(entry.deviceId, entry.isOutput, false);
    m_lastActivatedDeviceId = entry.deviceId;
    m_lastActivatedIsOutput = entry.isOutput;

    if (m_trayIcon) {
        m_trayIcon->showSwitchedNotification(entry.deviceName);
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
    // Apply theme stylesheet only to the content area, leaving title bar dark
    if (m_contentWidget) {
        m_contentWidget->setStyleSheet(ThemeManager::instance().styleSheet());
    }

    // Update subtitle label color from theme
    auto *subtitleLabel = findChild<QLabel *>("subtitleLabel");
    if (subtitleLabel) {
        ThemeColors c = ThemeManager::instance().colors();
        subtitleLabel->setStyleSheet(QString("color: %1; font-size: 12px; margin-bottom: 8px;").arg(c.textSecondary));
    }
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
