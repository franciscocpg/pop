#ifndef __HOSTAPP_H_INCL__
    #define __HOSTAPP_H_INCL__

    #include <QObject>
    #include <QThread>
    #include <QSettings>
    #include <QNetworkAccessManager>
    #include <QNetworkReply>

    #include "commontypes.h"
    #include "config.h"

class TorrentSubtitlesReply : public QObject
{
    Q_OBJECT

public:
    TorrentSubtitlesReply( QNetworkReply *reply, int theIndex, SubtitleItem sub ) :
       m_reply( reply ), torrentIndex( theIndex ), item( sub )
    {
        connect( m_reply, SIGNAL( finished() ), SLOT( onFinished() ) );
    }
    ~TorrentSubtitlesReply() { delete m_reply; }

    private slots:
    void onFinished()
    {
        if ( !m_reply->error() )
        {
            item.zipData = m_reply->readAll();
            emit finished( item );
        }
        deleteLater();
    }
    signals:
    void finished( SubtitleItem );

protected:
    QNetworkReply *m_reply;
    int torrentIndex;
    SubtitleItem item;
};



class HostApp;

class DownloadNotifyThread : public QThread
{
    Q_OBJECT

public:
    DownloadNotifyThread( HostApp *hostApp );
    ~DownloadNotifyThread();

    signals:
    void torrentProgressAvailable( int progressPeers, int dlSpeed, int seeds, int peers, QString msg );
    void torrentUrlAvailable( QString url );
    void downloadInfoChanged( DownloadInfo info );
    public slots:
    void startOnTorrent( int torrentId );
    void stopOnTorrent();

protected:
    void run();

private:
    volatile int tId;
};




class QGraphicsWebView;
class QWebFrame;
class QWidget;

class HostApp : public QObject
{
    Q_OBJECT

public:
    HostApp( QGraphicsWebView *view, QWidget *parent );
    ~HostApp();

    DownloadNotifyThread* thread() { return &m_thread; }
    int getVolume();

    public slots:
    void setProxy( ProxySettings proxy );
    void setVolume( int volume );

    // JS callback slots

    bool sendWinAction( QString str );
    QString getTorrent( QString url, QString fileName, QString subtitles_url );
    bool url_request( QString url );
    bool cancelTorrent( QString ATorrentId );

    bool vpn_isConnected();
    bool vpn_connect();
    bool vpn_disconnect();

    bool setConfig( QString config );

    QString getTempPath();
    QString setTempPath();
    bool OpenTempDir();
    bool setCleanOnExit( int isClear );
    int getUserFontSize() { return fontSize; }
    void setUserFontSize( int size );

    bool openBrowser( QString url );

    private slots:
    void attachObject();
    bool cancelTorrent( int index );
    bool cancelTorrent();

    // JS control slots
    void hideTorrentInfo();
    void updateTorrentProgress( int progressPeers, int dlSpeed, int seeds, int peers, QString msg );
    void vpnConnected();
    void vpnDisconnected();
    void hideLoading();
    void updateSubtitleThumbs();

    signals:
    void commandedMinimize();
    void toggledMaximize();
    void commandedClose();
    void haveSubtitles( SubtitleItem item );
    void downloadInfoChanged( DownloadInfo info );

private:
    QWebFrame *frame;
    DownloadNotifyThread m_thread;
    AppConfig *settings;
    QString downloadDir;
    bool cleanOnExit;
    int fontSize;
    QNetworkAccessManager netMan;
    int activeTorrent;
    TProxyList proxies;
};


#endif // __HOSTAPP_H_INCL__
