#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QStringList>
#include <QMap>

struct ThemeColors {
    QString windowBg;
    QString widgetBg;
    QString border;
    QString selectionBg;
    QString textPrimary;
    QString textSecondary;
    QString accent;
    QString accentHover;
    QString accentPressed;
    QString danger;
    QString dangerHover;
    QString success;
    QString headerBg;
    QString inputBg;
    QString inputText;
    QString notificationBg;
};

class ThemeManager {
public:
    ThemeManager();

    QStringList availableThemes() const;
    QString currentTheme() const;
    void setTheme(const QString &themeName);

    QString styleSheet() const;
    ThemeColors colors() const;

    static ThemeManager &instance();

private:
    void buildThemes();
    QString generateStyleSheet(const ThemeColors &colors) const;

    QString m_currentTheme;
    QMap<QString, ThemeColors> m_themes;
};

#endif // THEMEMANAGER_H
