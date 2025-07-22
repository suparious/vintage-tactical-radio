#include "VintageMeter.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QtMath>
#include <QTimer>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

VintageMeter::VintageMeter(QWidget* parent)
    : QWidget(parent)
    , minimum_(-100.0)
    , maximum_(0.0)
    , currentValue_(-100.0)
    , displayValue_(-100.0)
    , targetValue_(-100.0)
    , peakValue_(-100.0)
    , peakHold_(true) {
    
    setMinimumSize(250, 100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Setup animation timer
    animationTimer_ = new QTimer(this);
    animationTimer_->setInterval(20); // 50 FPS
    connect(animationTimer_, &QTimer::timeout, this, &VintageMeter::updateAnimation);
    animationTimer_->start();
    
    // Setup peak decay timer
    peakDecayTimer_ = new QTimer(this);
    peakDecayTimer_->setInterval(100); // 10 FPS for peak decay
    connect(peakDecayTimer_, &QTimer::timeout, this, &VintageMeter::decayPeak);
    peakDecayTimer_->start();
}

void VintageMeter::setRange(double min, double max) {
    if (min >= max) return;
    
    minimum_ = min;
    maximum_ = max;
    currentValue_ = qBound(minimum_, currentValue_, maximum_);
    targetValue_ = currentValue_;
    displayValue_ = currentValue_;
    peakValue_ = currentValue_;
    update();
}

void VintageMeter::setValue(double value) {
    double newValue = qBound(minimum_, value, maximum_);
    if (qFuzzyCompare(newValue, currentValue_)) return;
    
    currentValue_ = newValue;
    targetValue_ = newValue;
    
    // Update peak
    if (newValue > peakValue_) {
        peakValue_ = newValue;
    }
    
    emit valueChanged(currentValue_);
}

void VintageMeter::setMinimum(double min) {
    setRange(min, maximum_);
}

void VintageMeter::setMaximum(double max) {
    setRange(minimum_, max);
}

void VintageMeter::setLabel(const QString& label) {
    label_ = label;
    update();
}

void VintageMeter::setPeakHold(bool enable) {
    peakHold_ = enable;
    if (!enable) {
        peakValue_ = currentValue_;
    }
    update();
}

void VintageMeter::resetPeak() {
    peakValue_ = currentValue_;
    update();
}

QSize VintageMeter::sizeHint() const {
    return QSize(300, 120);
}

QSize VintageMeter::minimumSizeHint() const {
    return QSize(200, 80);
}

void VintageMeter::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect meterRect = rect().adjusted(10, 10, -10, -25);
    
    // Draw background
    drawBackground(painter, meterRect);
    
    // Draw scale
    drawScale(painter, meterRect);
    
    // Draw peak indicator
    if (peakHold_) {
        drawPeakIndicator(painter, meterRect);
    }
    
    // Draw needle
    drawNeedle(painter, meterRect, displayValue_);
    
    // Draw glass effect
    drawGlass(painter, meterRect);
    
    // Draw label
    if (!label_.isEmpty()) {
        QFont font = painter.font();
        font.setFamily("Arial");
        font.setPixelSize(11);
        font.setWeight(QFont::Bold);
        painter.setFont(font);
        painter.setPen(QColor(240, 240, 240));
        
        QRect labelRect(0, height() - 20, width(), 20);
        painter.drawText(labelRect, Qt::AlignCenter, label_);
    }
}

void VintageMeter::drawBackground(QPainter& painter, const QRect& rect) {
    // Meter face background
    QLinearGradient bgGradient(rect.topLeft(), rect.bottomLeft());
    bgGradient.setColorAt(0.0, QColor(40, 40, 35));
    bgGradient.setColorAt(1.0, QColor(20, 20, 15));
    
    painter.setPen(QPen(QColor(80, 80, 80), 2));
    painter.setBrush(bgGradient);
    painter.drawRoundedRect(rect, 5, 5);
    
    // Inner meter face
    QRect innerRect = rect.adjusted(5, 5, -5, -5);
    QLinearGradient innerGradient(innerRect.topLeft(), innerRect.bottomLeft());
    innerGradient.setColorAt(0.0, QColor(250, 245, 230));
    innerGradient.setColorAt(1.0, QColor(240, 235, 220));
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(innerGradient);
    painter.drawRoundedRect(innerRect, 3, 3);
}

void VintageMeter::drawScale(QPainter& painter, const QRect& rect) {
    QPoint center(rect.center().x(), rect.bottom() - 10);
    
    // Draw scale arc
    painter.setPen(QPen(QColor(60, 60, 60), 2));
    painter.setBrush(Qt::NoBrush);
    
    int arcRadius = rect.width() / 2 - 20;
    QRect arcRect(center.x() - arcRadius, center.y() - arcRadius,
                  arcRadius * 2, arcRadius * 2);
    painter.drawArc(arcRect, (90 + startAngle_) * 16, 
                    (endAngle_ - startAngle_) * 16);
    
    // Draw scale markings
    QFont font = painter.font();
    font.setFamily("Arial");
    font.setPixelSize(9);
    painter.setFont(font);
    
    // S-meter scale: 1, 3, 5, 7, 9, +20, +40, +60
    QStringList labels = {"1", "3", "5", "7", "9", "+20", "+40", "+60"};
    double values[] = {-90, -80, -70, -60, -50, -30, -10, 0};
    
    for (int i = 0; i < labels.size(); i++) {
        double value = values[i];
        double angle = valueToAngle(value);
        double rad = qDegreesToRadians(angle);
        
        // Draw tick mark
        int innerRadius = arcRadius - 5;
        int outerRadius = arcRadius + 5;
        
        QPointF innerPoint(center.x() + innerRadius * sin(rad),
                          center.y() - innerRadius * cos(rad));
        QPointF outerPoint(center.x() + outerRadius * sin(rad),
                          center.y() - outerRadius * cos(rad));
        
        painter.setPen(QPen(QColor(80, 80, 80), 2));
        painter.drawLine(innerPoint, outerPoint);
        
        // Draw label
        int labelRadius = arcRadius - 20;
        QPointF labelPoint(center.x() + labelRadius * sin(rad),
                          center.y() - labelRadius * cos(rad));
        
        painter.setPen(QColor(60, 60, 60));
        QRect labelRect(labelPoint.x() - 15, labelPoint.y() - 10, 30, 20);
        painter.drawText(labelRect, Qt::AlignCenter, labels[i]);
    }
    
    // Draw colored zones
    painter.setPen(Qt::NoPen);
    
    // S1-S9 zone (green to yellow)
    QConicalGradient s9Gradient(center, -90 - startAngle_);
    s9Gradient.setColorAt(0.0, QColor(0, 150, 0, 80));
    s9Gradient.setColorAt(0.7, QColor(200, 200, 0, 80));
    painter.setBrush(s9Gradient);
    
    QRect s9Rect = arcRect.adjusted(-10, -10, 10, 10);
    painter.drawPie(s9Rect, (90 + valueToAngle(-90)) * 16,
                    (valueToAngle(-50) - valueToAngle(-90)) * 16);
    
    // +20 to +60 zone (red)
    painter.setBrush(QColor(200, 0, 0, 80));
    painter.drawPie(s9Rect, (90 + valueToAngle(-30)) * 16,
                    (valueToAngle(0) - valueToAngle(-30)) * 16);
}

void VintageMeter::drawNeedle(QPainter& painter, const QRect& rect, double value) {
    QPoint center(rect.center().x(), rect.bottom() - 10);
    double angle = valueToAngle(value);
    double rad = qDegreesToRadians(angle);
    
    // Needle length
    int needleLen = rect.width() / 2 - 25;
    
    // Calculate needle tip
    QPointF tip(center.x() + needleLen * sin(rad),
                center.y() - needleLen * cos(rad));
    
    // Draw needle shadow
    painter.setPen(QPen(QColor(0, 0, 0, 80), 3));
    painter.drawLine(center + QPoint(2, 2), tip + QPoint(2, 2));
    
    // Draw needle
    painter.setPen(QPen(QColor(20, 20, 20), 2, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(center, tip);
    
    // Draw needle hub
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(40, 40, 40));
    painter.drawEllipse(center, 6, 6);
    painter.setBrush(QColor(80, 80, 80));
    painter.drawEllipse(center, 4, 4);
}

void VintageMeter::drawPeakIndicator(QPainter& painter, const QRect& rect) {
    if (peakValue_ <= minimum_) return;
    
    QPoint center(rect.center().x(), rect.bottom() - 10);
    double angle = valueToAngle(peakValue_);
    double rad = qDegreesToRadians(angle);
    
    int radius = rect.width() / 2 - 35;
    QPointF peakPoint(center.x() + radius * sin(rad),
                      center.y() - radius * cos(rad));
    
    // Draw peak marker
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 0, 0, 180));
    painter.drawEllipse(peakPoint, 3, 3);
}

void VintageMeter::drawGlass(QPainter& painter, const QRect& rect) {
    // Glass reflection effect
    QRect glassRect = rect.adjusted(5, 5, -5, -rect.height() / 2);
    QLinearGradient glassGradient(glassRect.topLeft(), glassRect.bottomLeft());
    glassGradient.setColorAt(0.0, QColor(255, 255, 255, 60));
    glassGradient.setColorAt(1.0, QColor(255, 255, 255, 0));
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(glassGradient);
    painter.drawRoundedRect(glassRect, 3, 3);
}

void VintageMeter::updateAnimation() {
    // Smooth needle movement
    double diff = targetValue_ - displayValue_;
    if (qAbs(diff) > 0.1) {
        displayValue_ += diff * dampingFactor_;
        update();
    }
}

void VintageMeter::decayPeak() {
    if (!peakHold_ || peakValue_ <= currentValue_) return;
    
    // Decay peak value
    peakValue_ -= peakDecayRate_ / 10.0; // Decay per 100ms
    if (peakValue_ < currentValue_) {
        peakValue_ = currentValue_;
    }
    update();
}

double VintageMeter::valueToAngle(double value) const {
    double normalized = (value - minimum_) / (maximum_ - minimum_);
    return startAngle_ + normalized * (endAngle_ - startAngle_);
}

QPointF VintageMeter::rotatePoint(const QPointF& point, const QPointF& center, double angle) const {
    double rad = qDegreesToRadians(angle);
    double cos_a = cos(rad);
    double sin_a = sin(rad);
    
    double x = point.x() - center.x();
    double y = point.y() - center.y();
    
    return QPointF(center.x() + x * cos_a - y * sin_a,
                   center.y() + x * sin_a + y * cos_a);
}

void VintageMeter::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}
