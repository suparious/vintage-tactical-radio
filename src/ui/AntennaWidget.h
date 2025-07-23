#ifndef ANTENNA_WIDGET_H
#define ANTENNA_WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QToolButton;
QT_END_NAMESPACE

class AntennaWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit AntennaWidget(QWidget* parent = nullptr);
    
    void updateFrequency(double frequencyHz);
    
signals:
    void detailsRequested();
    
private:
    QLabel* antennaLabel_;
    QLabel* iconLabel_;
    QToolButton* detailsButton_;
    
    double currentFrequency_;
    
    void showAntennaDetails();
    QString getAntennaIcon(const QString& iconName);
};

#endif // ANTENNA_WIDGET_H
