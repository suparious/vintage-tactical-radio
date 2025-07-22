#include "Settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

Settings::Settings() {
    // Set config file path
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    configFile_ = configPath + "/settings.json";
    
    // Initialize with defaults
    initializeDefaults();
}

Settings::~Settings() {
    // Auto-save on destruction
    save();
}

bool Settings::load() {
    return loadFromFile(configFile_);
}

bool Settings::save() {
    return saveToFile(configFile_);
}

bool Settings::loadFromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
#ifdef HAS_SPDLOG
        spdlog::warn("Failed to open settings file: {}", filename.toStdString());
#endif
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
#ifdef HAS_SPDLOG
        spdlog::error("Invalid settings file format");
#endif
        return false;
    }
    
    // Clear current settings
    settings_.clear();
    
    // Load from JSON
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        settings_[it.key()] = jsonValueToVariant(it.value());
    }
    
#ifdef HAS_SPDLOG
    spdlog::info("Settings loaded from: {}", filename.toStdString());
#endif
    
    return true;
}

bool Settings::saveToFile(const QString& filename) {
    QJsonObject root;
    
    // Convert settings to JSON
    for (const auto& pair : settings_) {
        root[pair.first] = variantToJsonValue(pair.second);
    }
    
    QJsonDocument doc(root);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
#ifdef HAS_SPDLOG
        spdlog::error("Failed to open settings file for writing: {}", filename.toStdString());
#endif
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
#ifdef HAS_SPDLOG
    spdlog::info("Settings saved to: {}", filename.toStdString());
#endif
    
    return true;
}

QVariant Settings::getValue(const QString& key, const QVariant& defaultValue) const {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        return it->second;
    }
    return defaultValue;
}

void Settings::setValue(const QString& key, const QVariant& value) {
    settings_[key] = value;
}

QJsonObject Settings::getGroup(const QString& groupName) const {
    QJsonObject group;
    QString prefix = groupName + ".";
    
    for (const auto& pair : settings_) {
        if (pair.first.startsWith(prefix)) {
            QString key = pair.first.mid(prefix.length());
            group[key] = variantToJsonValue(pair.second);
        }
    }
    
    return group;
}

void Settings::setGroup(const QString& groupName, const QJsonObject& group) {
    QString prefix = groupName + ".";
    
    // Remove existing group settings
    auto it = settings_.begin();
    while (it != settings_.end()) {
        if (it->first.startsWith(prefix)) {
            it = settings_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Add new group settings
    for (auto it = group.begin(); it != group.end(); ++it) {
        settings_[prefix + it.key()] = jsonValueToVariant(it.value());
    }
}

void Settings::resetToDefaults() {
    settings_.clear();
    initializeDefaults();
}

void Settings::resetGroup(const QString& groupName) {
    QString prefix = groupName + ".";
    
    auto it = settings_.begin();
    while (it != settings_.end()) {
        if (it->first.startsWith(prefix)) {
            it = settings_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Restore defaults for this group
    QJsonObject defaults = getDefaultSettings();
    if (defaults.contains(groupName)) {
        setGroup(groupName, defaults[groupName].toObject());
    }
}

QString Settings::exportToJson() const {
    QJsonObject root;
    
    for (const auto& pair : settings_) {
        root[pair.first] = variantToJsonValue(pair.second);
    }
    
    QJsonDocument doc(root);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

bool Settings::importFromJson(const QString& json) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }
    
    settings_.clear();
    
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        settings_[it.key()] = jsonValueToVariant(it.value());
    }
    
    return true;
}

QJsonObject Settings::getDefaultSettings() {
    QJsonObject defaults;
    
    // Audio settings
    QJsonObject audio;
    audio["device"] = "default";
    audio["sampleRate"] = 48000;
    audio["bitDepth"] = 16;
    audio["bufferSize"] = 1024;
    defaults["audio"] = audio;
    
    // Radio settings
    QJsonObject radio;
    radio["mode"] = "FM";
    radio["frequency"] = 96900000;
    radio["bandwidth"] = 200000;
    radio["gain"] = 30;
    radio["squelch"] = -20;
    defaults["radio"] = radio;
    
    // Equalizer settings
    QJsonObject equalizer;
    equalizer["mode"] = "modern";
    equalizer["preset"] = "flat";
    QJsonArray bands;
    for (int i = 0; i < 7; i++) {
        bands.append(0.0);
    }
    equalizer["bands"] = bands;
    defaults["equalizer"] = equalizer;
    
    // UI settings
    QJsonObject ui;
    ui["theme"] = "military-olive";
    ui["sounds"] = true;
    ui["animations"] = true;
    ui["windowGeometry"] = "";
    ui["windowState"] = "";
    defaults["ui"] = ui;
    
    // DSP settings
    QJsonObject dsp;
    dsp["agc"] = true;
    dsp["agcAttack"] = 0.01;
    dsp["agcDecay"] = 0.1;
    dsp["noiseReduction"] = false;
    dsp["noiseBlanker"] = false;
    dsp["notchFilter"] = false;
    defaults["dsp"] = dsp;
    
    return defaults;
}

QString Settings::getConfigPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString Settings::getDataPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

void Settings::initializeDefaults() {
    QJsonObject defaults = getDefaultSettings();
    
    // Flatten the defaults into settings map
    for (auto groupIt = defaults.begin(); groupIt != defaults.end(); ++groupIt) {
        if (groupIt.value().isObject()) {
            QJsonObject group = groupIt.value().toObject();
            for (auto it = group.begin(); it != group.end(); ++it) {
                QString key = groupIt.key() + "." + it.key();
                settings_[key] = jsonValueToVariant(it.value());
            }
        } else {
            settings_[groupIt.key()] = jsonValueToVariant(groupIt.value());
        }
    }
}

QVariant Settings::jsonValueToVariant(const QJsonValue& value) const {
    switch (value.type()) {
        case QJsonValue::Bool:
            return value.toBool();
        case QJsonValue::Double:
            return value.toDouble();
        case QJsonValue::String:
            return value.toString();
        case QJsonValue::Array:
            {
                QVariantList list;
                QJsonArray array = value.toArray();
                for (const auto& item : array) {
                    list.append(jsonValueToVariant(item));
                }
                return list;
            }
        case QJsonValue::Object:
            {
                QVariantMap map;
                QJsonObject obj = value.toObject();
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    map[it.key()] = jsonValueToVariant(it.value());
                }
                return map;
            }
        default:
            return QVariant();
    }
}

QJsonValue Settings::variantToJsonValue(const QVariant& value) const {
    switch (value.typeId()) {
        case QMetaType::Bool:
            return value.toBool();
        case QMetaType::Int:
        case QMetaType::Double:
            return value.toDouble();
        case QMetaType::QString:
            return value.toString();
        case QMetaType::QVariantList:
            {
                QJsonArray array;
                QVariantList list = value.toList();
                for (const auto& item : list) {
                    array.append(variantToJsonValue(item));
                }
                return array;
            }
        case QMetaType::QVariantMap:
            {
                QJsonObject obj;
                QVariantMap map = value.toMap();
                for (auto it = map.begin(); it != map.end(); ++it) {
                    obj[it.key()] = variantToJsonValue(it.value());
                }
                return obj;
            }
        default:
            return QJsonValue();
    }
}
