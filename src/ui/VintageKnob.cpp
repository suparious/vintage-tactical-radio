#include "VintageKnob.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <QRadialGradient>
#include <QConicalGradient>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

VintageKnob::VintageKnob(QWidget* parent)
    : QWidget(parent)
    , value_(0.0)
    , minimum_(0.0)
    , maximum_(100.0)
    , wrapping_(true)
    , isDragging_(false)
    , isHovered_(false) {
    
    setMinimumSize(100, 120);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void VintageKnob::setRange(double min, double max) {
    if (min >= max) return;
    
    minimum_ = min;
    maximum_ = max;
    value_ = qBound(minimum_, value_, maximum_);
    update();
}

void VintageKnob::setValue(double value) {
    double newValue = qBound(minimum_, value, maximum_);
    if (qFuzzyCompare(newValue, value_)) return;
    
    value_ = newValue;
    emit valueChanged(value_);
    update();
}

void VintageKnob::setMinimum(double min) {
    setRange(min, maximum_);
}

void VintageKnob::setMaximum(double max) {
    setRange(minimum_, max);
}

void VintageKnob::setLabel(const QString& label) {
    label_ = label;
    update();
}

void VintageKnob::setWrapping(bool wrap) {
    wrapping_ = wrap;
}

QSize VintageKnob::sizeHint() const {
    return QSize(100, 120);
}

QSize VintageKnob::minimumSizeHint() const {
    return QSize(80, 100);
}

void VintageKnob::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Calculate knob rectangle
    int size = qMin(width(), height() - 20);
    QRect knobRect((width() - size) / 2, 5, size, size);
    
    // Draw the knob
    drawKnob(painter, knobRect);
    
    // Draw the scale
    drawScale(painter, knobRect);
    
    // Draw the pointer
    double angle = valueToAngle(value_);
    drawPointer(painter, knobRect, angle);
    
    // Draw the label
    drawLabel(painter, rect());
}

void VintageKnob::drawKnob(QPainter& painter, const QRect& rect) {
    // Outer ring (aluminum bezel)
    QRadialGradient outerGradient(rect.center(), rect.width() / 2);
    outerGradient.setColorAt(0.0, QColor(180, 180, 180));
    outerGradient.setColorAt(0.9, QColor(120, 120, 120));
    outerGradient.setColorAt(1.0, QColor(80, 80, 80));
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(outerGradient);
    painter.drawEllipse(rect);
    
    // Inner knob (black plastic with texture)
    QRect innerRect = rect.adjusted(8, 8, -8, -8);
    QRadialGradient innerGradient(innerRect.center(), innerRect.width() / 2);
    
    if (isHovered_) {
        innerGradient.setColorAt(0.0, QColor(60, 60, 55));
        innerGradient.setColorAt(0.7, QColor(40, 40, 35));
        innerGradient.setColorAt(1.0, QColor(20, 20, 15));
    } else {
        innerGradient.setColorAt(0.0, QColor(50, 50, 45));
        innerGradient.setColorAt(0.7, QColor(30, 30, 25));
        innerGradient.setColorAt(1.0, QColor(10, 10, 5));
    }
    
    painter.setBrush(innerGradient);
    painter.drawEllipse(innerRect);
    
    // Add knurled edge effect
    painter.setPen(QPen(QColor(100, 100, 100, 50), 1));
    for (int i = 0; i < 36; i++) {
        double angle = i * 10.0 * M_PI / 180.0;
        int x1 = rect.center().x() + (rect.width() / 2 - 5) * cos(angle);
        int y1 = rect.center().y() + (rect.height() / 2 - 5) * sin(angle);
        int x2 = rect.center().x() + (rect.width() / 2 - 2) * cos(angle);
        int y2 = rect.center().y() + (rect.height() / 2 - 2) * sin(angle);
        painter.drawLine(x1, y1, x2, y2);
    }
}

void VintageKnob::drawPointer(QPainter& painter, const QRect& rect, double angle) {
    // Convert angle to radians
    double rad = qDegreesToRadians(angle - 90);
    
    // Calculate pointer position
    int centerX = rect.center().x();
    int centerY = rect.center().y();
    int radius = rect.width() / 2 - 15;
    
    int x = centerX + radius * cos(rad);
    int y = centerY + radius * sin(rad);
    
    // Draw pointer line
    painter.setPen(QPen(QColor(255, 100, 0), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(centerX, centerY, x, y);
    
    // Draw pointer dot
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 120, 0));
    painter.drawEllipse(QPoint(x, y), 4, 4);
}

void VintageKnob::drawScale(QPainter& painter, const QRect& rect) {
    painter.setPen(QPen(QColor(200, 200, 200), 2));
    
    // Draw scale marks
    int numMarks = 11;
    for (int i = 0; i < numMarks; i++) {
        double ratio = static_cast<double>(i) / (numMarks - 1);
        double angle = startAngle_ + ratio * (endAngle_ - startAngle_);
        double rad = qDegreesToRadians(angle - 90);
        
        int radius1 = rect.width() / 2 + 5;
        int radius2 = rect.width() / 2 + 10;
        
        // Major marks at 0%, 50%, and 100%
        if (i == 0 || i == numMarks / 2 || i == numMarks - 1) {
            radius2 += 5;
            painter.setPen(QPen(QColor(255, 255, 255), 2));
        } else {
            painter.setPen(QPen(QColor(180, 180, 180), 1));
        }
        
        int x1 = rect.center().x() + radius1 * cos(rad);
        int y1 = rect.center().y() + radius1 * sin(rad);
        int x2 = rect.center().x() + radius2 * cos(rad);
        int y2 = rect.center().y() + radius2 * sin(rad);
        
        painter.drawLine(x1, y1, x2, y2);
    }
}

void VintageKnob::drawLabel(QPainter& painter, const QRect& rect) {
    if (label_.isEmpty()) return;
    
    QFont font = painter.font();
    font.setFamily("Arial");
    font.setPixelSize(11);
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    
    painter.setPen(QColor(240, 240, 240));
    
    QRect labelRect(0, rect.height() - 20, rect.width(), 20);
    painter.drawText(labelRect, Qt::AlignCenter, label_);
    
    // Draw current value
    QString valueText = QString::number(value_, 'f', 1);
    QRect valueRect(0, rect.height() - 35, rect.width(), 15);
    font.setPixelSize(10);
    painter.setFont(font);
    painter.setPen(QColor(255, 150, 0));
    painter.drawText(valueRect, Qt::AlignCenter, valueText);
}

void VintageKnob::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging_ = true;
        dragStartPos_ = event->position();
        dragStartValue_ = value_;
        event->accept();
    } else {
        event->ignore();
    }
}

void VintageKnob::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging_) {
        // Calculate rotation based on mouse movement
        QPointF center(width() / 2.0, height() / 2.0 - 10);
        QPointF startVector = dragStartPos_ - center;
        QPointF currentVector = event->position() - center;
        
        double startAngle = qAtan2(startVector.y(), startVector.x());
        double currentAngle = qAtan2(currentVector.y(), currentVector.x());
        double deltaAngle = currentAngle - startAngle;
        
        // Convert angle to value change
        double angleRange = qDegreesToRadians(endAngle_ - startAngle_);
        double valueRange = maximum_ - minimum_;
        double deltaValue = (deltaAngle / angleRange) * valueRange;
        
        updateValue(dragStartValue_ + deltaValue);
        event->accept();
    } else {
        event->ignore();
    }
}

void VintageKnob::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && isDragging_) {
        isDragging_ = false;
        event->accept();
    } else {
        event->ignore();
    }
}

void VintageKnob::wheelEvent(QWheelEvent* event) {
    double step = (maximum_ - minimum_) / 100.0;
    double delta = event->angleDelta().y() / 120.0 * step;
    updateValue(value_ + delta);
    event->accept();
}

void VintageKnob::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    isHovered_ = true;
    update();
}

void VintageKnob::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    isHovered_ = false;
    update();
}

double VintageKnob::angleToValue(double angle) const {
    double normalizedAngle = (angle - startAngle_) / (endAngle_ - startAngle_);
    return minimum_ + normalizedAngle * (maximum_ - minimum_);
}

double VintageKnob::valueToAngle(double value) const {
    double normalizedValue = (value - minimum_) / (maximum_ - minimum_);
    return startAngle_ + normalizedValue * (endAngle_ - startAngle_);
}

double VintageKnob::normalizeValue(double value) const {
    if (wrapping_) {
        double range = maximum_ - minimum_;
        while (value < minimum_) value += range;
        while (value > maximum_) value -= range;
        return value;
    } else {
        return qBound(minimum_, value, maximum_);
    }
}

void VintageKnob::updateValue(double newValue) {
    double normalized = normalizeValue(newValue);
    setValue(normalized);
}
