#ifndef SPECTRUMDISPLAY_H
#define SPECTRUMDISPLAY_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <vector>
#include <deque>

class SpectrumDisplay : public QWidget {
    Q_OBJECT
    
    Q_PROPERTY(DisplayMode displayMode READ displayMode WRITE setDisplayMode)
    Q_PROPERTY(int averaging READ averaging WRITE setAveraging)
    Q_PROPERTY(float intensity READ intensity WRITE setIntensity)
    
public:
    enum DisplayMode {
        SPECTRUM,
        WATERFALL,
        BOTH
    };
    Q_ENUM(DisplayMode)
    
    explicit SpectrumDisplay(QWidget* parent = nullptr);
    
    DisplayMode displayMode() const { return displayMode_; }
    int averaging() const { return averaging_; }
    float intensity() const { return intensity_; }
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    
public slots:
    void updateSpectrum(const float* data, size_t length);
    void setDisplayMode(DisplayMode mode);
    void setAveraging(int samples);
    void setIntensity(float intensity);
    void clear();
    
    // Display options
    void setPersistence(bool enable);
    void setPhosphorDecay(float rate);
    void setColorScheme(int scheme);
    void setAutoRange(bool enable) { autoRange_ = enable; }
    void setDbRange(float min, float max) { minDb_ = min; maxDb_ = max; autoRange_ = false; }
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private:
    DisplayMode displayMode_;
    int averaging_;
    float intensity_;
    
    // Spectrum data
    std::vector<float> spectrumData_;
    std::vector<float> averagedData_;
    std::deque<std::vector<float>> averageBuffer_;
    
    // Waterfall data
    QImage waterfallImage_;
    int waterfallPos_;
    
    // Phosphor persistence
    std::vector<float> phosphorData_;
    bool persistenceEnabled_;
    float phosphorDecay_;
    
    // Visual parameters
    int colorScheme_;
    float minDb_;
    float maxDb_;
    
    // Auto-ranging
    bool autoRange_;
    float autoRangeMin_;
    float autoRangeMax_;
    float autoRangeAlpha_;
    
    // Drawing methods
    void drawBackground(QPainter& painter, const QRect& rect);
    void drawSpectrum(QPainter& painter, const QRect& rect);
    void drawWaterfall(QPainter& painter, const QRect& rect);
    void drawGrid(QPainter& painter, const QRect& rect);
    void drawPhosphor(QPainter& painter, const QRect& rect);
    
    // Helper methods
    QColor valueToColor(float value);
    void updateWaterfall();
    void updatePhosphor();
    float dbToPixel(float db, int height);
};

#endif // SPECTRUMDISPLAY_H
