#ifndef __SEEKSLIDER_H_INCL__
    #define __SEEKSLIDER_H_INCL__

    #include <QtCore/QPoint>
    #include <QtWidgets/QWidget>
    #include <QtWidgets/QSlider>
    #include "commontypes.h"

class QLabel;
class QTimer;
class VlcMediaPlayer;


class SeekSlider : public QSlider
{
    Q_OBJECT

public:
    SeekSlider( Qt::Orientation orientation, QWidget *parent = 0 );
    void updateCurrentSize( quint64 size );
    void updateFullSize( quint64 size );
    int getCurrentSize() { return currentSize; }
    int getFullSize() { return fullSize; }
    bool canStillPlay( int time );

protected:
    virtual void paintEvent( QPaintEvent *event );
    int fullSize;
    int currentSize;
};

class SeekControl : public QWidget
{
    Q_OBJECT

public:
    explicit SeekControl( QWidget *parent = 0 );
    SeekControl( QLabel *rateLabel );
    ~SeekControl();

    int getCurrentSize() { return _seek->getCurrentSize(); }
    int getFullSize() { return _seek->getFullSize(); }
    void setByteRateLabel( QLabel *lbl ) { byteRateLabel = lbl; }

    public slots:
    void downloadInfoChange( DownloadInfo info );

    private slots:
    void end();
    void updateCurrentTime( int time );
    void updateFullTime( int time );

    signals:
    void sought( int time );
    void paused();
    void needTextUpdate();
    void updateStatusLabel( const QString& text );

protected:
    void mouseMoveEvent( QMouseEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
//  void wheelEvent( QWheelEvent *event );

private:
    void initWidgetSeek();
    void updateEvent( const QPoint& pos );

    void lock() { _lock = true; }
    void unlock() { _lock = false; }

    bool _lock;

    SeekSlider *_seek;
    QLabel *_labelElapsed;
    QLabel *_labelFull;
    QLabel *byteRateLabel;
};

#endif // __SEEKSLIDER_H_INCL__
