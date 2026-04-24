#include "thememanager.h"

ThemeManager &ThemeManager::instance()
{
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager()
    : m_currentTheme("Catppuccin Mocha")
{
    buildThemes();
}

void ThemeManager::buildThemes()
{
    // Catppuccin Mocha (original dark theme)
    m_themes["Catppuccin Mocha"] = {
        "#1e1e2e", "#313244", "#45475a", "#585b70",
        "#cdd6f4", "#a6adc8", "#89b4fa", "#74c7ec", "#89dceb",
        "#f38ba8", "#eba0ac", "#a6e3a1", "#45475a", "#313244", "#f9e2af",
        "#89b4fa"
    };

    // Light
    m_themes["Light"] = {
        "#f5f5f5", "#ffffff", "#d0d0d0", "#c8dcf0",
        "#1e1e1e", "#5a5a5a", "#2563eb", "#1d4ed8", "#1e40af",
        "#dc2626", "#b91c1c", "#16a34a", "#e8e8e8", "#ffffff", "#1e1e1e",
        "#2563eb"
    };

    // Nord
    m_themes["Nord"] = {
        "#2e3440", "#3b4252", "#434c5e", "#4c566a",
        "#eceff4", "#d8dee9", "#88c0d0", "#81a1c1", "#5e81ac",
        "#bf616a", "#d08770", "#a3be8c", "#434c5e", "#3b4252", "#ebcb8b",
        "#88c0d0"
    };

    // Dracula
    m_themes["Dracula"] = {
        "#282a36", "#44475a", "#6272a4", "#44475a",
        "#f8f8f2", "#bfbfbf", "#bd93f9", "#ff79c6", "#ff92df",
        "#ff5555", "#ff6e6e", "#50fa7b", "#44475a", "#282a36", "#f1fa8c",
        "#bd93f9"
    };

    // Solarized Dark
    m_themes["Solarized Dark"] = {
        "#002b36", "#073642", "#586e75", "#2aa198",
        "#839496", "#657b83", "#268bd2", "#2aa198", "#859900",
        "#dc322f", "#cb4b16", "#859900", "#073642", "#002b36", "#b58900",
        "#268bd2"
    };

    // Tokyo Night
    m_themes["Tokyo Night"] = {
        "#1a1b26", "#24283b", "#414868", "#33467c",
        "#c0caf5", "#a9b1d6", "#7aa2f7", "#7dcfff", "#89ddff",
        "#f7768e", "#ff9e64", "#9ece6a", "#24283b", "#1a1b26", "#e0af68",
        "#7aa2f7"
    };

    // Gruvbox Dark
    m_themes["Gruvbox"] = {
        "#282828", "#3c3836", "#504945", "#665c54",
        "#ebdbb2", "#a89984", "#83a598", "#8ec07c", "#b8bb26",
        "#fb4934", "#fe8019", "#b8bb26", "#3c3836", "#282828", "#fabd2f",
        "#83a598"
    };
}

QStringList ThemeManager::availableThemes() const
{
    return {"Catppuccin Mocha", "Light", "Nord", "Dracula", "Solarized Dark", "Tokyo Night", "Gruvbox"};
}

QString ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

void ThemeManager::setTheme(const QString &themeName)
{
    if (m_themes.contains(themeName)) {
        m_currentTheme = themeName;
    }
}

ThemeColors ThemeManager::colors() const
{
    return m_themes.value(m_currentTheme);
}

QString ThemeManager::styleSheet() const
{
    return generateStyleSheet(m_themes.value(m_currentTheme));
}

QString ThemeManager::generateStyleSheet(const ThemeColors &c) const
{
    return QString(R"(
        QWidget#contentRoot {
            background-color: %1;
        }
        QWidget {
            color: %5;
            font-size: 13px;
            font-family: "Segoe UI", "SF Pro Text", "Inter", "Ubuntu", sans-serif;
        }

        /* Card surfaces (toolbar + table containers) */
        QFrame#card {
            background-color: %2;
            border: 1px solid %3;
            border-radius: 10px;
        }

        /* Header / accent */
        QLabel#titleLabel {
            font-size: 22px;
            font-weight: 700;
            color: %5;
            letter-spacing: 0.3px;
        }
        QLabel#subtitleLabel {
            color: %6;
            font-size: 12px;
        }
        QFrame#accentLine {
            background-color: %7;
            border: none;
            max-height: 1px;
            margin: 4px 0 6px 0;
        }
        QLabel#fieldLabel {
            color: %6;
            font-size: 11px;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.6px;
            padding-right: 4px;
        }

        /* Table */
        QTableWidget {
            background-color: %2;
            alternate-background-color: %2;
            border: none;
            gridline-color: %3;
            selection-background-color: %4;
            selection-color: %5;
            border-radius: 10px;
        }
        QTableWidget::item {
            padding: 8px 10px;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: %4;
            color: %5;
        }
        QHeaderView::section {
            background-color: %12;
            color: %6;
            padding: 10px 10px;
            border: none;
            border-bottom: 1px solid %3;
            font-weight: 600;
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QHeaderView { background-color: %12; }

        /* Buttons */
        QPushButton {
            background-color: %2;
            color: %5;
            border: 1px solid %3;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 600;
        }
        QPushButton:hover {
            border-color: %7;
        }
        QPushButton:pressed {
            background-color: %12;
        }

        QPushButton#primaryBtn {
            background-color: %7;
            color: %1;
            border: 1px solid %7;
        }
        QPushButton#primaryBtn:hover {
            background-color: %8;
            border-color: %8;
        }
        QPushButton#primaryBtn:pressed {
            background-color: %9;
            border-color: %9;
        }

        QPushButton#secondaryBtn {
            background-color: transparent;
            color: %5;
            border: 1px solid %3;
        }
        QPushButton#secondaryBtn:hover {
            border-color: %7;
            color: %7;
        }

        QPushButton#dangerBtn {
            background-color: transparent;
            color: %10;
            border: 1px solid %10;
        }
        QPushButton#dangerBtn:hover {
            background-color: %10;
            color: %1;
        }
        QPushButton#dangerBtn:pressed {
            background-color: %11;
            border-color: %11;
        }

        /* Inputs */
        QComboBox {
            background-color: %14;
            border: 1px solid %3;
            border-radius: 8px;
            padding: 7px 12px;
            min-width: 200px;
            color: %15;
            selection-background-color: %4;
        }
        QComboBox:hover {
            border-color: %7;
        }
        QComboBox::drop-down {
            border: none;
            width: 22px;
        }
        QComboBox QAbstractItemView {
            background-color: %14;
            border: 1px solid %3;
            border-radius: 6px;
            selection-background-color: %4;
            color: %5;
            padding: 4px;
            outline: 0;
        }
        QKeySequenceEdit {
            background-color: %14;
            border: 1px solid %3;
            border-radius: 6px;
            padding: 6px 8px;
            color: %15;
        }
        QKeySequenceEdit:focus {
            border-color: %7;
        }

        /* Status / footer */
        QLabel#statusLabel {
            color: %13;
            font-size: 12px;
            font-style: italic;
            padding-right: 8px;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 4px 2px 4px 0;
        }
        QScrollBar::handle:vertical {
            background: %3;
            border-radius: 5px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover {
            background: %4;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal {
            background: transparent;
            height: 10px;
            margin: 0 4px 2px 4px;
        }
        QScrollBar::handle:horizontal {
            background: %3;
            border-radius: 5px;
            min-width: 24px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
    )")
    .arg(c.windowBg)       // %1
    .arg(c.widgetBg)       // %2
    .arg(c.border)         // %3
    .arg(c.selectionBg)    // %4
    .arg(c.textPrimary)    // %5
    .arg(c.textSecondary)  // %6
    .arg(c.accent)         // %7
    .arg(c.accentHover)    // %8
    .arg(c.accentPressed)  // %9
    .arg(c.danger)         // %10
    .arg(c.dangerHover)    // %11
    .arg(c.headerBg)       // %12
    .arg(c.success)        // %13
    .arg(c.inputBg)        // %14
    .arg(c.inputText);     // %15
}
