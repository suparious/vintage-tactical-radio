#ifndef FREQUENCYDIAL_H
#define FREQUENCYDIAL_H

#include <QWidget>
#include <QTimer>

class FrequencyDial : public QWidget {
    Q_OBJECT
    
    Q_PROPERTY(double frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
    
public:
    explicit FrequencyDial(QWidget* parent = nullptr);
    
    double frequency() const { return frequency_; }
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    
signals:
    void frequencyChanged(double frequency);
    
public slots:
    void setFrequency(double frequency);
    void stepUp();
    void stepDown();
    void setStepSize(double stepSize);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    double frequency_;
    double stepSize_;
    double displayFrequency_;
    
    // Animation
    QTimer* animationTimer_;
    static constexpr double animationSpeed_ = 0.15;
    
    // Interaction
    bool isDragging_;
    QPoint dragStartPos_;
    double dragStartFrequency_;
    
    // Visual parameters
    int dialDiameter_;
    double dialRotation_;
    
    // Drawing methods
    void drawBackground(QPainter& painter, const QRect& rect);
    void drawDial(QPainter& painter, const QRect& rect);
    void drawFrequencyDisplay(QPainter& painter, const QRect& rect);
    void drawDigitalDisplay(QPainter& painter, const QRect& rect);
    void drawGlass(QPainter& painter, const QRect& rect);
    
    // Helper methods
    double frequencyToRotation(double freq) const;
    QString formatFrequency(double freq) const;
    void updateAnimation();
};

#endif // FREQUENCYDIAL_H
