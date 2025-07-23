#ifndef DECODERWIDGET_H
#define DECODERWIDGET_H

#include <QWidget>
#include <memory>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QLabel;
class QTextEdit;
class QTableWidget;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

class CTCSSDecoder;
class RDSDecoder;
class ADSBDecoder;

class DecoderWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit DecoderWidget(QWidget* parent = nullptr);
    ~DecoderWidget();
    
    // Set decoders
    void setCTCSSDecoder(CTCSSDecoder* decoder);
    void setRDSDecoder(RDSDecoder* decoder);
    void setADSBDecoder(ADSBDecoder* decoder);
    
    // Control
    void setFrequency(double frequency);
    void setMode(const QString& mode);
    
public slots:
    // Enable/disable decoders based on frequency and mode
    void updateDecoderAvailability();
    
signals:
    // Enable/disable signals for MainWindow to connect to DSPEngine
    void ctcssEnableChanged(bool enabled);
    void rdsEnableChanged(bool enabled);
    void adsbEnableChanged(bool enabled);
    
private slots:
    // CTCSS slots
    void onCTCSSToneDetected(float frequency, float level);
    void onCTCSSToneLost();
    void onCTCSSEnableChanged(bool enabled);
    
    // RDS slots
    void onRDSProgramServiceChanged(const QString& ps);
    void onRDSRadioTextChanged(const QString& rt);
    void onRDSProgramTypeChanged(uint8_t pty);
    void onRDSTrafficAnnouncementChanged(bool ta);
    void onRDSClockTimeReceived(const QDateTime& ct);
    void onRDSEnableChanged(bool enabled);
    
    // ADS-B slots
    void onADSBAircraftUpdated(uint32_t icao, const QVariant& aircraft);
    void onADSBAircraftLost(uint32_t icao);
    void onADSBEnableChanged(bool enabled);
    void updateADSBDisplay();
    
private:
    void setupUI();
    void createCTCSSTab();
    void createRDSTab();
    void createADSBTab();
    void connectSignals();
    void applyVintageStyle();
    
    // Current state
    double currentFrequency_;
    QString currentMode_;
    
    // Decoders
    CTCSSDecoder* ctcssDecoder_;
    RDSDecoder* rdsDecoder_;
    ADSBDecoder* adsbDecoder_;
    
    // Main widget
    QTabWidget* tabWidget_;
    
    // CTCSS widgets
    QCheckBox* ctcssEnable_;
    QLabel* ctcssToneLabel_;
    QLabel* ctcssLevelLabel_;
    QLabel* ctcssStatusLabel_;
    QTextEdit* ctcssHistory_;
    
    // RDS widgets
    QCheckBox* rdsEnable_;
    QLabel* rdsPILabel_;
    QLabel* rdsPSLabel_;
    QLabel* rdsPTYLabel_;
    QTextEdit* rdsRTDisplay_;
    QLabel* rdsClockLabel_;
    QLabel* rdsTALabel_;
    QLabel* rdsTPLabel_;
    QLabel* rdsMSLabel_;
    
    // ADS-B widgets
    QCheckBox* adsbEnable_;
    QTableWidget* adsbTable_;
    QLabel* adsbCountLabel_;
    QLabel* adsbMessageLabel_;
    QPushButton* adsbClearButton_;
    
    // Update timers
    class QTimer* adsbUpdateTimer_;
};

#endif // DECODERWIDGET_H
