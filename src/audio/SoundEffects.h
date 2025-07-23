#ifndef SOUNDEFFECTS_H
#define SOUNDEFFECTS_H

#include <QObject>
#include <QSoundEffect>
#include <QString>
#include <QMap>
#include <memory>

class SoundEffects : public QObject {
    Q_OBJECT
    
public:
    enum Effect {
        KNOB_TURN,
        SWITCH_CLICK,
        BUTTON_PRESS,
        BUTTON_RELEASE,
        STATIC_BURST,
        SQUELCH_TAIL,
        POWER_ON,
        POWER_OFF,
        MEMORY_BEEP,
        ERROR_BEEP
    };
    
    explicit SoundEffects(QObject* parent = nullptr);
    ~SoundEffects() = default;
    
    // Play a sound effect
    void play(Effect effect);
    
    // Enable/disable all sound effects
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Set volume for all effects (0.0 - 1.0)
    void setVolume(float volume);
    float getVolume() const { return volume_; }
    
    // Load custom sound files
    bool loadEffect(Effect effect, const QString& filename);
    
    // Generate synthesized effects (for missing files)
    void generateDefaultSounds();
    
private:
    bool enabled_;
    float volume_;
    
    // Sound effects storage
    QMap<Effect, std::unique_ptr<QSoundEffect>> effects_;
    
    // Create a synthesized click sound
    void generateClick(QSoundEffect* effect, int frequency, int duration);
    
    // Create a synthesized beep
    void generateBeep(QSoundEffect* effect, int frequency, int duration);
    
    // Create static noise
    void generateStatic(QSoundEffect* effect, int duration);
    
    // Load or generate a specific effect
    void initializeEffect(Effect effect);
    
    // Get default filename for an effect
    QString getDefaultFilename(Effect effect) const;
};

#endif // SOUNDEFFECTS_H
