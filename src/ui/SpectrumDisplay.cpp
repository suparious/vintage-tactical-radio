#include "SpectrumDisplay.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <algorithm>
#include <cmath>

SpectrumDisplay::SpectrumDisplay(QWidget* parent)
    : QWidget(parent)
    , displayMode_(BOTH)
    , averaging_(4)
    , intensity_(1.0f)
    , waterfallPos_(0)
    , persistenceEnabled_(true)
    , phosphorDecay_(0.95f)
    , colorScheme_(0)
    , minDb_(-100.0f)
    , maxDb_(0.0f) {
    
    setMinimumSize(400, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Initialize with black background
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);
}

void SpectrumDisplay::updateSpectrum(const float* data, size_t length) {
    // Update spectrum data
    spectrumData_.assign(data, data + length);
    
    // Update averaging buffer
    if (averaging_ > 1) {
        averageBuffer_.push_back(spectrumData_);
        if (averageBuffer_.size() > static_cast<size_t>(averaging_)) {
            averageBuffer_.pop_front();
        }
        
        // Calculate average
        averagedData_.resize(length, 0.0f);
        for (const auto& buffer : averageBuffer_) {
            for (size_t i = 0; i < length && i < buffer.size(); i++) {
                averagedData_[i] += buffer[i];
            }
        }
        
        float scale = 1.0f / averageBuffer_.size();
        for (float& value : averagedData_) {
            value *= scale;
        }
        
        spectrumData_ = averagedData_;
    }
    
    // Update waterfall
    if (displayMode_ == WATERFALL || displayMode_ == BOTH) {
        updateWaterfall();
    }
    
    // Update phosphor
    if (persistenceEnabled_) {
        updatePhosphor();
    }
    
    update();
}

void SpectrumDisplay::setDisplayMode(DisplayMode mode) {
    displayMode_ = mode;
    update();
}

void SpectrumDisplay::setAveraging(int samples) {
    averaging_ = qBound(1, samples, 32);
    averageBuffer_.clear();
    averagedData_.clear();
}

void SpectrumDisplay::setIntensity(float intensity) {
    intensity_ = qBound(0.1f, intensity, 2.0f);
    update();
}

void SpectrumDisplay::clear() {
    spectrumData_.clear();
    averagedData_.clear();
    averageBuffer_.clear();
    phosphorData_.clear();
    waterfallImage_ = QImage();
    waterfallPos_ = 0;
    update();
}

void SpectrumDisplay::setPersistence(bool enable) {
    persistenceEnabled_ = enable;
    if (!enable) {
        phosphorData_.clear();
    }
}

void SpectrumDisplay::setPhosphorDecay(float rate) {
    phosphorDecay_ = qBound(0.5f, rate, 0.99f);
}

void SpectrumDisplay::setColorScheme(int scheme) {
    colorScheme_ = scheme;
    update();
}

QSize SpectrumDisplay::sizeHint() const {
    return QSize(600, 300);
}

QSize SpectrumDisplay::minimumSizeHint() const {
    return QSize(300, 150);
}

void SpectrumDisplay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect displayRect = rect();
    
    // Draw background
    drawBackground(painter, displayRect);
    
    // Draw based on mode
    if (displayMode_ == SPECTRUM) {
        drawSpectrum(painter, displayRect);
    } else if (displayMode_ == WATERFALL) {
        drawWaterfall(painter, displayRect);
    } else { // BOTH
        QRect spectrumRect = displayRect;
        spectrumRect.setHeight(displayRect.height() / 2);
        
        QRect waterfallRect = displayRect;
        waterfallRect.setTop(displayRect.height() / 2);
        
        drawSpectrum(painter, spectrumRect);
        drawWaterfall(painter, waterfallRect);
    }
    
    // Draw grid overlay
    drawGrid(painter, displayRect);
}

void SpectrumDisplay::drawBackground(QPainter& painter, const QRect& rect) {
    // Phosphor-style CRT background
    QLinearGradient bgGradient(rect.topLeft(), rect.bottomLeft());
    bgGradient.setColorAt(0.0, QColor(0, 10, 0));
    bgGradient.setColorAt(1.0, QColor(0, 5, 0));
    
    painter.fillRect(rect, bgGradient);
    
    // Add scanline effect
    painter.setPen(QPen(QColor(0, 20, 0, 30), 1));
    for (int y = 0; y < rect.height(); y += 2) {
        painter.drawLine(rect.left(), y, rect.right(), y);
    }
}

void SpectrumDisplay::drawSpectrum(QPainter& painter, const QRect& rect) {
    if (spectrumData_.empty()) return;
    
    // Draw phosphor persistence layer
    if (persistenceEnabled_) {
        drawPhosphor(painter, rect);
    }
    
    // Draw current spectrum
    QPainterPath spectrumPath;
    
    float xScale = static_cast<float>(rect.width()) / spectrumData_.size();
    
    for (size_t i = 0; i < spectrumData_.size(); i++) {
        float x = rect.left() + i * xScale;
        float y = rect.bottom() - dbToPixel(spectrumData_[i], rect.height());
        
        if (i == 0) {
            spectrumPath.moveTo(x, y);
        } else {
            spectrumPath.lineTo(x, y);
        }
    }
    
    // Draw spectrum line
    painter.setPen(QPen(QColor(0, 255, 0), 2));
    painter.drawPath(spectrumPath);
    
    // Fill under curve with gradient
    spectrumPath.lineTo(rect.right(), rect.bottom());
    spectrumPath.lineTo(rect.left(), rect.bottom());
    spectrumPath.closeSubpath();
    
    QLinearGradient fillGradient(rect.topLeft(), rect.bottomLeft());
    fillGradient.setColorAt(0.0, QColor(0, 255, 0, 100));
    fillGradient.setColorAt(1.0, QColor(0, 255, 0, 0));
    
    painter.fillPath(spectrumPath, fillGradient);
}

void SpectrumDisplay::drawWaterfall(QPainter& painter, const QRect& rect) {
    if (!waterfallImage_.isNull()) {
        // Scale image to fit rect
        QImage scaled = waterfallImage_.scaled(rect.size(), 
                                              Qt::IgnoreAspectRatio, 
                                              Qt::SmoothTransformation);
        painter.drawImage(rect, scaled);
    }
}

void SpectrumDisplay::drawGrid(QPainter& painter, const QRect& rect) {
    painter.setPen(QPen(QColor(0, 100, 0, 100), 1, Qt::DotLine));
    
    // Vertical grid lines (frequency)
    int numVLines = 10;
    for (int i = 1; i < numVLines; i++) {
        int x = rect.left() + (rect.width() * i) / numVLines;
        painter.drawLine(x, rect.top(), x, rect.bottom());
    }
    
    // Horizontal grid lines (dB)
    int numHLines = 5;
    for (int i = 1; i < numHLines; i++) {
        int y = rect.top() + (rect.height() * i) / numHLines;
        painter.drawLine(rect.left(), y, rect.right(), y);
    }
    
    // Draw dB scale
    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);
    painter.setPen(QColor(0, 200, 0));
    
    for (int i = 0; i <= numHLines; i++) {
        int y = rect.top() + (rect.height() * i) / numHLines;
        float db = maxDb_ - (maxDb_ - minDb_) * i / numHLines;
        painter.drawText(rect.left() + 5, y + 3, 
                        QString("%1 dB").arg(static_cast<int>(db)));
    }
}

void SpectrumDisplay::drawPhosphor(QPainter& painter, const QRect& rect) {
    if (phosphorData_.size() != spectrumData_.size()) {
        phosphorData_.resize(spectrumData_.size(), minDb_);
    }
    
    // Draw phosphor persistence
    QPainterPath phosphorPath;
    float xScale = static_cast<float>(rect.width()) / phosphorData_.size();
    
    for (size_t i = 0; i < phosphorData_.size(); i++) {
        float x = rect.left() + i * xScale;
        float y = rect.bottom() - dbToPixel(phosphorData_[i], rect.height());
        
        if (i == 0) {
            phosphorPath.moveTo(x, y);
        } else {
            phosphorPath.lineTo(x, y);
        }
    }
    
    // Draw with fading green phosphor effect
    painter.setPen(QPen(QColor(0, 180, 0, 150), 1));
    painter.drawPath(phosphorPath);
}

void SpectrumDisplay::updateWaterfall() {
    if (waterfallImage_.isNull() || static_cast<size_t>(waterfallImage_.width()) != spectrumData_.size()) {
        // Initialize waterfall image
        waterfallImage_ = QImage(spectrumData_.size(), 256, QImage::Format_RGB32);
        waterfallImage_.fill(Qt::black);
        waterfallPos_ = 0;
    }
    
    // Add new line to waterfall
    for (size_t i = 0; i < spectrumData_.size(); i++) {
        QColor color = valueToColor(spectrumData_[i]);
        waterfallImage_.setPixelColor(i, waterfallPos_, color);
    }
    
    // Advance position
    waterfallPos_ = (waterfallPos_ + 1) % waterfallImage_.height();
}

void SpectrumDisplay::updatePhosphor() {
    if (phosphorData_.size() != spectrumData_.size()) {
        phosphorData_.resize(spectrumData_.size(), minDb_);
    }
    
    // Update phosphor with decay
    for (size_t i = 0; i < spectrumData_.size(); i++) {
        if (spectrumData_[i] > phosphorData_[i]) {
            phosphorData_[i] = spectrumData_[i];
        } else {
            phosphorData_[i] = phosphorData_[i] * phosphorDecay_ + 
                              minDb_ * (1.0f - phosphorDecay_);
        }
    }
}

QColor SpectrumDisplay::valueToColor(float value) {
    // Normalize value to 0-1 range
    float normalized = (value - minDb_) / (maxDb_ - minDb_);
    normalized = qBound(0.0f, normalized, 1.0f);
    normalized *= intensity_;
    
    // Apply color scheme
    switch (colorScheme_) {
        case 0: // Classic green phosphor
            return QColor(0, static_cast<int>(255 * normalized), 0);
            
        case 1: // Heat map
            if (normalized < 0.25f) {
                return QColor(0, 0, static_cast<int>(255 * normalized * 4));
            } else if (normalized < 0.5f) {
                return QColor(0, static_cast<int>(255 * (normalized - 0.25f) * 4), 255);
            } else if (normalized < 0.75f) {
                return QColor(static_cast<int>(255 * (normalized - 0.5f) * 4), 255, 
                            255 - static_cast<int>(255 * (normalized - 0.5f) * 4));
            } else {
                return QColor(255, 255 - static_cast<int>(255 * (normalized - 0.75f) * 4), 0);
            }
            
        case 2: // Grayscale
            {
                int gray = static_cast<int>(255 * normalized);
                return QColor(gray, gray, gray);
            }
            
        default:
            return QColor(0, static_cast<int>(255 * normalized), 0);
    }
}

float SpectrumDisplay::dbToPixel(float db, int height) {
    float normalized = (db - minDb_) / (maxDb_ - minDb_);
    return height * qBound(0.0f, normalized, 1.0f);
}

void SpectrumDisplay::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    
    // Reset waterfall image on resize
    waterfallImage_ = QImage();
    waterfallPos_ = 0;
}
