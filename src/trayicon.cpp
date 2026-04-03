#include "trayicon.h"
#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

TrayIcon::TrayIcon(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/tray.png"), this);
    m_trayIcon->setToolTip("QuickSnapAudio");

    m_menu = new QMenu();

    QAction *showAction = m_menu->addAction("Show Settings");
    connect(showAction, &QAction::triggered, this, [this]() {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    });

    m_menu->addSeparator();

    QAction *quitAction = m_menu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    m_trayIcon->setContextMenu(m_menu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::onActivated);
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

void TrayIcon::show()
{
    m_trayIcon->show();
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
}
