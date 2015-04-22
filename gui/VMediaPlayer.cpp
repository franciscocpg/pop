#include "VMediaPlayer.h"
#include "vlc.h"
#include <vlc-qt/Audio.h>
#include <vlc-qt/Media.h>
#include <vlc-qt/Video.h>

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QTime>
#include <QThread>
#include <QApplication>

VMediaPlayer::VMediaPlayer( VlcInstance *instance ) : VlcMediaPlayer( instance ), _media( 0 )
{
    connect( this, SIGNAL( stateChanged() ),
             this, SLOT( onStateChange() ) );
    connect( this, SIGNAL( stopped() ),
             this, SLOT( closeFile() ) );
}

VMediaPlayer::~VMediaPlayer()
{
    closeFile();
}

void VMediaPlayer::closeFile()
{
    if ( _media ) delete _media;
    _media = 0;
}

void VMediaPlayer::openFile( QString fileName, VlcVideoDelegate& widget )
{
    QFileInfo info( fileName );
    if ( !info.exists() )
    {
        qDebug() << "openMedia: file error";
        return;
    }
    fileName = info.canonicalFilePath();
    closeFile();

    setVideoWidget( &widget );
    _media = new VlcMedia( fileName, true, VLC::_instance );

    _fileName = fileName;

    this->open( _media );

    QTime waitTime = QTime::currentTime().addSecs( 2 );
    int sleep = 10;
    while ( QTime::currentTime() < waitTime )
    {
        qApp->processEvents();
        if ( state() != Vlc::Idle ) break;
        QThread::msleep( sleep );
        sleep *= 1.5;
    }
    // Ended or Error check?
}



void VMediaPlayer::setSubtitleFile( QString fileName )
{
    video()->setSubtitle( -1 );
    video()->setSubtitleFile( QDir::toNativeSeparators( fileName ) );
}

void VMediaPlayer::setMute( bool mute ) { if ( audio()->getMute() != mute ) audio()->toggleMute(); }

void VMediaPlayer::setTime( int time ) { VlcMediaPlayer::setTime( time ); }
void VMediaPlayer::setVolume( int volume ) { audio()->setVolume( volume ); }
void VMediaPlayer::setPause( bool setPause ) { setPause ? this->pause() : this->resume(); }

void VMediaPlayer::onStateChange()
{
    switch ( state() )
    {
    case Vlc::Buffering: emit stateText( "Buffering" ); break;
    case Vlc::Playing: emit stateText( "Playing" ); break;
    case Vlc::Paused: emit stateText( "Paused" ); break;
    case Vlc::Idle: emit stateText( "Idle" ); break;
    case Vlc::Opening: emit stateText( "Opening" ); break;
    case Vlc::Stopped: emit stateText( "Stopped" ); break;
    case Vlc::Ended: emit stateText( "Ended" ); break;
    case Vlc::Error: emit stateText( "Error" ); break;
    }
    emit stateImage( "" );
}

