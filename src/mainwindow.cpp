#include "mainwindow.h"
#include "configmanager.h"
#include "audiodevicemanager.h"
#include "hotkeymanager.h"
#include "bluetoothmanager.h"
#include "autostartmanager.h"
#include "deviceentry.h"
#include "trayicon.h"
#include "thememanager.h"

#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QCheckBox>
#include <QUuid>
#include <QMessageBox>
#include <QGroupBox>
#include <QFont>
#include <QApplication>
#include <QStyle>
#include <QSizeGrip>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QEvent>
#include <QTimer>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QPixmap>

// ============================================================================
// WindowControlButton — custom-painted minimize / maximize / close button
// ============================================================================

WindowControlButton::WindowControlButton(Kind kind, QWidget *parent)
    : QPushButton(parent), m_kind(kind)
{
    setFlat(true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
    setFixedSize(46, 32);
    setAttribute(Qt::WA_Hover, true);
}

void WindowControlButton::setKind(Kind kind)
{
    m_kind = kind;
    update();
}

void WindowControlButton::setHoverColor(const QColor &bg, const QColor &fg)
{
    m_hoverBg = bg;
    m_hoverFg = fg;
    update();
}

void WindowControlButton::setBaseColor(const QColor &fg)
{
    m_baseFg = fg;
    update();
}

void WindowControlButton::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void WindowControlButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

void WindowControlButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (m_hovered && m_hoverBg.alpha() > 0) {
        p.fillRect(rect(), m_hoverBg);
    }

    QColor stroke = m_hovered ? m_hoverFg : m_baseFg;
    QPen pen(stroke, 1.2);
    pen.setCapStyle(Qt::FlatCap);
    p.setPen(pen);

    // 10x10 glyph centered in the button
    const int s = 10;
    const QPoint c = rect().center();
    const QRect glyph(c.x() - s / 2, c.y() - s / 2, s, s);

    switch (m_kind) {
    case Kind::Minimize: {
        // Single horizontal line near vertical center
        p.drawLine(glyph.left(), c.y(), glyph.right(), c.y());
        break;
    }
    case Kind::Maximize: {
        // Hollow square
        p.drawRect(glyph.adjusted(0, 0, -1, -1));
        break;
    }
    case Kind::Restore: {
        // Two stacked squares
        QRect back(glyph.left() + 2, glyph.top(), s - 3, s - 3);
        QRect front(glyph.left(), glyph.top() + 2, s - 3, s - 3);
        p.drawRect(back);
        p.fillRect(front.adjusted(1, 1, 0, 0),
                   m_hovered ? m_hoverBg : palette().color(QPalette::Window));
        p.drawRect(front);
        break;
    }
    case Kind::Close: {
        // X
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen xpen(stroke, 1.4);
        p.setPen(xpen);
        p.drawLine(glyph.topLeft(), glyph.bottomRight());
        p.drawLine(glyph.topRight(), glyph.bottomLeft());
        break;
    }
    }
}

// ============================================================================
// TitleBar
// ============================================================================

namespace {
constexpr const char *kTitleBarBg = "#0f1218";
constexpr const char *kTitleBarText = "#e6e8ee";
constexpr const char *kTitleBarHover = "#262a35";
constexpr const char *kTitleBarBorder = "#1d2129";
}

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("TitleBarRoot");
    setFixedHeight(36);
    setAttribute(Qt::WA_StyledBackground, true);

    m_iconLabel = new QLabel(this);
    QPixmap icon(":/icons/tray.png");
    if (!icon.isNull()) {
        m_iconLabel->setPixmap(icon.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_iconLabel->setFixedSize(20, 20);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_titleLabel = new QLabel("QuickSnapAudio", this);
    m_titleLabel->setObjectName("TitleBarTitle");
    QFont tf = m_titleLabel->font();
    tf.setPointSizeF(tf.pointSizeF() > 0 ? tf.pointSizeF() : 9.0);
    tf.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(tf);
    m_titleLabel->setStyleSheet(QString("color: %1; padding-left: 8px; letter-spacing: 0.5px;").arg(kTitleBarText));

    const QColor hoverBg(kTitleBarHover);
    const QColor base(kTitleBarText);
    const QColor neutralFg(255, 255, 255);

    m_minBtn = new WindowControlButton(WindowControlButton::Kind::Minimize, this);
    m_minBtn->setBaseColor(base);
    m_minBtn->setHoverColor(hoverBg, neutralFg);

    m_maxBtn = new WindowControlButton(WindowControlButton::Kind::Maximize, this);
    m_maxBtn->setBaseColor(base);
    m_maxBtn->setHoverColor(hoverBg, neutralFg);

    m_closeBtn = new WindowControlButton(WindowControlButton::Kind::Close, this);
    m_closeBtn->setBaseColor(base);
    m_closeBtn->setHoverColor(QColor("#e81123"), QColor(255, 255, 255));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
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

void TitleBar::setMaximizedState(bool maximized)
{
    m_maxBtn->setKind(maximized ? WindowControlButton::Kind::Restore
                                : WindowControlButton::Kind::Maximize);
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

void TitleBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(kTitleBarBg));

    // Subtle 1-px bottom border
    p.setPen(QColor(kTitleBarBorder));
    p.drawLine(rect().left(), rect().bottom(), rect().right(), rect().bottom());
}

// ============================================================================
// MainWindow
// ============================================================================

MainWindow::MainWindow(ConfigManager *config, AudioDeviceManager *audio, HotkeyManager *hotkey,
                       BluetoothManager *bluetooth, QWidget *parent)
    : QMainWindow(parent),
      m_configManager(config),
      m_audioManager(audio),
      m_hotkeyManager(hotkey),
      m_bluetoothManager(bluetooth)
{
    QString savedTheme = m_configManager->loadTheme();
    ThemeManager::instance().setTheme(savedTheme);

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setupUi();
    loadFromConfig();
    setWindowTitle("QuickSnapAudio");
    setMinimumSize(760, 520);
    resize(820, 580);

    applyCurrentTheme();
    rebindAllHotkeys();
}

MainWindow::~MainWindow() = default;

void MainWindow::setTrayIcon(TrayIcon *tray)
{
    m_trayIcon = tray;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (windowState() & Qt::WindowMinimized) {
            QTimer::singleShot(0, this, [this]() {
                setWindowState(windowState() & ~Qt::WindowMinimized);
                hide();
            });
        }
        if (m_titleBar)
            m_titleBar->setMaximizedState(isMaximized());
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#0b0d12"));
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto *outer = new QVBoxLayout(m_centralWidget);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_titleBar = new TitleBar(m_centralWidget);
    m_titleBar->setTitle("QuickSnapAudio");
    outer->addWidget(m_titleBar);

    connect(m_titleBar, &TitleBar::minimizeClicked, this, &MainWindow::onTitleBarMinimize);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, &MainWindow::onTitleBarMaximize);
    connect(m_titleBar, &TitleBar::closeClicked, this, &MainWindow::onTitleBarClose);

    m_contentWidget = new QWidget(m_centralWidget);
    m_contentWidget->setObjectName("contentRoot");
    outer->addWidget(m_contentWidget, 1);

    auto *mainLayout = new QVBoxLayout(m_contentWidget);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(24, 20, 24, 18);

    // Header: title + accent line
    auto *titleLabel = new QLabel("QuickSnapAudio", m_contentWidget);
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel(
        "Bind hotkeys to audio devices for instant switching. Tap to toggle mute. "
        "Hold for 1.2s to disconnect or reconnect a wireless / Bluetooth device.",
        m_contentWidget);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setWordWrap(true);
    mainLayout->addWidget(subtitleLabel);

    auto *accentLine = new QFrame(m_contentWidget);
    accentLine->setObjectName("accentLine");
    accentLine->setFrameShape(QFrame::HLine);
    accentLine->setFixedHeight(1);
    mainLayout->addWidget(accentLine);

    // Theme + add device row in a card
    auto *toolbarCard = new QFrame(m_contentWidget);
    toolbarCard->setObjectName("card");
    auto *toolbarLayout = new QVBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(14, 12, 14, 12);
    toolbarLayout->setSpacing(10);

    auto *themeRow = new QHBoxLayout();
    themeRow->setSpacing(10);
    auto *themeLabel = new QLabel("Theme", toolbarCard);
    themeLabel->setObjectName("fieldLabel");
    m_themeCombo = new QComboBox(toolbarCard);
    m_themeCombo->setObjectName("themeCombo");
    m_themeCombo->setMinimumWidth(200);
    m_themeCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto themes = ThemeManager::instance().availableThemes();
    m_themeCombo->addItems(themes);
    m_themeCombo->setCurrentText(ThemeManager::instance().currentTheme());

    themeRow->addWidget(themeLabel);
    themeRow->addWidget(m_themeCombo);
    themeRow->addStretch();

    m_autostartCheck = new QCheckBox("Launch at system startup", toolbarCard);
    m_autostartCheck->setObjectName("autostartCheck");
    m_autostartCheck->setChecked(AutostartManager::isEnabled());
    themeRow->addWidget(m_autostartCheck);

    toolbarLayout->addLayout(themeRow);

    auto *addRow = new QHBoxLayout();
    addRow->setSpacing(10);
    auto *deviceLabel = new QLabel("Device", toolbarCard);
    deviceLabel->setObjectName("fieldLabel");
    m_deviceCombo = new QComboBox(toolbarCard);
    m_deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_refreshBtn = new QPushButton("Refresh", toolbarCard);
    m_refreshBtn->setObjectName("secondaryBtn");
    m_refreshBtn->setFixedWidth(100);

    m_addBtn = new QPushButton("Add Device", toolbarCard);
    m_addBtn->setObjectName("primaryBtn");
    m_addBtn->setFixedWidth(120);

    addRow->addWidget(deviceLabel);
    addRow->addWidget(m_deviceCombo);
    addRow->addWidget(m_refreshBtn);
    addRow->addWidget(m_addBtn);
    toolbarLayout->addLayout(addRow);

    mainLayout->addWidget(toolbarCard);

    // Table card
    auto *tableCard = new QFrame(m_contentWidget);
    tableCard->setObjectName("card");
    auto *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    m_table = new QTableWidget(0, 4, tableCard);
    m_table->setHorizontalHeaderLabels({"Device Name", "Type", "Hotkey", "Device ID"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_table->horizontalHeader()->resizeSection(2, 200);
    m_table->setColumnHidden(3, true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(42);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setFrameShape(QFrame::NoFrame);
    tableLayout->addWidget(m_table);

    mainLayout->addWidget(tableCard, 1);

    // Bottom row
    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(10);

    m_removeBtn = new QPushButton("Remove Selected", m_contentWidget);
    m_removeBtn->setObjectName("dangerBtn");

    m_saveBtn = new QPushButton("Save && Apply", m_contentWidget);
    m_saveBtn->setObjectName("primaryBtn");

    m_statusLabel = new QLabel("", m_contentWidget);
    m_statusLabel->setObjectName("statusLabel");

    bottomRow->addWidget(m_removeBtn);
    bottomRow->addStretch();
    bottomRow->addWidget(m_statusLabel);
    bottomRow->addWidget(m_saveBtn);

    auto *grip = new QSizeGrip(m_contentWidget);
    grip->setFixedSize(16, 16);
    bottomRow->addWidget(grip, 0, Qt::AlignBottom | Qt::AlignRight);

    mainLayout->addLayout(bottomRow);

    connect(m_addBtn,    &QPushButton::clicked, this, &MainWindow::onAddDevice);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveDevice);
    connect(m_saveBtn,   &QPushButton::clicked, this, &MainWindow::onSave);
    connect(m_refreshBtn,&QPushButton::clicked, this, &MainWindow::onRefreshDevices);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onThemeChanged);
    connect(m_autostartCheck, &QCheckBox::toggled,
            this, &MainWindow::onAutostartToggled);

    onRefreshDevices();
}

void MainWindow::onTitleBarMinimize() { hide(); }

void MainWindow::onTitleBarMaximize()
{
    if (isMaximized()) showNormal();
    else               showMaximized();
    if (m_titleBar) m_titleBar->setMaximizedState(isMaximized());
}

void MainWindow::onTitleBarClose() { hide(); }

void MainWindow::onRefreshDevices()
{
    m_deviceCombo->clear();
    auto devices = m_audioManager->enumerateDevices();
    for (const auto &dev : devices) {
        QString tag = dev.isOutput ? "Output" : "Input";
        if (BluetoothManager::looksLikeBluetooth(dev.name))
            tag += " · BT";
        QString label = QString("[%1] %2").arg(tag, dev.name);
        m_deviceCombo->addItem(label, QVariant::fromValue(QStringList({dev.id, dev.name, dev.isOutput ? "1" : "0"})));
    }
    m_statusLabel->setText(QString("Found %1 device(s)").arg(devices.size()));
}

void MainWindow::onAddDevice()
{
    int idx = m_deviceCombo->currentIndex();
    if (idx < 0) return;

    QStringList data = m_deviceCombo->currentData().toStringList();
    if (data.size() < 3) return;
    QString deviceId = data[0];
    QString deviceName = data[1];
    bool isOutput = data[2] == "1";

    int row = m_table->rowCount();
    m_table->insertRow(row);

    m_table->setItem(row, 0, new QTableWidgetItem(deviceName));
    m_table->setItem(row, 1, new QTableWidgetItem(isOutput ? "Output" : "Input"));

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
        if (entry.hotkey.isEmpty()) continue;

        // Always register a long-press handler. The BT operation itself will
        // gracefully no-op (with an explanatory notification) if the device
        // isn't actually a paired Bluetooth endpoint. This avoids the trap
        // where our name heuristic misses devices like "Headphones (X Stereo)".
        HotkeyManager::HotkeyCallback shortCb =
            [this, entry]() { handleHotkeyShortPress(entry); };
        HotkeyManager::HotkeyCallback longCb =
            [this, entry]() { handleHotkeyLongPress(entry); };

        m_hotkeyManager->registerHotkey(entry.id, entry.hotkey,
                                        std::move(shortCb), std::move(longCb), 1200);
    }
}

void MainWindow::handleHotkeyShortPress(const DeviceEntry &entry)
{
    // Toggle-mute behaviour:
    //  - Same hotkey on already-active device toggles mute
    //  - Different device switches and unmutes
    if (m_lastActivatedDeviceId == entry.deviceId &&
        m_lastActivatedIsOutput == entry.isOutput)
    {
        bool newMuted = false;
        if (m_audioManager->toggleMute(entry.deviceId, entry.isOutput, &newMuted)) {
            if (m_trayIcon) {
                m_trayIcon->showSwitchedNotification(
                    QString("%1 %2").arg(entry.deviceName, newMuted ? "muted" : "unmuted"));
            }
        }
        return;
    }

    m_audioManager->setDefaultDevice(entry.deviceId, entry.isOutput);
    m_audioManager->setMute(entry.deviceId, entry.isOutput, false);
    m_lastActivatedDeviceId = entry.deviceId;
    m_lastActivatedIsOutput = entry.isOutput;

    if (m_trayIcon) {
        m_trayIcon->showSwitchedNotification(entry.deviceName);
    }
}

void MainWindow::handleHotkeyLongPress(const DeviceEntry &entry)
{
    if (!m_bluetoothManager) return;

    const QString key = entry.deviceName.toLower();
    const bool currentlyDisconnected = m_disconnectedDevices.contains(key);

    QString err;
    bool ok = false;
    QString verb;
    if (currentlyDisconnected) {
        ok = m_bluetoothManager->connect(entry.deviceName, &err);
        verb = ok ? QStringLiteral("Reconnecting") : QStringLiteral("Reconnect failed");
        if (ok) m_disconnectedDevices.remove(key);
    } else {
        ok = m_bluetoothManager->disconnect(entry.deviceName, &err);
        verb = ok ? QStringLiteral("Disconnected") : QStringLiteral("Disconnect failed");
        if (ok) m_disconnectedDevices.insert(key);
    }

    if (m_trayIcon) {
        QString msg = QStringLiteral("%1 %2").arg(verb, entry.deviceName);
        if (!ok && !err.isEmpty())
            msg += QStringLiteral(" (%1)").arg(err);
        m_trayIcon->showSwitchedNotification(msg);
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

void MainWindow::onAutostartToggled(bool checked)
{
    QString err;
    const bool ok = AutostartManager::setEnabled(checked, &err);
    if (!ok) {
        // Revert checkbox state without re-triggering this slot.
        QSignalBlocker block(m_autostartCheck);
        m_autostartCheck->setChecked(AutostartManager::isEnabled());
        m_statusLabel->setText(
            QString("Could not %1 autostart%2")
                .arg(checked ? "enable" : "disable",
                     err.isEmpty() ? QString() : QString(": %1").arg(err)));
        return;
    }
    m_statusLabel->setText(checked ? "Autostart enabled"
                                   : "Autostart disabled");
}

void MainWindow::applyCurrentTheme()
{
    if (m_contentWidget) {
        m_contentWidget->setStyleSheet(ThemeManager::instance().styleSheet());
    }

    auto *subtitleLabel = findChild<QLabel *>("subtitleLabel");
    if (subtitleLabel) {
        ThemeColors c = ThemeManager::instance().colors();
        subtitleLabel->setStyleSheet(
            QString("color: %1; font-size: 12px; margin-bottom: 4px;").arg(c.textSecondary));
    }
}
