#ifndef MEMORYCHANNEL_H
#define MEMORYCHANNEL_H

#include <QString>
#include <QJsonObject>
#include <vector>

class MemoryChannel {
public:
    MemoryChannel();
    MemoryChannel(int index, double frequency, const QString& name = QString());
    
    // Getters
    int index() const { return index_; }
    double frequency() const { return frequency_; }
    QString name() const { return name_; }
    QString mode() const { return mode_; }
    double bandwidth() const { return bandwidth_; }
    int gain() const { return gain_; }
    double squelch() const { return squelch_; }
    QString antenna() const { return antenna_; }
    QString notes() const { return notes_; }
    bool isEmpty() const { return frequency_ == 0.0; }
    
    // Setters
    void setIndex(int index) { index_ = index; }
    void setFrequency(double freq) { frequency_ = freq; }
    void setName(const QString& name) { name_ = name; }
    void setMode(const QString& mode) { mode_ = mode; }
    void setBandwidth(double bw) { bandwidth_ = bw; }
    void setGain(int gain) { gain_ = gain; }
    void setSquelch(double sq) { squelch_ = sq; }
    void setAntenna(const QString& ant) { antenna_ = ant; }
    void setNotes(const QString& notes) { notes_ = notes; }
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    
    // Clear channel
    void clear();
    
private:
    int index_;
    double frequency_;
    QString name_;
    QString mode_;
    double bandwidth_;
    int gain_;
    double squelch_;
    QString antenna_;
    QString notes_;
};

class MemoryChannelManager {
public:
    MemoryChannelManager();
    
    // Channel operations
    void setChannel(int index, const MemoryChannel& channel);
    MemoryChannel getChannel(int index) const;
    void clearChannel(int index);
    void clearAll();
    
    // Bank operations
    std::vector<MemoryChannel> getBank(int bankIndex) const;
    void setBank(int bankIndex, const std::vector<MemoryChannel>& channels);
    
    // Search
    std::vector<MemoryChannel> findByName(const QString& name) const;
    std::vector<MemoryChannel> findByFrequency(double freq, double tolerance = 1000.0) const;
    
    // Import/Export
    bool saveToFile(const QString& filename) const;
    bool loadFromFile(const QString& filename);
    
    // Quick access
    void addQuickChannel(double frequency, const QString& name);
    std::vector<MemoryChannel> getQuickChannels() const;
    
    // Constants
    static constexpr int CHANNELS_PER_BANK = 100;
    static constexpr int NUM_BANKS = 10;
    static constexpr int TOTAL_CHANNELS = CHANNELS_PER_BANK * NUM_BANKS;
    
private:
    std::vector<MemoryChannel> channels_;
    std::vector<MemoryChannel> quickChannels_;
    
    int indexToBank(int index) const;
    int indexInBank(int index) const;
};

#endif // MEMORYCHANNEL_H
