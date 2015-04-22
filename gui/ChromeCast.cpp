#include "ChromeCast.h"
#include "NjsProcess.h"
#include "GlyphButton.h"
#include "VLC.h"

#include <QDebug>

#include <QPushButton>
#include <QMovie>

#include <QFrame>
#include <QBoxLayout>
#include <QLabel>

#include <QJsonDocument>
#include <QJsonObject>
#include <QWidget>

#include <QNetworkProxy>
#include <QThread>
#include <QEventLoop>
#include "VMediaPlayer.h"

const char CHROMECAST_SCRIPT_DISCOVERY[] = "device-discovery.js";
const char CHROMECAST_SCRIPT_SERVER[] = "server.js";
const char CHROMECAST_DEVICE_TEXT_LOCAL[] = "Popcorn Time";

const int CHROMECAST_DISCOVERY_RESTARTS = 3;

CastDevice::CastDevice() : type( CAST_DEVICETYPE_INVALID ) { }

CastDevice::CastDevice( const QJsonObject& jObj ) : type( CAST_DEVICETYPE_INVALID )
{
    QString typeStr = jObj.value( "deviceType" ).toString();

    if ( typeStr == "googlecast" ) type = CAST_DEVICETYPE_CHROMECAST;
    else if ( typeStr == "airplay" ) type = CAST_DEVICETYPE_AIRPLAY;
    else return;

    name = jObj.value( "name" ).toString().toLatin1();
    ip = jObj.value( "ip" ).toString().toLatin1();
    url = jObj.value( "url" ).toString().toLatin1();
}







ChromeCast::ChromeCast( QWidget *parent ) :
   QObject( parent ),
   netMan( parent ),
   discoveryProc( new NjsProcess( this ) ),
   discoveryRestartCounter( CHROMECAST_DISCOVERY_RESTARTS ),
   castProc( new NjsProcess( this ) ),
   popup( new Frame( parent ) ), //Qt::Dialog |
   view( new ListWidget ),
   castBtn( 0 ),
   btnMovie( new QMovie( ":/chromecast_search" ) ),
   isCasting( false ),
   m_state( Idle )
{
    netMan.setProxy( QNetworkProxy( QNetworkProxy::NoProxy ) );

    volumeTimer.setSingleShot( true );
    connect( &volumeTimer, SIGNAL( timeout() ), this, SLOT( onVolumeTimer() ) );

    connect( discoveryProc, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( discoveryStopped( int, QProcess::ExitStatus ) ) );
    connect( discoveryProc, SIGNAL( haveToken( QByteArray, QByteArray ) ), this, SLOT( parseNjsData( QByteArray, QByteArray ) ) );
    discoveryProc->setArguments( QStringList( CHROMECAST_SCRIPT_DISCOVERY ) );
    discoveryProc->start();

    connect( castProc, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( serverStopped( int, QProcess::ExitStatus ) ) );
    connect( castProc, SIGNAL( haveToken( QByteArray, QByteArray ) ), this, SLOT( parseNjsData( QByteArray, QByteArray ) ) );
    castProc->setArguments( QStringList( CHROMECAST_SCRIPT_SERVER ) );

    QVBoxLayout *vl = new QVBoxLayout( popup );
    QLabel *lbl = new QLabel( "Play on:" );
    vl->addWidget( lbl, 0 );
    lbl->setStyleSheet( "" );

    vl->addWidget( view );
    vl->setSizeConstraint( QLayout::SetMinimumSize );
    view->addItem( CHROMECAST_DEVICE_TEXT_LOCAL );
    view->setSelectionMode( QAbstractItemView::SingleSelection );
    view->item( 0 )->setSelected( true );
    view->setStyleSheet( "QListView { border:0px; font:10pt; } " );
    connect(view, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(onListItemClicked(QListWidgetItem*)) );

    popup->setStyleSheet( "QFrame {background-color:black; color:#cacaca; border:1px solid #666666; padding:0px; margin:0px; font:10pt;} "
                          "QLabel {font:13pt; border:0px} " );

    connect( btnMovie, SIGNAL( frameChanged( int ) ), this, SLOT( frameChange( int ) ) );
    if ( btnMovie->loopCount() != -1 ) connect( btnMovie, SIGNAL( finished() ), btnMovie, SLOT( start() ) );
    btnMovie->setCacheMode( QMovie::CacheAll );
}

void ChromeCast::frameChange( int frame )
{
    (void) frame;
//  static QPixmap pixmap;
//  qDebug() << "frame" << frame << btnMovie->isValid() << ( btnMovie->currentPixmap().toImage() == pixmap.toImage());
//  pixmap = btnMovie->currentPixmap();
    if ( castBtn ) castBtn->setIcon( QIcon( btnMovie->currentPixmap() ) );
}


void ChromeCast::discoveryStopped( int exitCode, QProcess::ExitStatus exitStatus )
{
    (void) exitCode; (void) exitStatus;
    qDebug() << "discovery finished, exit code" << exitCode << ( exitStatus == QProcess::NormalExit ? "" : "crashed" );
    startDiscovery( true );
}

void ChromeCast::serverStopped( int exitCode, QProcess::ExitStatus exitStatus )
{
    (void) exitCode; (void) exitStatus;
    qDebug() << "server finished, exit code" << exitCode << ( exitStatus == QProcess::NormalExit ? "" : "crashed" );
    handleCastingSwitch( false );
}


void ChromeCast::setButton( GlyphButton *btn )
{
    castBtn = btn;
    QObject::connect( castBtn, SIGNAL( clicked() ), this, SLOT( buttonClicked() ) );
    castBtn->setCheckable( false );
}


void ChromeCast::buttonClicked()
{
    if ( castBtn->isCheckable() )
    {
        stop();
        return;
    }
    popup->adjustSize();

    QWidget *src = qobject_cast<QWidget *>( sender() );
    if ( src )
    {
        const QPoint srcCenter = src->parentWidget()->mapToGlobal( src->geometry().center() );
        const QSize pSize = popup->size();
        const QPoint newPos = QPoint( srcCenter.x() - pSize.width(), srcCenter.y() - pSize.height() );
        popup->move( newPos );
    }
    if ( devs.size() < 2 ) startDiscovery( false );
    popup->show();
}

void ChromeCast::stop()
{
    if ( isCasting )
    {
        QTimer timer;
        timer.setSingleShot( true );
        QEventLoop loop;
        QObject::connect( &timer, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
        QNetworkReply *reply = issueCommand( "command/stop" );
        if ( reply )
        {
            QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
            timer.start( 500 );
            qDebug() << "stopping";
            loop.exec();
            qDebug() << "stopped, reply finished" << reply->isFinished() << "timer active" << timer.isActive();
        }
    }
    castProc->kill();
}

void ChromeCast::pause() { if ( state() != Paused ) issueCommand( "command/pause" ); }
void ChromeCast::setTime( int time ) { issueCommand( "command/seek?seek=" + QByteArray::number( double( time ) / 1000 ) ); }
void ChromeCast::setMute( bool muted ) { issueCommand( QByteArray( "command/mute?muted=" ) + ( muted ? "1" : "0"  ) ); }
void ChromeCast::onVolumeTimer() { issueCommand( "command/volume?volume=" + QByteArray::number( double(lastVolume) / 200 ) ); }

void ChromeCast::setPause( bool setPause )
{
    if ( state() == ( setPause ? Paused : Playing ) ) return;
    issueCommand( setPause ? "command/pause" : "command/play" );
}

void ChromeCast::setVolume( int volume )
{
    static QTime blockUntil = QTime::currentTime();
    if ( lastVolume == volume ) return;
    lastVolume = volume;
    if ( QTime::currentTime() < blockUntil  ) volumeTimer.start( 250 );
    else
    {
        issueCommand( "command/volume?volume=" + QByteArray::number( double( volume ) / 200 ) );
        blockUntil = QTime::currentTime().addMSecs( 500 );
    }
}


void ChromeCast::parseDiscovery( QByteArray data )
{
    QJsonParseError err;
    QJsonObject jObj = QJsonDocument::fromJson( data, &err ).object();
    if ( jObj.isEmpty() )
    {
        qDebug() << "JSON error" << err.errorString() << data;
        return;
    }

    CastDevice item( jObj );

    if ( !item.isValid() || devs.contains( item ) ) return;
    devs += item;
    view->addItem( item.name );
    view->adjustSize();
    popup->adjustSize();
    qDebug() << "discovered" << item.type << item.name << item.ip << item.url;
}

void ChromeCast::parseMStat( QByteArray data )
{
    QJsonParseError err;
    QJsonObject jObj = QJsonDocument::fromJson( data, &err ).object();
    if ( jObj.isEmpty() )
    {
        qDebug() << "JSON error" << err.errorString() << data;
        return;
    }

    QString state = jObj.value( "playerState" ).toString( "INVALID_VALUE" );
    if ( state == "BUFFERING" )
    {
        state = "Buffering";
        setState( Playing );
        const int time = jObj.value( "media" ).toObject().value( "duration" ).toDouble( -1 ) * 1000;
        if ( time >= 0 ) emit lengthChanged( time );
    }
    else if ( state == "PLAYING" )
    {
        state = "Playing";
        setState( Playing );
        emit playing();
    }
    else if ( state == "PAUSED" )
    {
        state = "Paused";
        setState( Paused );
        emit paused();
    }
    else if ( state == "INVALID_VALUE" ) state = "";
    if ( state.size() )
    {
        state.prepend( QString( CAST_DEVICETYPE_USER_STRING[currentDevice.type] ) + " " );
        emit stateText( state );
    }

    switch ( currentDevice.type )
    {
    case CAST_DEVICETYPE_AIRPLAY: emit stateImage( ":/airplay_icon" ); break;
    case CAST_DEVICETYPE_CHROMECAST: emit stateImage( ":/chromecast_icon" ); break;
    default: emit stateImage( "" ); break;
    }


    const int time = jObj.value( "currentTime" ).toDouble( -1 ) * 1000;

    if ( time >= 0 ) emit timeChanged( time );

}

void ChromeCast::startDiscovery( bool crashed )
{
    qDebug() << "startDiscovery" << crashed;
    if ( discoveryProc->state() != QProcess::NotRunning ) return;
    if ( crashed )
    {
        if ( discoveryRestartCounter <= 0 ) return;
        --discoveryRestartCounter;
    }
    else discoveryRestartCounter = CHROMECAST_DISCOVERY_RESTARTS;

    devs.clear();
    while ( view->count() > 1 ) view->takeItem( view->count() - 1 );
    discoveryProc->start();
}

void ChromeCast::handleCastingSwitch( bool switchedToEnable )
{
    if ( switchedToEnable == isCasting ) return;
    if ( switchedToEnable )
    {
        if ( state() == Idle ) setState( Opening );
    }
    else
    {
        if ( state() != Idle ) setState( Idle );
    }
    emit castingSwitched( isCasting = switchedToEnable );
    qDebug() << "Casting is now" << ( isCasting ? "connected" : "disconnected" );
    totalLength = 1;
    lastVolume = 100;
    isCasting ? stateRefreshtimer.start( 500, this ) : stateRefreshtimer.stop();
    btnMovie->stop();
    castBtn->setCheckable( isCasting );
    castBtn->setChecked( isCasting );
    castBtn->changedState( castBtn->isChecked() );
    if ( !switchedToEnable ) 
    {
        view->item( 0 )->setSelected( true );
        currentDevice = CastDevice();
    }
}

void ChromeCast::timerEvent( QTimerEvent * )
{ issueCommand( "command/mediaState" ); }

void ChromeCast::parseNjsData( QByteArray key, QByteArray value )
{
    if ( key == "DEVICE_FOUND" ) parseDiscovery( value );
    else if ( key == "LISTEN_URL" )
    {
        serverUrl = value;
//      netMan.get( QNetworkRequest( serverUrl.toString() + "command/deviceState" ) );
//      qDebug() << serverUrl << "stat req";
        QString req = serverUrl.toString() + "command/load?deviceType=%1"
           "&deviceIP=%2"
           "&torrentFile=%3";
        req = req.arg( CAST_DEVICETYPE_STRING[currentDevice.type] )
           .arg( currentDevice.ip.toString() )
           .arg( QString( QUrl::toPercentEncoding( VLC::_player->getFileName() ) ) );

        QNetworkReply *reply = netMan.get( QNetworkRequest( req ) );
        connect( reply, SIGNAL( finished() ), this, SLOT( onReqestFinish() ) );
        qDebug() << req << "stat req";

    }
    else if ( key == "DSTAT" ) handleCastingSwitch( value == "CONNECTED" );
    else if ( key == "MSTAT" ) parseMStat( value );
    else if ( key == "ERR" )
    {
        qDebug() << "parseCast: ERR" << value;
        stop();
        stateText( "Casting error" );
    }
}

void ChromeCast::onListItemClicked( QListWidgetItem *item )
{
    popup->hide();

    if ( item->text() == currentDevice.name ) return;

    stop();

    currentDevice = CastDevice();

    if ( item->text() == CHROMECAST_DEVICE_TEXT_LOCAL ) return;

    foreach( CastDevice dev, devs )
    {
        if ( item->text() != dev.name ) continue;
        currentDevice = dev;
        btnMovie->start();
        castProc->restart();
//      qDebug() << "selected" << dev.name << "at" << dev.ip;
        emit deviceSelected();
        return;
    }
}

QNetworkReply* ChromeCast::issueCommand( QByteArray command )
{
    if ( m_state != Paused && m_state != Playing ) return 0;

//  static QString reqArgs = "%1deviceType=%2&deviceIP=%3";
//  command += reqArgs.arg( QString( command.contains( '?' ) ? '&' : '?' ),
//                          CAST_DEVICETYPE_STRING[currentDevice.type],
//                          currentDevice.ip.toString() );

    if ( !command.contains( "command/mediaState" ) ) qDebug() << "command" << command;
    QNetworkReply *reply = netMan.get( QNetworkRequest( serverUrl.toString() + command ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( onReqestFinish() ) );
    QThread::msleep( 50 );
    return reply;
}

void ChromeCast::onReqestFinish()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
//  if ( reply ) qDebug() << "reply to" << reply->request().url().path() << ":" << reply->readAll();
    delete reply;
}

void ChromeCast::setState( State newState )
{
    if ( state() == newState ) return;
    qDebug() << "setState" << newState << "from" << m_state;
    m_state = newState;
    emit stateChanged();
}
