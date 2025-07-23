#include "AntennaWidget.h"
#include "../core/AntennaRecommendation.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMessageBox>
#include <QStyle>

AntennaWidget::AntennaWidget(QWidget* parent)
    : QWidget(parent)
    , currentFrequency_(96.9e6) {
    
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(5);
    
    // Antenna icon
    iconLabel_ = new QLabel(this);
    iconLabel_->setFixedSize(20, 20);
    iconLabel_->setScaledContents(true);
    layout->addWidget(iconLabel_);
    
    // Antenna text
    antennaLabel_ = new QLabel(this);
    antennaLabel_->setWordWrap(false);
    layout->addWidget(antennaLabel_, 1);
    
    // Details button
    detailsButton_ = new QToolButton(this);
    detailsButton_->setText("?");
    detailsButton_->setFixedSize(20, 20);
    detailsButton_->setToolTip(tr("Show antenna details"));
    connect(detailsButton_, &QToolButton::clicked, this, &AntennaWidget::showAntennaDetails);
    layout->addWidget(detailsButton_);
    
    // Set initial antenna
    updateFrequency(currentFrequency_);
}

void AntennaWidget::updateFrequency(double frequencyHz) {
    currentFrequency_ = frequencyHz;
    
    auto recommendation = AntennaRecommendation::getRecommendation(frequencyHz);
    
    // Update text
    antennaLabel_->setText(QString("Antenna: %1 (%2)")
                          .arg(recommendation.antennaType)
                          .arg(recommendation.gain));
    
    // Update icon
    iconLabel_->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxInformation));
    
    // Update tooltip
    QString tooltip = QString("Frequency: %1 MHz\n")
                     .arg(frequencyHz / 1e6, 0, 'f', 3);
    tooltip += QString("Recommended: %1\n").arg(recommendation.antennaType);
    tooltip += QString("Impedance: %1").arg(recommendation.impedance);
    
    antennaLabel_->setToolTip(tooltip);
}

void AntennaWidget::showAntennaDetails() {
    QString advice = AntennaRecommendation::getAntennaAdvice(currentFrequency_);
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Antenna Recommendation"));
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(QString("<h3>For %1 MHz:</h3>").arg(currentFrequency_ / 1e6, 0, 'f', 3));
    msgBox.setInformativeText(advice);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    
    // Add custom style for vintage look
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #3a3a2a; color: #ffcc00; }"
        "QMessageBox QLabel { color: #ffcc00; }"
        "QMessageBox QPushButton { "
        "  background-color: #5a5a4a; "
        "  color: #ffcc00; "
        "  border: 2px solid #ffcc00; "
        "  padding: 5px 15px; "
        "  font-weight: bold; "
        "}"
        "QMessageBox QPushButton:hover { background-color: #6a6a5a; }"
    );
    
    msgBox.exec();
}

QString AntennaWidget::getAntennaIcon(const QString& iconName) {
    // This could be expanded to return actual antenna icon paths
    // For now, we'll use Qt's standard icons as placeholders
    Q_UNUSED(iconName);
    return QString();
}
