#ifndef ANTENNA_RECOMMENDATION_H
#define ANTENNA_RECOMMENDATION_H

#include <QString>
#include <QVector>

class AntennaRecommendation {
public:
    struct Recommendation {
        QString antennaType;
        QString frequencyRange;
        QString gain;
        QString impedance;
        QString matchingRequired;
        QString notes;
        QString icon; // Icon name for visual display
    };
    
    static Recommendation getRecommendation(double frequencyHz);
    static QString getAntennaAdvice(double frequencyHz);
    
private:
    struct FrequencyBand {
        double minFreq;
        double maxFreq;
        Recommendation recommendation;
    };
    
    static const QVector<FrequencyBand>& getBands();
};

#endif // ANTENNA_RECOMMENDATION_H
