#ifndef VINTAGEKNOB_H
#define VINTAGEKNOB_H

#include <QWidget>
#include <QPointF>

class VintageKnob : public QWidget {
    Q_OBJECT
    
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(QString label READ label WRITE setLabel)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)
    
public:
    explicit VintageKnob(QWidget* parent = nullptr);
    
    double value() const { return value_; }
    double minimum() const { return minimum_; }
    double maximum() const { return maximum_; }
    QString label() const { return label_; }
    bool wrapping() const { return wrapping_; }
    
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
    void setWrapping(bool wrap);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private:
    double value_;
    double minimum_;
    double maximum_;
    QString label_;
    bool wrapping_;
    
    bool isDragging_;
    QPointF dragStartPos_;
    double dragStartValue_;
    bool isHovered_;
    
    // Visual parameters
    static constexpr int knobDiameter_ = 80;
    static constexpr int notchLength_ = 10;
    static constexpr double startAngle_ = -135.0; // degrees
    static constexpr double endAngle_ = 135.0;
    
    // Helper methods
    double angleToValue(double angle) const;
    double valueToAngle(double value) const;
    double normalizeValue(double value) const;
    void updateValue(double newValue);
    
    // Drawing methods
    void drawKnob(QPainter& painter, const QRect& rect);
    void drawPointer(QPainter& painter, const QRect& rect, double angle);
    void drawScale(QPainter& painter, const QRect& rect);
    void drawLabel(QPainter& painter, const QRect& rect);
};

#endif // VINTAGEKNOB_H
