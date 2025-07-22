#include "VintageTheme.h"
#include <QWidget>
#include <QPalette>
#include <QApplication>

void VintageTheme::applyTheme(QWidget* widget, Theme theme) {
    // Apply palette
    widget->setPalette(getPalette(theme));
    
    // Apply stylesheet
    widget->setStyleSheet(getStyleSheet(theme));
    
    // Force update
    widget->update();
}

QString VintageTheme::getStyleSheet(Theme theme) {
    QString baseStyle = getBaseStyleSheet();
    
    switch (theme) {
        case MILITARY_OLIVE:
            return baseStyle + getMilitaryOliveStyle();
        case NAVY_GREY:
            return baseStyle + getNavyGreyStyle();
        case NIGHT_MODE:
            return baseStyle + getNightModeStyle();
        case DESERT_TAN:
            return baseStyle + getDesertTanStyle();
        case BLACK_OPS:
            return baseStyle + getBlackOpsStyle();
        default:
            return baseStyle + getMilitaryOliveStyle();
    }
}

QPalette VintageTheme::getPalette(Theme theme) {
    QPalette palette;
    
    QColor bg = getBackgroundColor(theme);
    QColor panel = getPanelColor(theme);
    QColor text = getTextColor(theme);
    
    palette.setColor(QPalette::Window, bg);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, panel);
    palette.setColor(QPalette::AlternateBase, panel.darker(110));
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::BrightText, text.lighter(120));
    palette.setColor(QPalette::Button, panel);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::Highlight, getIndicatorColor(theme, true));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    
    return palette;
}

QColor VintageTheme::getBackgroundColor(Theme theme) {
    switch (theme) {
        case MILITARY_OLIVE: return QColor(59, 59, 47);      // #3B3B2F
        case NAVY_GREY: return QColor(44, 62, 80);          // #2C3E50
        case NIGHT_MODE: return QColor(26, 0, 0);           // #1A0000
        case DESERT_TAN: return QColor(189, 174, 147);      // #BDAE93
        case BLACK_OPS: return QColor(16, 16, 16);          // #101010
        default: return QColor(59, 59, 47);
    }
}

QColor VintageTheme::getPanelColor(Theme theme) {
    switch (theme) {
        case MILITARY_OLIVE: return QColor(74, 74, 61);      // #4A4A3D
        case NAVY_GREY: return QColor(52, 73, 94);          // #34495E
        case NIGHT_MODE: return QColor(45, 0, 0);           // #2D0000
        case DESERT_TAN: return QColor(210, 195, 168);      // #D2C3A8
        case BLACK_OPS: return QColor(32, 32, 32);          // #202020
        default: return QColor(74, 74, 61);
    }
}

QColor VintageTheme::getTextColor(Theme theme) {
    switch (theme) {
        case MILITARY_OLIVE: return QColor(244, 230, 215);   // #F4E6D7
        case NAVY_GREY: return QColor(236, 240, 241);       // #ECF0F1
        case NIGHT_MODE: return QColor(255, 0, 0);          // #FF0000
        case DESERT_TAN: return QColor(51, 51, 51);         // #333333
        case BLACK_OPS: return QColor(0, 255, 0);           // #00FF00
        default: return QColor(244, 230, 215);
    }
}

QColor VintageTheme::getDisplayColor(Theme theme) {
    switch (theme) {
        case MILITARY_OLIVE: return QColor(255, 107, 0);     // #FF6B00 Amber
        case NAVY_GREY: return QColor(0, 255, 136);         // #00FF88 Green phosphor
        case NIGHT_MODE: return QColor(204, 0, 0);          // #CC0000 Deep red
        case DESERT_TAN: return QColor(0, 100, 200);        // #0064C8 Blue LCD
        case BLACK_OPS: return QColor(0, 255, 255);         // #00FFFF Cyan
        default: return QColor(255, 107, 0);
    }
}

QColor VintageTheme::getMeterColor(Theme theme) {
    switch (theme) {
        case MILITARY_OLIVE: return QColor(255, 215, 0);     // #FFD700 Gold
        case NAVY_GREY: return QColor(0, 255, 136);         // #00FF88 Green
        case NIGHT_MODE: return QColor(255, 51, 51);        // #FF3333 Light red
        case DESERT_TAN: return QColor(100, 100, 100);      // #646464 Gray
        case BLACK_OPS: return QColor(0, 255, 0);           // #00FF00 Green
        default: return QColor(255, 215, 0);
    }
}

QColor VintageTheme::getIndicatorColor(Theme theme, bool active) {
    if (!active) {
        return getTextColor(theme).darker(300);
    }
    
    switch (theme) {
        case MILITARY_OLIVE: return QColor(0, 255, 0);       // #00FF00 Green
        case NAVY_GREY: return QColor(0, 255, 0);           // #00FF00 Green
        case NIGHT_MODE: return QColor(255, 0, 0);          // #FF0000 Red
        case DESERT_TAN: return QColor(255, 165, 0);        // #FFA500 Orange
        case BLACK_OPS: return QColor(0, 255, 255);         // #00FFFF Cyan
        default: return QColor(0, 255, 0);
    }
}

QString VintageTheme::getBaseStyleSheet() {
    return R"(
        QMainWindow {
            font-family: "Arial", sans-serif;
            font-size: 12px;
        }
        
        QGroupBox {
            font-weight: bold;
            border: 2px solid;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        
        QPushButton {
            min-height: 30px;
            min-width: 80px;
            font-weight: bold;
            border: 2px solid;
            border-radius: 4px;
            padding: 5px;
        }
        
        QPushButton:pressed {
            padding: 7px 3px 3px 7px;
        }
        
        QComboBox {
            min-height: 25px;
            padding: 3px;
            border: 2px solid;
            border-radius: 4px;
        }
        
        QComboBox::drop-down {
            width: 20px;
            border-left: 2px solid;
        }
        
        QComboBox::down-arrow {
            width: 10px;
            height: 10px;
        }
        
        QLabel {
            font-size: 11px;
        }
        
        QSlider::groove:horizontal {
            height: 8px;
            border-radius: 4px;
        }
        
        QSlider::handle:horizontal {
            width: 18px;
            height: 18px;
            margin: -5px 0;
            border-radius: 9px;
        }
    )";
}

QString VintageTheme::getMilitaryOliveStyle() {
    return R"(
        QMainWindow {
            background-color: #3B3B2F;
            color: #F4E6D7;
        }
        
        QGroupBox {
            background-color: #4A4A3D;
            border-color: #6A6A5D;
            color: #F4E6D7;
        }
        
        QPushButton {
            background-color: #5A5A4D;
            border-color: #7A7A6D;
            color: #F4E6D7;
        }
        
        QPushButton:hover {
            background-color: #6A6A5D;
        }
        
        QPushButton:pressed {
            background-color: #4A4A3D;
        }
        
        QPushButton#startStopButton:checked {
            background-color: #FF6B00;
            color: #000000;
        }
        
        QComboBox {
            background-color: #5A5A4D;
            border-color: #7A7A6D;
            color: #F4E6D7;
        }
        
        QComboBox::drop-down {
            border-color: #7A7A6D;
        }
        
        QComboBox::down-arrow {
            image: none;
            border: 5px solid #F4E6D7;
            border-top: none;
            border-left: 3px solid transparent;
            border-right: 3px solid transparent;
        }
        
        QSlider::groove:horizontal {
            background-color: #3A3A2D;
            border: 1px solid #5A5A4D;
        }
        
        QSlider::handle:horizontal {
            background-color: #FF6B00;
            border: 2px solid #7A7A6D;
        }
    )";
}

QString VintageTheme::getNavyGreyStyle() {
    return R"(
        QMainWindow {
            background-color: #2C3E50;
            color: #ECF0F1;
        }
        
        QGroupBox {
            background-color: #34495E;
            border-color: #546E8A;
            color: #ECF0F1;
        }
        
        QPushButton {
            background-color: #445A74;
            border-color: #546E8A;
            color: #ECF0F1;
        }
        
        QPushButton:hover {
            background-color: #546E8A;
        }
        
        QPushButton:pressed {
            background-color: #34495E;
        }
        
        QPushButton#startStopButton:checked {
            background-color: #00FF88;
            color: #000000;
        }
        
        QComboBox {
            background-color: #445A74;
            border-color: #546E8A;
            color: #ECF0F1;
        }
        
        QSlider::groove:horizontal {
            background-color: #2C3E50;
            border: 1px solid #445A74;
        }
        
        QSlider::handle:horizontal {
            background-color: #00FF88;
            border: 2px solid #546E8A;
        }
    )";
}

QString VintageTheme::getNightModeStyle() {
    return R"(
        QMainWindow {
            background-color: #1A0000;
            color: #FF0000;
        }
        
        QGroupBox {
            background-color: #2D0000;
            border-color: #660000;
            color: #FF0000;
        }
        
        QPushButton {
            background-color: #3D0000;
            border-color: #660000;
            color: #FF0000;
        }
        
        QPushButton:hover {
            background-color: #4D0000;
        }
        
        QPushButton:pressed {
            background-color: #2D0000;
        }
        
        QPushButton#startStopButton:checked {
            background-color: #FF0000;
            color: #000000;
        }
        
        QComboBox {
            background-color: #3D0000;
            border-color: #660000;
            color: #FF0000;
        }
        
        QSlider::groove:horizontal {
            background-color: #1A0000;
            border: 1px solid #3D0000;
        }
        
        QSlider::handle:horizontal {
            background-color: #FF0000;
            border: 2px solid #660000;
        }
    )";
}

QString VintageTheme::getDesertTanStyle() {
    return R"(
        QMainWindow {
            background-color: #BDAE93;
            color: #333333;
        }
        
        QGroupBox {
            background-color: #D2C3A8;
            border-color: #A08970;
            color: #333333;
        }
        
        QPushButton {
            background-color: #C5B69C;
            border-color: #A08970;
            color: #333333;
        }
        
        QPushButton:hover {
            background-color: #D5C6AC;
        }
        
        QPushButton:pressed {
            background-color: #B5A68C;
        }
        
        QPushButton#startStopButton:checked {
            background-color: #0064C8;
            color: #FFFFFF;
        }
        
        QComboBox {
            background-color: #C5B69C;
            border-color: #A08970;
            color: #333333;
        }
        
        QSlider::groove:horizontal {
            background-color: #BDAE93;
            border: 1px solid #A08970;
        }
        
        QSlider::handle:horizontal {
            background-color: #0064C8;
            border: 2px solid #A08970;
        }
    )";
}

QString VintageTheme::getBlackOpsStyle() {
    return R"(
        QMainWindow {
            background-color: #101010;
            color: #00FF00;
        }
        
        QGroupBox {
            background-color: #202020;
            border-color: #00FF00;
            color: #00FF00;
        }
        
        QPushButton {
            background-color: #303030;
            border-color: #00FF00;
            color: #00FF00;
        }
        
        QPushButton:hover {
            background-color: #404040;
        }
        
        QPushButton:pressed {
            background-color: #202020;
        }
        
        QPushButton#startStopButton:checked {
            background-color: #00FFFF;
            color: #000000;
        }
        
        QComboBox {
            background-color: #303030;
            border-color: #00FF00;
            color: #00FF00;
        }
        
        QSlider::groove:horizontal {
            background-color: #101010;
            border: 1px solid #00FF00;
        }
        
        QSlider::handle:horizontal {
            background-color: #00FFFF;
            border: 2px solid #00FF00;
        }
    )";
}
