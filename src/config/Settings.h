#ifndef SETTINGS_H
#define SETTINGS_H

#include <memory>
#include <string>
#include <map>
#include <QVariant>
#include <QString>
#include <QJsonObject>
#include <QJsonValue>

class Settings {
public:
    Settings();
    ~Settings();
    
    // Load/Save operations
    bool load();
    bool save();
    bool loadFromFile(const QString& filename);
    bool saveToFile(const QString& filename);
    
    // Get/Set values
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);
    
    // Grouped settings
    QJsonObject getGroup(const QString& groupName) const;
    void setGroup(const QString& groupName, const QJsonObject& group);
    
    // Reset operations
    void resetToDefaults();
    void resetGroup(const QString& groupName);
    
    // Export/Import
    QString exportToJson() const;
    bool importFromJson(const QString& json);
    
    // Default settings
    static QJsonObject getDefaultSettings();
    
    // Paths
    QString getConfigPath() const;
    QString getDataPath() const;
    
private:
    std::map<QString, QVariant> settings_;
    QString configFile_;
    
    void initializeDefaults();
    QVariant jsonValueToVariant(const QJsonValue& value) const;
    QJsonValue variantToJsonValue(const QVariant& value) const;
};

#endif // SETTINGS_H
