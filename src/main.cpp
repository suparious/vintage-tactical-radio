#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <memory>
#include <iostream>

#include "ui/MainWindow.h"
#include "config/Settings.h"
#include "core/RTLSDRDevice.h"

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#endif

void setupLogging() {
#ifdef HAS_SPDLOG
    try {
        auto configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configPath);
        
        auto logPath = configPath.toStdString() + "/vintage-tactical-radio.log";
        auto logger = spdlog::rotating_logger_mt("vtr", logPath, 1048576 * 10, 5);
        logger->set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);
        
        spdlog::info("Vintage Tactical Radio v1.0.0 starting...");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
#endif
}

void applyVintageStyle(QApplication& app) {
    // Set the base style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Load vintage stylesheet
    QFile styleFile(":/themes/military-olive.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("Vintage Tactical Radio");
    app.setOrganizationName("VintageRadioProject");
    app.setApplicationVersion("1.0.0");
    
    // Setup logging
    setupLogging();
    
    // Load settings
    auto settings = std::make_shared<Settings>();
    settings->load();
    
    // Check for RTL-SDR device
    try {
        auto rtlsdr = std::make_unique<RTLSDRDevice>();
        if (rtlsdr->getDeviceCount() == 0) {
            QMessageBox::critical(nullptr, "No RTL-SDR Found",
                "No RTL-SDR device was found. Please connect an RTL-SDR device and restart the application.");
            return 1;
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "RTL-SDR Error",
            QString("Failed to initialize RTL-SDR: %1").arg(e.what()));
        return 1;
    }
    
    // Apply vintage theme
    applyVintageStyle(app);
    
    // Create and show main window
    MainWindow window(settings);
    window.show();
    
    // Run application
    int result = app.exec();
    
    // Save settings on exit
    settings->save();
    
#ifdef HAS_SPDLOG
    spdlog::info("Vintage Tactical Radio shutting down...");
#endif
    
    return result;
}
