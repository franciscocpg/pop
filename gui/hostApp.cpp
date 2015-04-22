#include <QGraphicsWebView>
#include <QWebFrame>
#include <QWebPage>
#include <QWidget>
#include <QFileDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkProxy>

#include "hostApp.h"
#include "defaults.h"
#include "vlc.h"


#ifdef WIN32
    #include "../lib/lib.h"
#endif
#if defined __APPLE__ && defined __MACH__
    #include "lib.h"
#endif

HostApp::HostApp( QGraphicsWebView *view, QWidget *parent ) : QObject( parent ), frame( 0 ), m_thread( this ),
   settings( new AppConfig ), netMan( this ), activeTorrent( -1 )
{
    QWebPage *page = view->page();
    frame = page->mainFrame();

    downloadDir = settings->value( SETTINGS_KEY_DOWNLOAD_DIR ).toString();
    cleanOnExit = settings->value( SETTINGS_KEY_CLEAN_ON_EXIT, CLEAN_ON_EXIT_DEFAULT ).toBool();
    fontSize = settings->value( SETTINGS_KEY_FONT_SIZE, SETTINGS_DEFAULT_FONT_SIZE ).toInt();
    if ( fontSize < -2 || fontSize > 2 ) fontSize = 0;
    setUserFontSize( fontSize );
    if ( downloadDir.isEmpty() ) downloadDir = QDir::homePath() + "/Downloads/PopcornTime";
    attachObject();
    torrentInit( downloadDir.size() ? downloadDir.toLocal8Bit().constData() : 0, cleanOnExit ? 1 : 0 );
    QObject::connect( frame, SIGNAL( javaScriptWindowObjectCleared() ), this, SLOT( attachObject() ) );
    QObject::connect( settings, SIGNAL( newProxySettings( ProxySettings ) ), this, SLOT( setProxy( ProxySettings ) ) );
}

HostApp::~HostApp()
{
    delete settings;
    qDebug( "torrentDeInit start" );
    torrentDeInit();
    qDebug( "torrentDeInit end" );
}

void HostApp::attachObject()
{
    frame->addToJavaScriptWindowObject( QString( "hostApp" ), this );
}



// JS control slots


void HostApp::hideTorrentInfo()     { frame->evaluateJavaScript( "app.torrent.hideLoading()" ); }
void HostApp::vpnConnected()        { frame->evaluateJavaScript( "app.vpn.connected()" ); qDebug() << "app.vpn.connected()"; }
void HostApp::vpnDisconnected()     { frame->evaluateJavaScript( "app.vpn.disconnected()" ); qDebug() << "app.vpn.disconnected()"; }
void HostApp::hideLoading()         { frame->evaluateJavaScript( "app.torrent.hideLoading()" ); }
void HostApp::updateTorrentProgress( int progressPeers, int dlSpeed, int seeds, int peers, QString msg )
{
    QString js = QString( "app.torrent.updateInfo(%1,%2,%3,%4,\"%5\")" )
       .arg( progressPeers ).arg( dlSpeed ).arg( seeds ).arg( peers ).arg( msg );
//  qDebug() << "updateProgress " << js;
    frame->evaluateJavaScript( js );
}

void HostApp::updateSubtitleThumbs()
{
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>( sender() );
    if ( !reply ) return;
    if ( !reply->error() )
    {
        QString js = QString( "utils.url_response[\"%1\"](%2)" )
           .arg( reply->url().toString() )
           .arg( QString( reply->readAll() ) );
//      qDebug() << "updateSubtitles " << js;
        frame->evaluateJavaScript( js );
    }
//  else qDebug() << "url_request finished with error " << reply->error();
    reply->deleteLater();
}



void HostApp::setProxy( ProxySettings proxy )
{
    qDebug() << "setting proxy";
    if ( proxy.isValid() )
    {
        QNetworkProxy::setApplicationProxy( proxy.networkProxy() );

//      frame->page()->networkAccessManager()->setProxy( proxy.networkProxy() ); // setNetworkAccessManager( new QNetworkAccessManager( frame->page() )
        torrentSetProxy( proxy.host.constData(), proxy.port, proxy.user.constData(), proxy.pass.constData() );
        vpnConnected();
        qDebug() << "proxy set" << proxy.host << proxy.port;
    }
    else
    {
        QNetworkProxy::setApplicationProxy( QNetworkProxy( QNetworkProxy::NoProxy ) );
        torrentSetProxy();
        vpnDisconnected();
        qDebug() << "proxy cleared";
    }
}

void HostApp::setVolume( int volume ) { settings->setValue( SETTINGS_KEY_VOLUME, volume ); }
int  HostApp::getVolume() { return settings->value( SETTINGS_KEY_VOLUME, 100 ).toInt(); }

// JS callback slots

bool HostApp::sendWinAction( QString str )
{
    if ( str == "max" ) emit toggledMaximize();
    else if ( str == "min" ) emit commandedMinimize();
    else if ( str == "close" ) emit commandedClose();
    return true;
}

QString HostApp::getTorrent( QString url, QString fileName, QString subtitles_url )
{
    qDebug() << "Get Torrent" << url << fileName << subtitles_url;

    const int index = torrentAddStart( url.toLatin1(), fileName.toLocal8Bit() );
    qDebug() << "torrentAddStart finish" << index;
    if ( index < 0 )
    {
        hideLoading();
        return QString::number( index );
    }

    m_thread.startOnTorrent( index );

//  qDebug() << "requestSubtitles" << subtitles_url;
    QJsonArray jsonArr = QJsonDocument::fromJson( subtitles_url.toUtf8() ).array();
    while ( jsonArr.size() )
    {
        QJsonArray item = jsonArr.takeAt( 0 ).toArray();
        QString sUrl = item.at( 0 ).toString();
        QByteArray abbr =  item.at( 1 ).toString().toLatin1();
        QString lang = item.at( 2 ).toString();
        SubtitleItem sub( lang, abbr );
//      qDebug() << "subtitle " << sUrl << "abbr:" << sub.langAbbr << "lang:" << sub.language;
        QUrl qurl( sUrl );
        if ( qurl.isValid() )
        {
            TorrentSubtitlesReply *reply = new TorrentSubtitlesReply( netMan.get( QNetworkRequest( qurl ) ),
                                                                      index, sub );
            connect( reply, SIGNAL( finished( SubtitleItem ) ), this, SIGNAL( haveSubtitles( SubtitleItem ) ) );
        }
    }
    if ( activeTorrent > 0 ) qDebug() << "torrent active on add ";
    activeTorrent = index;
    return QString::number( index );
}

bool HostApp::url_request( QString url )
{
//  qDebug() << "url_request" << url;
    QUrl qurl( url );
    if ( !qurl.isValid() ) return false;
    QNetworkReply *reply = netMan.get( QNetworkRequest( qurl ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( updateSubtitleThumbs() ) );
    return true;
}

bool HostApp::cancelTorrent( QString ATorrentId )
{
    qDebug() << "cancel Torrent HostApp" << ATorrentId;
    bool ok;
    int index = ATorrentId.toInt( &ok );
    if ( !ok ) return false;
    return cancelTorrent( index );
}

bool HostApp::cancelTorrent( int index )
{
    if ( !index ) qDebug() << "cancel Torrent: no index";
    else qDebug() << "cancel Torrent" << index;
    m_thread.stopOnTorrent();
    activeTorrent = 0;
    return torrentStopDelete( index );
}

bool HostApp::cancelTorrent()
{
    if ( activeTorrent < 1 ) qDebug() << "cancel Torrent: no index";
    qDebug() << "cancel active Torrent";
    return cancelTorrent( activeTorrent );
}

bool HostApp::vpn_isConnected()
{
    qDebug() << "vpn vpn_isConnected " << ( QNetworkProxy::applicationProxy().type() == QNetworkProxy::Socks5Proxy ? "on" : "off" ) << "type " << QNetworkProxy::applicationProxy().type();

    return QNetworkProxy::applicationProxy().type() == QNetworkProxy::Socks5Proxy;
}

bool HostApp::vpn_connect()
{
    settings->connectProxy();
    qDebug() << "vpn connect ";
    return true;
}

bool HostApp::vpn_disconnect()
{
    settings->disconnectProxy();
    qDebug() << "vpn disconnect ";
    return true;
}

bool HostApp::setConfig( QString config )
{
    qDebug() << "setConfig" << config.size() << config;
    settings->importJson( config, true );
    return true;
}

QString HostApp::getTempPath()
{
    QJsonObject obj;
    obj.insert( "path", QJsonValue( downloadDir ) );
    obj.insert( "cleanOnExit", QJsonValue( cleanOnExit ) );

    QString result = QJsonDocument( obj ).toJson();
    result.replace( "\\", "\\\\" );
    qDebug() << "getTempPath" << result;
    return result;
}

QString HostApp::setTempPath()
{
    QString result  = QFileDialog::getExistingDirectory( QApplication::activeWindow(),
                                                         "Select download directory",
                                                         downloadDir.size() ? downloadDir : QDir::homePath(),
                                                         QFileDialog::ShowDirsOnly );
    if ( result.size() )
    {
        downloadDir = result;
        settings->setValue( SETTINGS_KEY_DOWNLOAD_DIR, downloadDir );
        torrentInit( result.toLocal8Bit().constData(), cleanOnExit ? 1 : 0 );
        qDebug() << "setTempPath" << result << downloadDir;
    }
    return getTempPath();
}

bool HostApp::OpenTempDir()
{
    QString path = QDir::toNativeSeparators( downloadDir );
    return QDesktopServices::openUrl( QUrl( "file:///" + path ) );
}

bool HostApp::setCleanOnExit( int isClear )
{
    cleanOnExit = isClear;
    settings->setValue( SETTINGS_KEY_CLEAN_ON_EXIT, cleanOnExit );
    return true;
}

void HostApp::setUserFontSize( int size )
{
    fontSize = size;
    settings->setValue( SETTINGS_KEY_FONT_SIZE, fontSize );
    int s;
    switch ( fontSize )
    {
    case -2: s = 20; break;
    case -1: s = 18; break;
    default:
    case  0: s = 16; break;
    case  1: s = 11; break;
    case  2: s = 7; break;
    }
    QStringList list( QString( "--freetype-rel-fontsize=%1" ).arg( s ) );
    VLC::resetInstance( list );
}

bool HostApp::openBrowser( QString url )
{
    return QDesktopServices::openUrl( QUrl( url ) );
}






DownloadNotifyThread::DownloadNotifyThread( HostApp *hostApp ) : tId( 0 )
{
    QObject::connect( this,     SIGNAL( torrentProgressAvailable( int, int, int, int, QString ) ),
                      hostApp,  SLOT( updateTorrentProgress( int, int, int, int, QString ) ) );
    QObject::connect( this,     SIGNAL( torrentUrlAvailable( QString ) ),
                      hostApp,  SLOT( hideLoading() ) );
    QObject::connect( this,     SIGNAL( downloadInfoChanged( DownloadInfo ) ),
                      hostApp,  SIGNAL( downloadInfoChanged( DownloadInfo ) ) );
}

DownloadNotifyThread::~DownloadNotifyThread()
{
    stopOnTorrent();
}


void DownloadNotifyThread::startOnTorrent( int torrentId )
{
    stopOnTorrent();
    tId = torrentId;
    start();
}

void DownloadNotifyThread::stopOnTorrent()
{
    if ( !isRunning() ) return;
    requestInterruption();
    wait();
}

void DownloadNotifyThread::run()
{
    dlInfo info;
    int progress = 0;
    int havePieces = 0;
    int pieceBytes = 0;
    QByteArray url;
    url.resize( 1000 );
    bool haveUrl = false;
    forever
    {
        msleep( 1000 / TORRENT_INFO_UPDATES_PER_SECOND );
        if ( QThread::currentThread()->isInterruptionRequested() ) return;
        info = *torrentGetInfo( tId );
        if ( haveUrl ) emit downloadInfoChanged( DownloadInfo( info ) );
        else
        {
            if ( ( haveUrl = torrentGetFileUrl( tId, url.data(), url.size() )) == true )
            {
                QString theUrl( url );
                theUrl.replace( "\\", "/" );
                theUrl.replace( "\"", "" );
                qDebug() << "UrlAvailable" << theUrl;
                emit torrentUrlAvailable( theUrl );
            }
            else
            {
//              qDebug() << "pieces" << info.piecesDone << "of" << info.piecesTotal;
                if ( havePieces < info.piecesDone )
                {
                    havePieces = info.piecesDone;
                    pieceBytes = 0;
                }
                pieceBytes += info.downloadRateBs / TORRENT_INFO_UPDATES_PER_SECOND;
                const int currentProgress = ( pieceBytes + info.piecesDone * info.pieceSize ) * 100 / ( info.pieceSize * TORRENT_PIECES_TO_PLAY );
                if ( currentProgress > progress ) progress = currentProgress;
                if ( progress > 100 ) progress = 100;
                else if ( progress == 0 && (info.seeders || info.peers ) ) progress = 3;
                QString msg = progress ? TK_DOWNLOADING : TK_STARTING_DOWNLOAD;
                emit torrentProgressAvailable( progress, info.downloadRateBs / 1000, info.seeders, info.peers, msg );
            }
        }
    }
}


