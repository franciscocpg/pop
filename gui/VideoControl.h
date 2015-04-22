#ifndef __VIDEOCONTROL_H_INCL__
    #define __VIDEOCONTROL_H_INCL__

    #include <QFrame>
    #include <QTimer>
    #include <QSlider>
    #include "commontypes.h"
    #include "SubComboBox.h"
    #include "ChromeCast.h"


class VolumeSlider : public QSlider
{
    Q_OBJECT
public:
    VolumeSlider() : QSlider( Qt::Horizontal ) { }
    public slots:
    void provide() { emit valueChanged( value() ); }
};

class SeekControl;
class QSlider;
class VlcMediaPlayer;
class GlyphButton;
class QPushButton;

class QLabel;

class VideoControl : public QFrame
{
    Q_OBJECT

public:
    VideoControl( QWidget *parent = 0 );
    QPushButton* fullscreenButton();

    signals:
    void uiShow();
    void uiHide();
    void volumeSet( int volume );
    void paused();
    void volumeChanged( int );
    void newImage( QString resourcePng );

    public slots:
    void resetPlayback();
    void onPlaying();
    void updateDownloadStatus( DownloadInfo info );
    void showForDefaultPeriod();
    void showForPeriod( unsigned int ms );
    void unshow();
    void addSubtitle( SubtitleItem item );
    void setVolume( int volume );
    void reconnectMediaPlayer();
    void pauseReq();
    void pauseToggleReq();
    void castSwitched( bool casting );
    void provideControlSettings();
    void updateLabel( const QString& newText );

    private slots:
    void volumeUp();
    void volumeDown();

protected:
    virtual void enterEvent( QEvent * );
//  virtual void leaveEvent( QEvent * );

    QFrame *m_hidableFrame;
    SeekControl *m_seek;
    VolumeSlider *m_vol;
    QLabel *m_statusLabel;
    GlyphButton *m_pauseBtn;
    GlyphButton *m_muteBtn;
    GlyphButton *m_fullscrBtn;
    GlyphButton *m_castBtn;
    SubComboBox *m_subs;
    QTimer timer;
    ChromeCast cast;
    QList<QMetaObject::Connection> connList;
};

#endif // __VIDEOCONTROL_H_INCL__
