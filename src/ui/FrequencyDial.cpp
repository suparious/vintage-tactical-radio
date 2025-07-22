#include "FrequencyDial.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QtMath>

FrequencyDial::FrequencyDial(QWidget* parent)
    : QWidget(parent)
    , frequency_(96900000) // 96.9 MHz
    , stepSize_(100000)    // 100 kHz
    , displayFrequency_(frequency_)
    , isDragging_(false)
    , dialRotation_(0.0) {
    
    setMinimumSize(200, 250);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    
    // Setup animation timer
    animationTimer_ = new QTimer(this);
    animationTimer_->setInterval(20); // 50 FPS
    connect(animationTimer_, &QTimer::timeout, this, &FrequencyDial::updateAnimation);
    animationTimer_->start();
}

void FrequencyDial::setFrequency(double frequency) {
    if (qFuzzyCompare(frequency, frequency_)) return;
    
    frequency_ = qBound(500000.0, frequency, 1700000000.0); // 500 kHz to 1.7 GHz
    emit frequencyChanged(frequency_);
}

void FrequencyDial::stepUp() {
    setFrequency(frequency_ + stepSize_);
}

void FrequencyDial::stepDown() {
    setFrequency(frequency_ - stepSize_);
}

void FrequencyDial::setStepSize(double stepSize) {
    stepSize_ = stepSize;
}

QSize FrequencyDial::sizeHint() const {
    return QSize(250, 300);
}

QSize FrequencyDial::minimumSizeHint() const {
    return QSize(200, 250);
}

void FrequencyDial::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect dialRect = rect().adjusted(20, 20, -20, -80);
    
    // Draw background
    drawBackground(painter, rect());
    
    // Draw the main dial
    drawDial(painter, dialRect);
    
    // Draw frequency display
    drawFrequencyDisplay(painter, rect());
    
    // Draw digital display
    drawDigitalDisplay(painter, rect());
    
    // Draw glass effect
    drawGlass(painter, dialRect);
}

void FrequencyDial::drawBackground(QPainter& painter, const QRect& rect) {
    // Panel background
    QLinearGradient bgGradient(rect.topLeft(), rect.bottomLeft());
    bgGradient.setColorAt(0.0, QColor(50, 50, 45));
    bgGradient.setColorAt(1.0, QColor(30, 30, 25));
    
    painter.setPen(QPen(QColor(20, 20, 20), 2));
    painter.setBrush(bgGradient);
    painter.drawRoundedRect(rect, 8, 8);
    
    // Inner frame
    QRect innerRect = rect.adjusted(10, 10, -10, -10);
    painter.setPen(QPen(QColor(80, 80, 75), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(innerRect, 5, 5);
}

void FrequencyDial::drawDial(QPainter& painter, const QRect& rect) {
    // Dial face background
    QRadialGradient dialGradient(rect.center(), rect.width() / 2);
    dialGradient.setColorAt(0.0, QColor(240, 235, 220));
    dialGradient.setColorAt(0.8, QColor(230, 225, 210));
    dialGradient.setColorAt(1.0, QColor(200, 195, 180));
    
    painter.setPen(QPen(QColor(100, 100, 95), 2));
    painter.setBrush(dialGradient);
    painter.drawEllipse(rect);
    
    // Draw frequency scale
    painter.save();
    painter.translate(rect.center());
    painter.rotate(dialRotation_);
    
    // Major frequency marks
    painter.setPen(QPen(QColor(60, 60, 60), 2));
    QFont font = painter.font();
    font.setFamily("Arial");
    font.setPixelSize(10);
    painter.setFont(font);
    
    // Draw logarithmic scale
    for (int i = 0; i < 360; i += 30) {
        painter.save();
        painter.rotate(i);
        
        // Major tick
        painter.drawLine(0, -rect.width() / 2 + 5, 0, -rect.width() / 2 + 15);
        
        // Frequency label (simplified for display)
        if (i % 60 == 0) {
            painter.translate(0, -rect.width() / 2 + 30);
            painter.rotate(-i - dialRotation_);
            
            int freqMHz = 88 + (i / 30);
            painter.drawText(QRect(-20, -10, 40, 20), 
                           Qt::AlignCenter, 
                           QString::number(freqMHz));
        }
        
        painter.restore();
    }
    
    // Minor marks
    painter.setPen(QPen(QColor(120, 120, 120), 1));
    for (int i = 0; i < 360; i += 6) {
        if (i % 30 != 0) {
            painter.save();
            painter.rotate(i);
            painter.drawLine(0, -rect.width() / 2 + 5, 0, -rect.width() / 2 + 10);
            painter.restore();
        }
    }
    
    painter.restore();
    
    // Center hub
    QRadialGradient hubGradient(rect.center(), 30);
    hubGradient.setColorAt(0.0, QColor(100, 100, 95));
    hubGradient.setColorAt(0.7, QColor(80, 80, 75));
    hubGradient.setColorAt(1.0, QColor(60, 60, 55));
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(hubGradient);
    painter.drawEllipse(rect.center(), 25, 25);
    
    // Pointer indicator (fixed at top)
    QPainterPath pointer;
    pointer.moveTo(rect.center().x() - 10, rect.top() + 20);
    pointer.lineTo(rect.center().x(), rect.top() + 5);
    pointer.lineTo(rect.center().x() + 10, rect.top() + 20);
    pointer.closeSubpath();
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 100, 0));
    painter.drawPath(pointer);
}

void FrequencyDial::drawFrequencyDisplay(QPainter& painter, const QRect& rect) {
    // Mechanical counter style display
    QRect displayRect(rect.left() + 30, rect.bottom() - 70, rect.width() - 60, 30);
    
    // Display background
    painter.setPen(QPen(QColor(40, 40, 35), 1));
    painter.setBrush(QColor(20, 20, 15));
    painter.drawRoundedRect(displayRect, 3, 3);
    
    // Display digits
    QString freqText = formatFrequency(displayFrequency_);
    
    QFont font;
    font.setFamily("Courier");
    font.setPixelSize(20);
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    
    // Draw each digit in its own cell
    int digitWidth = displayRect.width() / freqText.length();
    for (int i = 0; i < freqText.length(); i++) {
        QRect digitRect(displayRect.left() + i * digitWidth, 
                       displayRect.top(),
                       digitWidth,
                       displayRect.height());
        
        // Cell background
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(30, 30, 25));
        painter.drawRect(digitRect.adjusted(2, 2, -2, -2));
        
        // Digit
        painter.setPen(QColor(255, 150, 0));
        painter.drawText(digitRect, Qt::AlignCenter, freqText[i]);
    }
}

void FrequencyDial::drawDigitalDisplay(QPainter& painter, const QRect& rect) {
    // Secondary digital display
    QRect digitalRect(rect.left() + 30, rect.bottom() - 35, rect.width() - 60, 25);
    
    QFont font;
    font.setFamily("Arial");
    font.setPixelSize(12);
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    
    painter.setPen(QColor(180, 180, 180));
    
    QString digitalText;
    if (frequency_ >= 1e9) {
        digitalText = QString("%1 GHz").arg(frequency_ / 1e9, 0, 'f', 3);
    } else if (frequency_ >= 1e6) {
        digitalText = QString("%1 MHz").arg(frequency_ / 1e6, 0, 'f', 3);
    } else {
        digitalText = QString("%1 kHz").arg(frequency_ / 1e3, 0, 'f', 1);
    }
    
    painter.drawText(digitalRect, Qt::AlignCenter, digitalText);
}

void FrequencyDial::drawGlass(QPainter& painter, const QRect& rect) {
    // Glass reflection on dial
    QRect glassRect = rect.adjusted(10, 10, -10, -rect.height() / 2);
    QLinearGradient glassGradient(glassRect.topLeft(), glassRect.bottomLeft());
    glassGradient.setColorAt(0.0, QColor(255, 255, 255, 40));
    glassGradient.setColorAt(1.0, QColor(255, 255, 255, 0));
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(glassGradient);
    painter.drawEllipse(glassRect);
}

void FrequencyDial::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        isDragging_ = true;
        dragStartPos_ = event->pos();
        dragStartFrequency_ = frequency_;
        event->accept();
    } else {
        event->ignore();
    }
}

void FrequencyDial::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging_) {
        // Calculate rotation based on vertical mouse movement
        int deltaY = event->pos().y() - dragStartPos_.y();
        
        // Each pixel of movement = 10 kHz
        double deltaFreq = -deltaY * 10000;
        setFrequency(dragStartFrequency_ + deltaFreq);
        
        event->accept();
    } else {
        event->ignore();
    }
}

void FrequencyDial::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && isDragging_) {
        isDragging_ = false;
        event->accept();
    } else {
        event->ignore();
    }
}

void FrequencyDial::wheelEvent(QWheelEvent* event) {
    double delta = event->angleDelta().y() / 120.0;
    setFrequency(frequency_ + delta * stepSize_);
    event->accept();
}

void FrequencyDial::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Up:
            stepUp();
            break;
        case Qt::Key_Down:
            stepDown();
            break;
        case Qt::Key_PageUp:
            setFrequency(frequency_ + stepSize_ * 10);
            break;
        case Qt::Key_PageDown:
            setFrequency(frequency_ - stepSize_ * 10);
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
}

QString FrequencyDial::formatFrequency(double freq) const {
    // Format as XXX.XXX
    int mhz = static_cast<int>(freq / 1e6);
    int khz = static_cast<int>((freq - mhz * 1e6) / 1e3);
    return QString("%1.%2").arg(mhz, 3, 10, QChar('0'))
                           .arg(khz, 3, 10, QChar('0'));
}

double FrequencyDial::frequencyToRotation(double freq) const {
    // Convert frequency to dial rotation angle
    // This is simplified - real radios use logarithmic scales
    double logFreq = log10(freq);
    double logMin = log10(500000);    // 500 kHz
    double logMax = log10(1700000000); // 1.7 GHz
    
    double normalized = (logFreq - logMin) / (logMax - logMin);
    return normalized * 360.0 * 3; // 3 full rotations across the range
}

void FrequencyDial::updateAnimation() {
    // Smooth dial rotation
    double targetRotation = frequencyToRotation(frequency_);
    double diff = targetRotation - dialRotation_;
    
    // Handle wrap-around
    if (diff > 180) diff -= 360;
    if (diff < -180) diff += 360;
    
    if (qAbs(diff) > 0.1) {
        dialRotation_ += diff * animationSpeed_;
        update();
    }
    
    // Smooth frequency display
    double freqDiff = frequency_ - displayFrequency_;
    if (qAbs(freqDiff) > 1) {
        displayFrequency_ += freqDiff * animationSpeed_;
        update();
    }
}
