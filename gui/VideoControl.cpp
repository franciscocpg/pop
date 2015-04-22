#include <QBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QMetaMethod>
#include "VMediaPlayer.h"
//#include <vlc-qt/Audio.h>
//#include <vlc-qt/Video.h>
#include <vlc-qt/WidgetVolumeSlider.h>
#include "vlc.h"
#include "defaults.h"
#include "SeekSlider.h"
#include "VideoControl.h"
#include "GlyphButton.h"

//class VolumeSlider : public VlcWidgetVolumeSlider
//{
//protected:
//    void mousePressEvent( QMouseEvent *event ) { event->accept();  }
//    void mouseReleaseEvent( QMouseEvent *event ) { event->ignore(); }
//};





VideoControl::VideoControl( QWidget *parent )
   : QFrame( parent ), m_statusLabel( new QLabel ), cast( parent )

{
    QFrame::setStyleSheet( "QFrame{ background-color:black; color:gray; border:none; padding:0; spacing:0;}" );
    this->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
    connect( &timer, SIGNAL( timeout() ), this, SLOT( unshow() ) );

    m_hidableFrame = new QFrame();
    ( new QVBoxLayout( this ))->addWidget( m_hidableFrame );
//  m_hidableFrame->setStyleSheet( ".QFrame{ background-color:red;color:gray; border:none; padding:0; spacing:0;}" );


    QVBoxLayout *vl = new QVBoxLayout( m_hidableFrame );

    QLabel *byteRateLabel = new QLabel;

    m_seek = new SeekControl( byteRateLabel );
    vl->addWidget( m_seek );
    connect( m_seek, SIGNAL( updateStatusLabel( const QString& ) ), this, SLOT( updateLabel( const QString& ) ) );
    connect( m_seek, SIGNAL( paused() ),                            this, SLOT( pauseReq() ) );

    QHBoxLayout *hl = new QHBoxLayout();
    vl->addLayout( hl );

    m_pauseBtn = new GlyphButton( "pause", "pause-over", "play", "play-over" );
    hl->addWidget( m_pauseBtn );

//  btn = new GlyphButton( "stop", "stop-over" );
//  hl->addWidget( btn );
//  connect( btn, SIGNAL( released() ), VLC::_player, SLOT( stop() ) );

    m_muteBtn = new GlyphButton( "sound", "sound-over", "mute", "mute-over" );
    hl->addWidget( m_muteBtn );

    m_vol = new VolumeSlider();
    m_vol->setMaximum( 200 );
    m_vol->setValue( m_vol->maximum() );
    m_vol->setFixedWidth( 200 );
    m_vol->setStyleSheet(
       "QSlider::groove:horizontal {"
       " border: 1px solid #999999;"
       " height: 5px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */"
       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);"
       " margin: 2px 0; }"
       "QSlider::handle:horizontal {"
       " background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);"
       " border: 1px solid #5c5c5c;"
       " width: 5px;"
       " margin: -6px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */"
       " border-radius: 3px;}"
       );
    hl->addWidget( m_vol );
    connect( m_vol, SIGNAL( valueChanged( int ) ),  this, SIGNAL( volumeChanged( int ) ) );

    hl->addWidget( m_statusLabel );

    hl->addSpacerItem( new QSpacerItem( 20, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    QLabel *lbl = new QLabel( "Downloading" );
    hl->addWidget( lbl );

    hl->addWidget( byteRateLabel );

    hl->addSpacerItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    m_subs = new SubComboBox;
    hl->addWidget( m_subs );

    m_fullscrBtn = new GlyphButton( "full-scr", "full-scr-over", "exit-full-scr", "exit-full-scr-over" );
    hl->addWidget( m_fullscrBtn );

    m_castBtn = new GlyphButton( "chromecast_off", "chromecast_off_over", "chromecast_on", "chromecast_on_over" );
    m_castBtn->setStyleSheet( "padding-bottom: 4px;" );
    hl->addWidget( m_castBtn );

    cast.setButton( m_castBtn );

    QObject::connect( &cast, SIGNAL( castingSwitched( bool ) ),
                      this, SLOT( castSwitched( bool ) ) );

    reconnectMediaPlayer();

    resetPlayback();
    QObject::connect( VLC::VLCObject(), SIGNAL( mediaPlayerReplaced() ),
                      this, SLOT( reconnectMediaPlayer() ) );
}

void VideoControl::reconnectMediaPlayer()
{
    QObject *player = VLC::_player;
    if ( !player ) return;

    connect( player, SIGNAL( stopped() ), &cast, SLOT( stop() ) );
    connect( player, SIGNAL( end() ),                m_seek, SLOT( end() ) );
    connect( player, SIGNAL( stopped() ),            m_seek, SLOT( end() ) );

    connect( &cast, SIGNAL( deviceSelected() ),      player, SLOT( pause() ) );
    connect( player, SIGNAL( paused() ),             this, SLOT( showForDefaultPeriod() ) );
    connect( player, SIGNAL( playing() ),            this, SLOT( showForDefaultPeriod() ) );

    connect( m_subs, SIGNAL( haveSubtitleFile( QString ) ), player, SLOT( setSubtitleFile( QString ) ) );

    castSwitched( false );
}

void VideoControl::castSwitched( bool casting )
{
    QObject *player = qobject_cast<ChromeCast *>( sender() );
    if ( !casting || !player ) player = VLC::_player;
    else VLC::_player->pause();

    while ( connList.size() ) QObject::disconnect( connList.takeFirst() );

    connList += connect( player, SIGNAL( lengthChanged( int ) ), m_seek, SLOT( updateFullTime( int ) ) );
    connList += connect( player, SIGNAL( timeChanged( int ) ),   m_seek, SLOT( updateCurrentTime( int ) ) );
    connList += connect( player, SIGNAL( stateText( const QString& ) ), this, SLOT( updateLabel( const QString& ) ) );
    connList += connect( m_seek, SIGNAL( sought( int ) ),       player, SLOT( setTime( int ) ) );
    connList += connect( m_seek, SIGNAL( needTextUpdate() ),    player, SLOT( onStateChange() ) );
    connList += connect( this,   SIGNAL( paused() ),            player, SLOT( pause() ) );
    connList += connect( m_pauseBtn, SIGNAL( toggled( bool ) ), player, SLOT( setPause( bool ) ) );
//  connList += connect( player, SIGNAL( paused() ),            m_pauseBtn, SLOT( uncheck() ) );
//  connList += connect( player, SIGNAL( playing() ),           m_pauseBtn, SLOT( check() ) );
//  connList += connect( player, SIGNAL( muteStatus( bool ) ),  m_muteBtn, SLOT( setChecked( bool ) ) );
    connList += connect( m_muteBtn, SIGNAL( toggled( bool ) ),  player, SLOT( setMute( bool ) ) );
    connList += connect( player, SIGNAL( paused() ),            this, SLOT( showForDefaultPeriod() ) );
    connList += connect( player, SIGNAL( playing() ),           this, SLOT( showForDefaultPeriod() ) );
    connList += connect( m_vol, SIGNAL( valueChanged( int ) ),  player, SLOT( setVolume( int ) ) );
    connList += connect( player, SIGNAL( stateChanged() ),      this, SLOT( provideControlSettings() ) );
    connList += connect( player, SIGNAL( stateImage( QString ) ), this, SIGNAL( newImage( QString ) ) );

    provideControlSettings();
    if ( player == VLC::_player ) emit newImage( "" );
}

void VideoControl::provideControlSettings()
{
    m_vol->provide();
    m_pauseBtn->provide();
    m_muteBtn->provide();
}


void VideoControl::volumeUp()   { m_vol->triggerAction( QAbstractSlider::SliderPageStepAdd ); };
void VideoControl::volumeDown() { m_vol->triggerAction( QAbstractSlider::SliderPageStepSub ); };
void VideoControl::pauseReq()   { m_pauseBtn->check(); };
void VideoControl::pauseToggleReq()   { m_pauseBtn->toggle(); };

void VideoControl::addSubtitle( SubtitleItem item ) { m_subs->addSubtitle( item ); }
void VideoControl::setVolume( int volume ) { m_vol->setValue( volume ); }

QPushButton* VideoControl::fullscreenButton() { return m_fullscrBtn; }
void VideoControl::onPlaying() { showForPeriod( CONTROL_HIDE_INTERVAL_MOUSE_AWAY_MS ); }
void VideoControl::showForDefaultPeriod() { showForPeriod( CONTROL_HIDE_INTERVAL_MOUSE_AWAY_MS ); }
void VideoControl::showForPeriod( unsigned int ms )
{
//  qDebug() << "showPeriod" << ms;
    timer.start( ms );
    if ( m_hidableFrame->isVisible() ) return;
    m_hidableFrame->show();
    emit uiShow();
}

void VideoControl::unshow()
{
//  qDebug() << "unshow";
    timer.stop();
    if ( m_hidableFrame->isHidden() ) return;
    m_hidableFrame->hide();
    emit uiHide();
}

//void VideoControl::leaveEvent( QEvent * ) { showForPeriod( CONTROL_HIDE_INTERVAL_MOUSE_AWAY_MS ); }
void VideoControl::enterEvent( QEvent * ) { showForPeriod( CONTROL_HIDE_INTERVAL_MOUSE_OVER_MS ); }


void VideoControl::resetPlayback()
{
    qDebug() << "resetPlayback ";
    m_subs->clearSubtitles();
//  if ( VLC::_player && VLC::_player->video() ) VLC::_player->video()->setSubtitle( -1 );
//  qDebug() << "resetPlayback 1";
}


void VideoControl::updateDownloadStatus( DownloadInfo info ) { m_seek->downloadInfoChange( info ); }

void VideoControl::updateLabel( const QString& newText )
{
    if ( newText.contains( "Paused", Qt::CaseInsensitive ) && m_statusLabel->text() == TK_NET_BUFFERING ) return;
    m_statusLabel->setText( newText );
}

