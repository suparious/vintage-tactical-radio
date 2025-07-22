#include "MemoryChannel.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <cmath>

MemoryChannel::MemoryChannel()
    : index_(0)
    , frequency_(0.0)
    , mode_("FM")
    , bandwidth_(200000)
    , gain_(30)
    , squelch_(-20.0) {
}

MemoryChannel::MemoryChannel(int index, double frequency, const QString& name)
    : index_(index)
    , frequency_(frequency)
    , name_(name)
    , mode_("FM")
    , bandwidth_(200000)
    , gain_(30)
    , squelch_(-20.0) {
}

QJsonObject MemoryChannel::toJson() const {
    QJsonObject json;
    json["index"] = index_;
    json["frequency"] = frequency_;
    json["name"] = name_;
    json["mode"] = mode_;
    json["bandwidth"] = bandwidth_;
    json["gain"] = gain_;
    json["squelch"] = squelch_;
    json["antenna"] = antenna_;
    json["notes"] = notes_;
    return json;
}

void MemoryChannel::fromJson(const QJsonObject& json) {
    index_ = json["index"].toInt();
    frequency_ = json["frequency"].toDouble();
    name_ = json["name"].toString();
    mode_ = json["mode"].toString("FM");
    bandwidth_ = json["bandwidth"].toDouble(200000);
    gain_ = json["gain"].toInt(30);
    squelch_ = json["squelch"].toDouble(-20.0);
    antenna_ = json["antenna"].toString();
    notes_ = json["notes"].toString();
}

void MemoryChannel::clear() {
    frequency_ = 0.0;
    name_.clear();
    mode_ = "FM";
    bandwidth_ = 200000;
    gain_ = 30;
    squelch_ = -20.0;
    antenna_.clear();
    notes_.clear();
}

// MemoryChannelManager implementation

MemoryChannelManager::MemoryChannelManager() {
    channels_.resize(TOTAL_CHANNELS);
    
    // Initialize channel indices
    for (int i = 0; i < TOTAL_CHANNELS; i++) {
        channels_[i].setIndex(i);
    }
    
    // Add some default quick channels
    addQuickChannel(96900000, "CJAX Jack FM");
    addQuickChannel(104900000, "Virgin Radio");
    addQuickChannel(102700000, "The Peak");
    addQuickChannel(156800000, "Marine Ch 16");
    addQuickChannel(121500000, "Aviation Emergency");
}

void MemoryChannelManager::setChannel(int index, const MemoryChannel& channel) {
    if (index >= 0 && index < TOTAL_CHANNELS) {
        channels_[index] = channel;
        channels_[index].setIndex(index);
    }
}

MemoryChannel MemoryChannelManager::getChannel(int index) const {
    if (index >= 0 && index < TOTAL_CHANNELS) {
        return channels_[index];
    }
    return MemoryChannel();
}

void MemoryChannelManager::clearChannel(int index) {
    if (index >= 0 && index < TOTAL_CHANNELS) {
        channels_[index].clear();
        channels_[index].setIndex(index);
    }
}

void MemoryChannelManager::clearAll() {
    for (auto& channel : channels_) {
        int idx = channel.index();
        channel.clear();
        channel.setIndex(idx);
    }
}

std::vector<MemoryChannel> MemoryChannelManager::getBank(int bankIndex) const {
    std::vector<MemoryChannel> bank;
    
    if (bankIndex >= 0 && bankIndex < NUM_BANKS) {
        int startIdx = bankIndex * CHANNELS_PER_BANK;
        int endIdx = startIdx + CHANNELS_PER_BANK;
        
        for (int i = startIdx; i < endIdx; i++) {
            bank.push_back(channels_[i]);
        }
    }
    
    return bank;
}

void MemoryChannelManager::setBank(int bankIndex, const std::vector<MemoryChannel>& channels) {
    if (bankIndex >= 0 && bankIndex < NUM_BANKS) {
        int startIdx = bankIndex * CHANNELS_PER_BANK;
        
        for (size_t i = 0; i < channels.size() && i < CHANNELS_PER_BANK; i++) {
            channels_[startIdx + i] = channels[i];
            channels_[startIdx + i].setIndex(startIdx + i);
        }
    }
}

std::vector<MemoryChannel> MemoryChannelManager::findByName(const QString& name) const {
    std::vector<MemoryChannel> results;
    
    QString searchName = name.toLower();
    for (const auto& channel : channels_) {
        if (!channel.isEmpty() && channel.name().toLower().contains(searchName)) {
            results.push_back(channel);
        }
    }
    
    return results;
}

std::vector<MemoryChannel> MemoryChannelManager::findByFrequency(double freq, double tolerance) const {
    std::vector<MemoryChannel> results;
    
    for (const auto& channel : channels_) {
        if (!channel.isEmpty() && std::abs(channel.frequency() - freq) <= tolerance) {
            results.push_back(channel);
        }
    }
    
    return results;
}

bool MemoryChannelManager::saveToFile(const QString& filename) const {
    QJsonObject root;
    
    // Save regular channels
    QJsonArray channelsArray;
    for (const auto& channel : channels_) {
        if (!channel.isEmpty()) {
            channelsArray.append(channel.toJson());
        }
    }
    root["channels"] = channelsArray;
    
    // Save quick channels
    QJsonArray quickArray;
    for (const auto& channel : quickChannels_) {
        quickArray.append(channel.toJson());
    }
    root["quickChannels"] = quickArray;
    
    QJsonDocument doc(root);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

bool MemoryChannelManager::loadFromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }
    
    clearAll();
    quickChannels_.clear();
    
    QJsonObject root = doc.object();
    
    // Load regular channels
    QJsonArray channelsArray = root["channels"].toArray();
    for (const auto& value : channelsArray) {
        MemoryChannel channel;
        channel.fromJson(value.toObject());
        if (channel.index() >= 0 && channel.index() < TOTAL_CHANNELS) {
            channels_[channel.index()] = channel;
        }
    }
    
    // Load quick channels
    QJsonArray quickArray = root["quickChannels"].toArray();
    for (const auto& value : quickArray) {
        MemoryChannel channel;
        channel.fromJson(value.toObject());
        quickChannels_.push_back(channel);
    }
    
    return true;
}

void MemoryChannelManager::addQuickChannel(double frequency, const QString& name) {
    MemoryChannel channel(-1, frequency, name);
    quickChannels_.push_back(channel);
}

std::vector<MemoryChannel> MemoryChannelManager::getQuickChannels() const {
    return quickChannels_;
}

int MemoryChannelManager::indexToBank(int index) const {
    return index / CHANNELS_PER_BANK;
}

int MemoryChannelManager::indexInBank(int index) const {
    return index % CHANNELS_PER_BANK;
}
