#ifndef VINTAGEMETER_H
#define VINTAGEMETER_H

#include <QWidget>
#include <QTimer>
#include <deque>

class VintageMeter : public QWidget {
    Q_OBJECT
    
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(QString label READ label WRITE setLabel)
    Q_PROPERTY(bool peakHold READ peakHold WRITE setPeakHold)
    
public:
    explicit VintageMeter(QWidget* parent = nullptr);
    
    double value() const { return currentValue_; }
    double minimum() const { return minimum_; }
    double maximum() const { return maximum_; }
    QString label() const { return label_; }
    bool peakHold() const { return peakHold_; }
    
    void setRange(double min, double max);
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    
signals:
    void valueChanged(double value);
    
public slots:
    void setValue(double value);
    void setMinimum(double min);
    void setMaximum(double max);
    void setLabel(const QString& label);
    void setPeakHold(bool enable);
    void resetPeak();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    void updateAnimation();
    void decayPeak();
    
private:
    // Values
    double minimum_;
    double maximum_;
    double currentValue_;
    double displayValue_;
    double targetValue_;
    double peakValue_;
    QString label_;
    
    // Features
    bool peakHold_;
    
    // Animation
    QTimer* animationTimer_;
    QTimer* peakDecayTimer_;
    static constexpr double dampingFactor_ = 0.15;
    static constexpr double peakDecayRate_ = 0.5; // dB per second
    
    // Visual parameters
    static constexpr int needleLength_ = 80;
    static constexpr double startAngle_ = -60.0; // degrees from vertical
    static constexpr double endAngle_ = 60.0;
    
    // Drawing methods
    void drawBackground(QPainter& painter, const QRect& rect);
    void drawScale(QPainter& painter, const QRect& rect);
    void drawNeedle(QPainter& painter, const QRect& rect, double value);
    void drawPeakIndicator(QPainter& painter, const QRect& rect);
    void drawGlass(QPainter& painter, const QRect& rect);
    
    // Helper methods
    double valueToAngle(double value) const;
    QPointF rotatePoint(const QPointF& point, const QPointF& center, double angle) const;
};

#endif // VINTAGEMETER_H
