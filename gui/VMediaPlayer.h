#ifndef __MEDIAPLAYER_H_INCL__
    #define __MEDIAPLAYER_H_INCL__

    #include <vlc-qt/MediaPlayer.h>
    #include <QString>

class VMediaPlayer : public VlcMediaPlayer
{
    Q_OBJECT

public:
    VMediaPlayer( VlcInstance *instance );
    ~VMediaPlayer();
    QString getFileName() { return _fileName; }

    public slots:
    void closeFile();
    void openFile( QString fileName, VlcVideoDelegate& widget );

    void setSubtitleFile( QString fileName );
    void setMute( bool mute );
    void setTime( int time );
    void setVolume( int volume );
    void setPause( bool );

    private slots:
    void onStateChange();

    signals:
    void stateText( const QString& text );
    void stateImage( QString resourcePng );

private:
    VlcMedia *_media;
    QString _fileName;
};

#endif // __MEDIAPLAYER_H_INCL__
