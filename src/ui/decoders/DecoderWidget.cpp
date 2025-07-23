#include "DecoderWidget.h"
#include "../../decoders/CTCSSDecoder.h"
#include "../../decoders/RDSDecoder.h"
#include "../../decoders/ADSBDecoder.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QTimer>
#include <QDateTime>
#include <QHeaderView>

DecoderWidget::DecoderWidget(QWidget* parent)
    : QWidget(parent)
    , currentFrequency_(0)
    , ctcssDecoder_(nullptr)
    , rdsDecoder_(nullptr)
    , adsbDecoder_(nullptr) {
    
    setupUI();
    connectSignals();
    applyVintageStyle();
    
    // Create update timer for ADS-B
    adsbUpdateTimer_ = new QTimer(this);
    adsbUpdateTimer_->setInterval(1000);  // Update every second
    connect(adsbUpdateTimer_, &QTimer::timeout, this, &DecoderWidget::updateADSBDisplay);
}

DecoderWidget::~DecoderWidget() {
    // Decoders are owned by DSPEngine, so we don't delete them
}

void DecoderWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create tab widget
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setObjectName("decoderTabs");
    
    // Create tabs
    createCTCSSTab();
    createRDSTab();
    createADSBTab();
    
    mainLayout->addWidget(tabWidget_);
}

void DecoderWidget::createCTCSSTab() {
    auto* ctcssWidget = new QWidget();
    auto* layout = new QVBoxLayout(ctcssWidget);
    
    // Enable checkbox
    ctcssEnable_ = new QCheckBox(tr("Enable CTCSS Detection"));
    ctcssEnable_->setObjectName("decoderEnable");
    layout->addWidget(ctcssEnable_);
    
    // Status group
    auto* statusGroup = new QGroupBox(tr("CTCSS Status"));
    auto* statusLayout = new QGridLayout(statusGroup);
    
    statusLayout->addWidget(new QLabel(tr("Tone:")), 0, 0);
    ctcssToneLabel_ = new QLabel(tr("---.- Hz"));
    ctcssToneLabel_->setObjectName("decoderValue");
    statusLayout->addWidget(ctcssToneLabel_, 0, 1);
    
    statusLayout->addWidget(new QLabel(tr("Level:")), 1, 0);
    ctcssLevelLabel_ = new QLabel(tr("-- dB"));
    ctcssLevelLabel_->setObjectName("decoderValue");
    statusLayout->addWidget(ctcssLevelLabel_, 1, 1);
    
    statusLayout->addWidget(new QLabel(tr("Status:")), 2, 0);
    ctcssStatusLabel_ = new QLabel(tr("Idle"));
    ctcssStatusLabel_->setObjectName("decoderStatus");
    statusLayout->addWidget(ctcssStatusLabel_, 2, 1);
    
    layout->addWidget(statusGroup);
    
    // History
    auto* historyGroup = new QGroupBox(tr("Detection History"));
    auto* historyLayout = new QVBoxLayout(historyGroup);
    
    ctcssHistory_ = new QTextEdit();
    ctcssHistory_->setObjectName("decoderHistory");
    ctcssHistory_->setReadOnly(true);
    ctcssHistory_->setMaximumHeight(150);
    historyLayout->addWidget(ctcssHistory_);
    
    layout->addWidget(historyGroup);
    layout->addStretch();
    
    tabWidget_->addTab(ctcssWidget, tr("CTCSS"));
}

void DecoderWidget::createRDSTab() {
    auto* rdsWidget = new QWidget();
    auto* layout = new QVBoxLayout(rdsWidget);
    
    // Enable checkbox
    rdsEnable_ = new QCheckBox(tr("Enable RDS Decoding"));
    rdsEnable_->setObjectName("decoderEnable");
    layout->addWidget(rdsEnable_);
    
    // Station info group
    auto* stationGroup = new QGroupBox(tr("Station Information"));
    auto* stationLayout = new QGridLayout(stationGroup);
    
    stationLayout->addWidget(new QLabel(tr("PI:")), 0, 0);
    rdsPILabel_ = new QLabel(tr("----"));
    rdsPILabel_->setObjectName("decoderValue");
    stationLayout->addWidget(rdsPILabel_, 0, 1);
    
    stationLayout->addWidget(new QLabel(tr("PS:")), 0, 2);
    rdsPSLabel_ = new QLabel(tr("--------"));
    rdsPSLabel_->setObjectName("decoderValue");
    rdsPSLabel_->setMinimumWidth(80);
    stationLayout->addWidget(rdsPSLabel_, 0, 3);
    
    stationLayout->addWidget(new QLabel(tr("PTY:")), 1, 0);
    rdsPTYLabel_ = new QLabel(tr("None"));
    rdsPTYLabel_->setObjectName("decoderValue");
    stationLayout->addWidget(rdsPTYLabel_, 1, 1, 1, 3);
    
    stationLayout->addWidget(new QLabel(tr("Clock:")), 2, 0);
    rdsClockLabel_ = new QLabel(tr("--:--"));
    rdsClockLabel_->setObjectName("decoderValue");
    stationLayout->addWidget(rdsClockLabel_, 2, 1);
    
    // Flags
    auto* flagLayout = new QHBoxLayout();
    rdsTALabel_ = new QLabel(tr("TA: OFF"));
    rdsTALabel_->setObjectName("decoderFlag");
    flagLayout->addWidget(rdsTALabel_);
    
    rdsTPLabel_ = new QLabel(tr("TP: OFF"));
    rdsTPLabel_->setObjectName("decoderFlag");
    flagLayout->addWidget(rdsTPLabel_);
    
    rdsMSLabel_ = new QLabel(tr("Music"));
    rdsMSLabel_->setObjectName("decoderFlag");
    flagLayout->addWidget(rdsMSLabel_);
    flagLayout->addStretch();
    
    stationLayout->addLayout(flagLayout, 2, 2, 1, 2);
    
    layout->addWidget(stationGroup);
    
    // RadioText group
    auto* rtGroup = new QGroupBox(tr("RadioText"));
    auto* rtLayout = new QVBoxLayout(rtGroup);
    
    rdsRTDisplay_ = new QTextEdit();
    rdsRTDisplay_->setObjectName("decoderText");
    rdsRTDisplay_->setReadOnly(true);
    rdsRTDisplay_->setMaximumHeight(100);
    rtLayout->addWidget(rdsRTDisplay_);
    
    layout->addWidget(rtGroup);
    layout->addStretch();
    
    tabWidget_->addTab(rdsWidget, tr("RDS"));
}

void DecoderWidget::createADSBTab() {
    auto* adsbWidget = new QWidget();
    auto* layout = new QVBoxLayout(adsbWidget);
    
    // Enable checkbox
    adsbEnable_ = new QCheckBox(tr("Enable ADS-B Decoding (1090 MHz)"));
    adsbEnable_->setObjectName("decoderEnable");
    layout->addWidget(adsbEnable_);
    
    // Status bar
    auto* statusLayout = new QHBoxLayout();
    adsbCountLabel_ = new QLabel(tr("Aircraft: 0"));
    adsbCountLabel_->setObjectName("decoderStatus");
    statusLayout->addWidget(adsbCountLabel_);
    
    adsbMessageLabel_ = new QLabel(tr("Messages: 0"));
    adsbMessageLabel_->setObjectName("decoderStatus");
    statusLayout->addWidget(adsbMessageLabel_);
    
    statusLayout->addStretch();
    
    adsbClearButton_ = new QPushButton(tr("Clear"));
    adsbClearButton_->setMaximumWidth(60);
    statusLayout->addWidget(adsbClearButton_);
    
    layout->addLayout(statusLayout);
    
    // Aircraft table
    adsbTable_ = new QTableWidget();
    adsbTable_->setObjectName("adsbTable");
    adsbTable_->setColumnCount(8);
    adsbTable_->setHorizontalHeaderLabels({
        tr("ICAO"), tr("Callsign"), tr("Altitude"), tr("Speed"),
        tr("Track"), tr("Lat"), tr("Lon"), tr("Last Seen")
    });
    
    adsbTable_->horizontalHeader()->setStretchLastSection(true);
    adsbTable_->setAlternatingRowColors(true);
    adsbTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    adsbTable_->setSortingEnabled(true);
    
    layout->addWidget(adsbTable_);
    
    tabWidget_->addTab(adsbWidget, tr("ADS-B"));
}

void DecoderWidget::connectSignals() {
    // CTCSS enable
    connect(ctcssEnable_, &QCheckBox::toggled,
            this, &DecoderWidget::onCTCSSEnableChanged);
    
    // RDS enable
    connect(rdsEnable_, &QCheckBox::toggled,
            this, &DecoderWidget::onRDSEnableChanged);
    
    // ADS-B enable
    connect(adsbEnable_, &QCheckBox::toggled,
            this, &DecoderWidget::onADSBEnableChanged);
    
    // ADS-B clear
    connect(adsbClearButton_, &QPushButton::clicked, [this]() {
        adsbTable_->setRowCount(0);
    });
}

void DecoderWidget::setCTCSSDecoder(CTCSSDecoder* decoder) {
    if (ctcssDecoder_) {
        // Disconnect old signals
        disconnect(ctcssDecoder_, nullptr, this, nullptr);
    }
    
    ctcssDecoder_ = decoder;
    
    if (ctcssDecoder_) {
        connect(ctcssDecoder_, &CTCSSDecoder::toneDetected,
                this, &DecoderWidget::onCTCSSToneDetected);
        connect(ctcssDecoder_, &CTCSSDecoder::toneLost,
                this, &DecoderWidget::onCTCSSToneLost);
    }
}

void DecoderWidget::setRDSDecoder(RDSDecoder* decoder) {
    if (rdsDecoder_) {
        disconnect(rdsDecoder_, nullptr, this, nullptr);
    }
    
    rdsDecoder_ = decoder;
    
    if (rdsDecoder_) {
        connect(rdsDecoder_, &RDSDecoder::programServiceChanged,
                this, &DecoderWidget::onRDSProgramServiceChanged);
        connect(rdsDecoder_, &RDSDecoder::radioTextChanged,
                this, &DecoderWidget::onRDSRadioTextChanged);
        connect(rdsDecoder_, &RDSDecoder::programTypeChanged,
                this, &DecoderWidget::onRDSProgramTypeChanged);
        connect(rdsDecoder_, &RDSDecoder::trafficAnnouncementChanged,
                this, &DecoderWidget::onRDSTrafficAnnouncementChanged);
        connect(rdsDecoder_, &RDSDecoder::clockTimeReceived,
                this, &DecoderWidget::onRDSClockTimeReceived);
    }
}

void DecoderWidget::setADSBDecoder(ADSBDecoder* decoder) {
    if (adsbDecoder_) {
        disconnect(adsbDecoder_, nullptr, this, nullptr);
    }
    
    adsbDecoder_ = decoder;
    
    if (adsbDecoder_) {
        // We need to handle the aircraft struct carefully
        connect(adsbDecoder_, &ADSBDecoder::aircraftUpdated,
                [this](uint32_t icao, const ADSBDecoder::Aircraft& aircraft) {
                    // Convert to QVariant for thread safety
                    QVariantMap data;
                    data["icao"] = icao;
                    data["callsign"] = aircraft.callsign;
                    data["altitude"] = aircraft.altitude;
                    data["speed"] = aircraft.groundSpeed;
                    data["track"] = aircraft.track;
                    data["latitude"] = aircraft.latitude;
                    data["longitude"] = aircraft.longitude;
                    data["verticalRate"] = aircraft.verticalRate;
                    data["onGround"] = aircraft.onGround;
                    
                    QMetaObject::invokeMethod(this, "onADSBAircraftUpdated",
                                            Qt::QueuedConnection,
                                            Q_ARG(uint32_t, icao),
                                            Q_ARG(QVariant, data));
                });
        
        connect(adsbDecoder_, &ADSBDecoder::aircraftLost,
                this, &DecoderWidget::onADSBAircraftLost);
    }
}

void DecoderWidget::setFrequency(double frequency) {
    currentFrequency_ = frequency;
    updateDecoderAvailability();
}

void DecoderWidget::setMode(const QString& mode) {
    currentMode_ = mode;
    updateDecoderAvailability();
}

void DecoderWidget::updateDecoderAvailability() {
    // CTCSS is available for all modes that output audio
    bool ctcssAvailable = (currentMode_ == "FM-Narrow" || 
                          currentMode_ == "FM-Wide" ||
                          currentMode_ == "AM");
    ctcssEnable_->setEnabled(ctcssAvailable);
    if (!ctcssAvailable && ctcssEnable_->isChecked()) {
        ctcssEnable_->setChecked(false);
    }
    
    // RDS is only available for FM broadcast band
    bool rdsAvailable = (currentMode_ == "FM-Wide" && 
                        currentFrequency_ >= 88e6 && 
                        currentFrequency_ <= 108e6);
    rdsEnable_->setEnabled(rdsAvailable);
    if (!rdsAvailable && rdsEnable_->isChecked()) {
        rdsEnable_->setChecked(false);
    }
    
    // ADS-B is only available at 1090 MHz
    bool adsbAvailable = (currentFrequency_ >= 1089e6 && 
                         currentFrequency_ <= 1091e6);
    adsbEnable_->setEnabled(adsbAvailable);
    if (!adsbAvailable && adsbEnable_->isChecked()) {
        adsbEnable_->setChecked(false);
    }
}

void DecoderWidget::onCTCSSEnableChanged(bool enabled) {
    if (ctcssDecoder_) {
        if (enabled) {
            ctcssDecoder_->start();
            ctcssStatusLabel_->setText(tr("Searching..."));
        } else {
            ctcssDecoder_->stop();
            ctcssStatusLabel_->setText(tr("Disabled"));
            ctcssToneLabel_->setText(tr("---.- Hz"));
            ctcssLevelLabel_->setText(tr("-- dB"));
        }
    }
    emit ctcssEnableChanged(enabled);
}

void DecoderWidget::onCTCSSToneDetected(float frequency, float level) {
    ctcssToneLabel_->setText(QString("%1 Hz").arg(frequency, 0, 'f', 1));
    ctcssLevelLabel_->setText(QString("%1 dB").arg(20 * log10(level), 0, 'f', 1));
    ctcssStatusLabel_->setText(tr("Tone Detected"));
    
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ctcssHistory_->append(QString("%1 - Detected: %2 Hz")
                         .arg(timestamp)
                         .arg(frequency, 0, 'f', 1));
}

void DecoderWidget::onCTCSSToneLost() {
    ctcssStatusLabel_->setText(tr("Searching..."));
    
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ctcssHistory_->append(QString("%1 - Tone lost").arg(timestamp));
}

void DecoderWidget::onRDSEnableChanged(bool enabled) {
    if (rdsDecoder_) {
        if (enabled) {
            rdsDecoder_->start();
            // Clear display
            rdsPILabel_->setText(tr("----"));
            rdsPSLabel_->setText(tr("--------"));
            rdsPTYLabel_->setText(tr("None"));
            rdsRTDisplay_->clear();
            rdsClockLabel_->setText(tr("--:--"));
        } else {
            rdsDecoder_->stop();
        }
    }
    emit rdsEnableChanged(enabled);
}

void DecoderWidget::onRDSProgramServiceChanged(const QString& ps) {
    rdsPSLabel_->setText(ps.isEmpty() ? tr("--------") : ps);
}

void DecoderWidget::onRDSRadioTextChanged(const QString& rt) {
    rdsRTDisplay_->setPlainText(rt);
}

void DecoderWidget::onRDSProgramTypeChanged(uint8_t pty) {
    rdsPTYLabel_->setText(RDSDecoder::getProgramTypeName(pty));
}

void DecoderWidget::onRDSTrafficAnnouncementChanged(bool ta) {
    rdsTALabel_->setText(ta ? tr("TA: ON") : tr("TA: OFF"));
    rdsTALabel_->setProperty("active", ta);
    rdsTALabel_->style()->polish(rdsTALabel_);
}

void DecoderWidget::onRDSClockTimeReceived(const QDateTime& ct) {
    rdsClockLabel_->setText(ct.toString("hh:mm"));
}

void DecoderWidget::onADSBEnableChanged(bool enabled) {
    if (adsbDecoder_) {
        if (enabled) {
            adsbDecoder_->start();
            adsbUpdateTimer_->start();
            adsbTable_->setRowCount(0);
        } else {
            adsbDecoder_->stop();
            adsbUpdateTimer_->stop();
        }
    }
    emit adsbEnableChanged(enabled);
}

void DecoderWidget::onADSBAircraftUpdated(uint32_t icao, const QVariant& aircraftVar) {
    QVariantMap aircraft = aircraftVar.toMap();
    
    // Find or create row
    int row = -1;
    for (int i = 0; i < adsbTable_->rowCount(); i++) {
        if (adsbTable_->item(i, 0)->data(Qt::UserRole).toUInt() == icao) {
            row = i;
            break;
        }
    }
    
    if (row == -1) {
        row = adsbTable_->rowCount();
        adsbTable_->insertRow(row);
        
        // Create items
        for (int col = 0; col < 8; col++) {
            auto* item = new QTableWidgetItem();
            adsbTable_->setItem(row, col, item);
        }
        
        // Store ICAO in first column
        adsbTable_->item(row, 0)->setData(Qt::UserRole, icao);
    }
    
    // Update data
    adsbTable_->item(row, 0)->setText(QString("%1").arg(icao, 6, 16, QChar('0')).toUpper());
    adsbTable_->item(row, 1)->setText(aircraft["callsign"].toString());
    adsbTable_->item(row, 2)->setText(QString("%1 ft").arg(aircraft["altitude"].toFloat(), 0, 'f', 0));
    adsbTable_->item(row, 3)->setText(QString("%1 kt").arg(aircraft["speed"].toFloat(), 0, 'f', 0));
    adsbTable_->item(row, 4)->setText(QString("%1°").arg(aircraft["track"].toFloat(), 0, 'f', 0));
    adsbTable_->item(row, 5)->setText(QString("%1").arg(aircraft["latitude"].toDouble(), 0, 'f', 4));
    adsbTable_->item(row, 6)->setText(QString("%1").arg(aircraft["longitude"].toDouble(), 0, 'f', 4));
    adsbTable_->item(row, 7)->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
}

void DecoderWidget::onADSBAircraftLost(uint32_t icao) {
    for (int i = 0; i < adsbTable_->rowCount(); i++) {
        if (adsbTable_->item(i, 0)->data(Qt::UserRole).toUInt() == icao) {
            adsbTable_->removeRow(i);
            break;
        }
    }
}

void DecoderWidget::updateADSBDisplay() {
    if (adsbDecoder_) {
        adsbCountLabel_->setText(QString("Aircraft: %1").arg(adsbTable_->rowCount()));
    }
}

void DecoderWidget::applyVintageStyle() {
    setStyleSheet(R"(
        QTabWidget#decoderTabs {
            background-color: #3a3a2a;
        }
        
        QTabWidget::pane {
            border: 2px solid #6a6a5a;
            background-color: #2a2a1a;
        }
        
        QTabBar::tab {
            background-color: #4a4a3a;
            color: #aaaaaa;
            padding: 5px 15px;
            margin-right: 2px;
            border: 1px solid #6a6a5a;
            border-bottom: none;
        }
        
        QTabBar::tab:selected {
            background-color: #5a5a4a;
            color: #ffcc00;
            border-bottom: 2px solid #5a5a4a;
        }
        
        QCheckBox#decoderEnable {
            color: #aaaaaa;
            font-weight: bold;
        }
        
        QCheckBox#decoderEnable:checked {
            color: #00ff00;
        }
        
        QLabel#decoderValue {
            color: #ffcc00;
            font-family: monospace;
            font-size: 12px;
            font-weight: bold;
            background-color: #1a1a0a;
            border: 1px solid #4a4a3a;
            padding: 2px 5px;
        }
        
        QLabel#decoderStatus {
            color: #aaaaaa;
            font-weight: bold;
        }
        
        QLabel#decoderFlag {
            color: #888888;
            font-family: monospace;
            padding: 2px 5px;
            border: 1px solid #4a4a3a;
        }
        
        QLabel#decoderFlag[active="true"] {
            color: #00ff00;
            background-color: #1a2a1a;
        }
        
        QTextEdit#decoderHistory, QTextEdit#decoderText {
            background-color: #1a1a0a;
            color: #aaaaaa;
            border: 1px solid #4a4a3a;
            font-family: monospace;
            font-size: 11px;
        }
        
        QTableWidget#adsbTable {
            background-color: #1a1a0a;
            alternate-background-color: #2a2a1a;
            color: #aaaaaa;
            gridline-color: #4a4a3a;
            font-family: monospace;
            font-size: 11px;
        }
        
        QTableWidget#adsbTable QHeaderView::section {
            background-color: #4a4a3a;
            color: #ffcc00;
            border: 1px solid #6a6a5a;
            padding: 3px;
        }
        
        QGroupBox {
            color: #aaaaaa;
            border: 2px solid #4a4a3a;
            margin-top: 10px;
            padding-top: 10px;
            font-weight: bold;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
    )");
}
