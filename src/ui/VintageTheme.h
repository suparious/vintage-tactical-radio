#ifndef VINTAGETHEME_H
#define VINTAGETHEME_H

#include <QString>
#include <QColor>

class QWidget;
class QPalette;

class VintageTheme {
public:
    enum Theme {
        MILITARY_OLIVE,
        NAVY_GREY,
        NIGHT_MODE,
        DESERT_TAN,
        BLACK_OPS
    };
    
    static void applyTheme(QWidget* widget, Theme theme);
    static QString getStyleSheet(Theme theme);
    static QPalette getPalette(Theme theme);
    
    // Color scheme getters
    static QColor getBackgroundColor(Theme theme);
    static QColor getPanelColor(Theme theme);
    static QColor getTextColor(Theme theme);
    static QColor getDisplayColor(Theme theme);
    static QColor getMeterColor(Theme theme);
    static QColor getIndicatorColor(Theme theme, bool active);
    
private:
    static QString getBaseStyleSheet();
    static QString getMilitaryOliveStyle();
    static QString getNavyGreyStyle();
    static QString getNightModeStyle();
    static QString getDesertTanStyle();
    static QString getBlackOpsStyle();
};

#endif // VINTAGETHEME_H
