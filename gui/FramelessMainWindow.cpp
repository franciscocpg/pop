#include "FramelessMainWindow.h"
#include "GraphicsBrowser.h"
#include "vlc.h"
#include "defaults.h"
#include "hostApp.h"
#include "VideoControl.h"

#include <QApplication>
#include <QShortcut>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>

//#include <QGraphicsOpacityEffect>

#include <vlc-qt/Video.h>
#include "vlc.h"
#include "VMediaPlayer.h"

void EventfulVidgetVideo::setImage( QString resourcePng )
{
    static QString styleSheet = "background-image: url(%1); "
       "background-position: center center; "
       "background-repeat: no-repeat;";
    if ( resourcePng.size() )
    {
        if ( !_icon ) _icon = new QWidget( this );
        _icon->move( 0, 0 );
        _icon->resize( this->size() );
//      qDebug() << styleSheet.arg( resourcePng );
        _icon->setStyleSheet( styleSheet.arg( resourcePng ) );
        _icon->show();
    }
    else if ( _icon )
    {
        delete _icon;
        _icon = 0;
    }
}

void EventfulVidgetVideo::leaveEvent( QEvent * ) { emit mouseLeave(); }
void EventfulVidgetVideo::enterEvent( QEvent * ) { emit mouseEnter(); }


FramelessMainWindow::FramelessMainWindow( QWidget *parent ) :
   QMainWindow( parent ),
   m_scene( new QGraphicsScene( this ) ),
   m_browser( new GraphicsBrowser( m_scene, *this ) ),
   m_mouseFilter( *this, *qApp ),
   m_video( new EventfulVidgetVideo( this ) ),
   m_intro( new QGraphicsScene ),
   m_control( new VideoControl( this ) ),
   m_closeControl( new QFrame( this ) ),
   m_hostApp( new HostApp( m_browser->webView(), this ) )
{
    m_video->hide();
    m_control->hide();
    m_browser->webView()->page()->settings()->setAttribute( QWebSettings::JavascriptCanOpenWindows, true );

    m_control->setVolume( m_hostApp->getVolume() );

    m_closeControl->hide();
    m_closeControl->setStyleSheet( "QFrame{background-color:black; } "
                                   "QPushButton { background-image: url(':/close-styled'); width:31;} "
                                   "QWidget{border:none; padding:0; spacing:0; height:30;}" );
    QHBoxLayout *hl = new QHBoxLayout( m_closeControl );
    hl->setContentsMargins( QMargins() );
    hl->addSpacerItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum ) );

    m_closeButton = new QPushButton();
    m_closeButton->setCursor( Qt::PointingHandCursor );
//  btn->setFlat( true );
    hl->addWidget( m_closeButton );
    connect( m_control, SIGNAL( uiShow() ), m_closeButton, SLOT( show() ) );
    connect( m_control, SIGNAL( uiHide() ), m_closeButton, SLOT( hide() ) );


    setWindowTitle( "Popcorn Time" );
    m_intro->addPixmap( QPixmap( ":/intro" ).scaled( DEFAULT_WIDTH, DEFAULT_HEIGHT, Qt::IgnoreAspectRatio ) );
    m_browser->setScene( m_intro );
    new QShortcut( QKeySequence( "Ctrl+M" ), this, SLOT( minimize() ) );

    QObject::connect( m_hostApp->thread(), SIGNAL( torrentUrlAvailable( QString ) ),
                      this, SLOT( playMedia( QString ) ) );
    QObject::connect( m_browser->webView(), SIGNAL( loadFinished( bool ) ),
                      this, SLOT( removeIntro() ) );
    QObject::connect( m_hostApp, SIGNAL( commandedClose() ),
                      this, SLOT( close() ) );
    QObject::connect( m_hostApp, SIGNAL( commandedMinimize() ),
                      this, SLOT( minimize() ) );
    QObject::connect( m_hostApp, SIGNAL( toggledMaximize() ),
                      this, SLOT( toggleMaximize() ) );
    QObject::connect( m_hostApp, SIGNAL( haveSubtitles( SubtitleItem ) ),
                      m_control, SLOT( addSubtitle( SubtitleItem ) ) );
    QObject::connect( m_hostApp, SIGNAL( downloadInfoChanged( DownloadInfo ) ),
                      m_control, SLOT( updateDownloadStatus( DownloadInfo ) ) );
    QObject::connect( m_control, SIGNAL( volumeChanged( int ) ),
                      m_hostApp, SLOT( setVolume( int ) ) );

    QObject::connect( m_control, SIGNAL( newImage( QString ) ),
                      m_video, SLOT( setImage( QString ) ) );

    QObject::connect( m_control->fullscreenButton(), SIGNAL( clicked() ),
                      this, SLOT( toggleMaximize() ) );
    QObject::connect( &m_mouseFilter, SIGNAL( enterPressed() ),
                      this, SLOT( toggleMaximize() ) );
    QObject::connect( &m_mouseFilter, SIGNAL( upPressed() ),
                      m_control, SLOT( volumeUp() ) );
    QObject::connect( &m_mouseFilter, SIGNAL( downPressed() ),
                      m_control, SLOT( volumeDown() ) );
    QObject::connect( &m_mouseFilter, SIGNAL( pausePressed() ),
                      m_control, SLOT( pauseToggleReq() ) );

    connect( m_video, SIGNAL( mouseEnter() ), m_control, SLOT( showForDefaultPeriod() ) );

    this->setWindowFlags( Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint );
#ifdef Q_OS_MAC
    fixNativeWindow( this );
#endif
    this->setMinimumSize( 800, 330 );
    this->setMouseTracking( true );
    this->setCentralWidget( m_browser );
    qApp->installEventFilter( this );

    reconnectMediaPlayer();

    QObject::connect( VLC::VLCObject(), SIGNAL( mediaPlayerReplaced() ),
                      this, SLOT( reconnectMediaPlayer() ) );
}

void FramelessMainWindow::reconnectMediaPlayer()
{
    VlcMediaPlayer *player = VLC::_player;
    if ( !player ) return;

    connect( m_closeButton, SIGNAL( clicked() ), player, SLOT( stop() ) );

    connect( player, SIGNAL( opening() ), this, SLOT( playbackStarted() ) );
    connect( player, SIGNAL( opening() ), m_control, SLOT( onPlaying() ) );
    connect( player, SIGNAL( stopped() ), this, SLOT( playbackStopped() ) );
    connect( player, SIGNAL( stopped() ), m_hostApp, SLOT( cancelTorrent() ) );
    connect( player, SIGNAL( stopped() ), m_control, SLOT( resetPlayback() ) );
    connect( player, SIGNAL( stopped() ), m_control, SLOT( resetPlayback() ) );
}


FramelessMainWindow::~FramelessMainWindow()
{
    if ( VLC::_player ) VLC::_player->closeFile(); 
    delete m_scene;
    delete m_browser;
}

void FramelessMainWindow::playMedia( QString url ) { VLC::_player->openFile( url, *m_video ); }

void FramelessMainWindow::playbackStarted()
{
//  qDebug() << "playbackStarted 1";
    updateVideoSize();
    m_video->show();
    if ( isMaximized() ) showFullScreen();
    m_control->show();
    m_closeControl->show();
//  m_control->raise();
//  qDebug() << "playbackStarted 2";
}

void FramelessMainWindow::playbackStopped()
{
//  qDebug() << "playbackStopped 1";
    m_video->hide();
//  qDebug() << "playbackStopped 2";
//  VLC::closeMedia();
//  qDebug() << "playbackStopped 3";
    m_control->hide();
    m_closeControl->hide();
//  qDebug() << "playbackStopped 4";
    if ( isFullScreen() ) showMaximized();
//  qDebug() << "playbackStopped 5";
}

void FramelessMainWindow::toggleMaximize()
{
    const bool willGoNormal = isMaximized() || isFullScreen();
    m_control->fullscreenButton()->setChecked( !willGoNormal );
    if ( willGoNormal ) showNormal();
    else isPlayerShown() ? showFullScreen() : showMaximized();
}

void FramelessMainWindow::minimize()
{
#ifdef Q_OS_MAC
    minaturize( this );
#else
    this->showMinimized();
#endif
}

void FramelessMainWindow::resizeEvent( QResizeEvent *evt )
{
    QMainWindow::resizeEvent( evt );
    updateVideoSize();
}


void FramelessMainWindow::updateVideoSize()
{
    const QRect rect = this->rect();
    m_closeControl->move( 0, 0 );
    m_closeControl->resize( rect.width(), m_closeControl->height() );
    m_video->move( 0, m_closeControl->height() );
    m_video->resize( rect.width(), rect.height() - m_control->height() - m_closeControl->height() );
    m_control->resize( rect.width(), 100 );
    m_control->move( 0, rect.height() - m_control->height() );
}


void FramelessMainWindow::removeIntro()
{
    m_browser->setScene( m_scene );
    QObject::disconnect( m_browser->webView(),
                         SIGNAL( loadFinished( bool ) ),
                         this,
                         SLOT( removeIntro() ) );
    delete m_intro;
    m_intro = 0;
}

bool FramelessMainWindow::isPlayerShown()
{
    return !m_video->isHidden();
}
