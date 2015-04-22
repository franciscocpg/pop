#ifndef __CHROMECAST_H_INCL__
    #define __CHROMECAST_H_INCL__

    #include <QObject>
    #include <QByteArray>
    #include <QProcess>
    #include <QList>
    #include <QStringListModel>
    #include <QListWidget>
    #include <QHostAddress>
    #include <QUrl>
    #include <QFrame>
    #include <QNetworkAccessManager>
    #include <QNetworkReply>
    #include <QBasicTimer>
    #include <QTimer>


class ListWidget : public QListWidget
{
    Q_OBJECT

public:
    QSize sizeHint() const { return contentsSize(); }

};

class Frame : public QFrame
{
    Q_OBJECT

public:
    explicit Frame( QWidget *parent = 0 ) : QFrame( parent, Qt::Popup ) { setFocusPolicy( Qt::ClickFocus ); hide(); }

protected:
    void focusOutEvent( QFocusEvent *event ) { (void) event; close(); }
};

enum CAST_DEVICETYPE
{
    CAST_DEVICETYPE_INVALID = 0,
    CAST_DEVICETYPE_CHROMECAST,
    CAST_DEVICETYPE_AIRPLAY,
    CAST_DEVICETYPE_LOCAL
};

const char *const CAST_DEVICETYPE_STRING[] =
{
    "",
    "googlecast",
    "airplay",
    ""
};

const char *const CAST_DEVICETYPE_USER_STRING[] =
{
    "",
    "Google Cast",
    "AirPlay",
    ""
};

struct CastDevice
{
public:
    CastDevice();
    CastDevice( const QJsonObject& obj );
    bool operator==( const CastDevice& src )  { return src.type == type && src.name == name && src.ip == ip; }
    bool isValid() { return type != CAST_DEVICETYPE_INVALID && name.size() && !ip.isNull() && url.size(); }
    CAST_DEVICETYPE type;

public:
    QByteArray name;
    QHostAddress ip;
    QByteArray url;
};


class GlyphButton;
class QMovie;
class QListWidgetItem;
class NjsProcess;

class ChromeCast : public QObject
{
    Q_OBJECT

    enum State
    {
        Idle = 0,
        Opening,
        Playing,
        Paused,
    };

public:
    ChromeCast( QWidget *parent = 0 );
    void setButton( GlyphButton *btn );
    State state() { return m_state; }

    public slots:
    void buttonClicked();
    void stop();
    void pause();
    void setTime( int );
    void setMute( bool );
    void setPause( bool );
    void setVolume( int );

    private slots:
    void frameChange( int frame );
    void parseNjsData( QByteArray key, QByteArray value );
    void discoveryStopped( int exitCode, QProcess::ExitStatus exitStatus );
    void serverStopped( int exitCode, QProcess::ExitStatus exitStatus );
    void onListItemClicked( QListWidgetItem *item );
    void onReqestFinish();
    void onVolumeTimer();

    signals:
    void lengthChanged( int );
    void timeChanged( int );
    void paused();
    void playing();
    void deviceSelected();

    void stateText( const QString& text );
    void stateImage( QString resourcePng );
    void castingSwitched( bool casting );
    void stateChanged();

protected:
    virtual void timerEvent( QTimerEvent * );

private:
    void parseDiscovery( QByteArray data );
    void parseMStat( QByteArray data );
    void startDiscovery( bool crashed );
    void handleCastingSwitch( bool switchedToEnable );
    QNetworkReply* issueCommand( QByteArray command );
    void setState( State newState );

    QNetworkAccessManager netMan;
    QTimer volumeTimer;
    int lastVolume;
    NjsProcess *discoveryProc;
    int discoveryRestartCounter;
    NjsProcess *castProc;
    Frame *popup;
    ListWidget *view;
    GlyphButton *castBtn;
    QMovie *btnMovie;
    QList<CastDevice> devs;
    QStringListModel model;
    QUrl serverUrl;
    CastDevice currentDevice;
    bool isCasting;
    float totalLength;
    QBasicTimer stateRefreshtimer;
    State m_state;
};

#endif // __CHROMECAST_H_INCL__
