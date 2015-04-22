#include "ResizeDragEventHandler.h"

#include "FramelessMainWindow.h"
#include <QEvent.h>
#include <QDebug>
#include "defaults.h"
#include <QApplication>
#include <QObjectList>
#include "GraphicsBrowser.h"
#include "vlc.h"

#include "VMediaPlayer.h"

#include "VideoControl.h"

ResizeDragEventHandler::ResizeDragEventHandler( FramelessMainWindow& window, QObject& filteredWidget ) : QObject( &window ),
   m_ResizeState( IdleResize ), mw( &window )
{
    filteredWidget.installEventFilter( this );
}

ResizeDragEventHandler::~ResizeDragEventHandler()
{
//  mw->removeEventFilter( this );
}

bool ResizeDragEventHandler::eventFilter( QObject *obj, QEvent *event )
{
    (void) obj;
#ifdef Q_OS_WIN
    if ( event->type() == QEvent::MouseButtonDblClick )
    {
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        const QRect r = mw->geometry();
        const bool headerArea = QRect( r.topLeft(), QPoint( r.right() - DRAG_ZONE_RIGHT_MARGIN, r.top() + DRAG_ZONE_HEIGHT ) ).contains( e->globalPos() );
        if ( e->button() != Qt::LeftButton || !headerArea ) return false;
        mw->isMaximized() ? mw->showNormal() : mw->showMaximized();
        e->accept();
        return true;
    }
#endif
    if ( event->type() == QEvent::MouseButtonPress )
    {
//      qDebug() << "win" << obj->isWindowType() << "wid" << obj-isWidgetType();
//      if ( obj->objectName() != "FramelessMainWindowClassWindow" ) return false;
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        if ( e->button() != Qt::LeftButton ) return false;
        m_ResizeState = getResizeState( e->globalPos() );
        m_ResizeClickCoords = e->globalPos();
        if ( m_ResizeState == IdleResize ) return false;
        e->accept();
        return true;
    }
    if ( event->type() == QEvent::MouseButtonRelease )
    {
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        if ( m_ResizeState == IdleResize ) return false;
        m_ResizeState = IdleResize;
        QApplication::restoreOverrideCursor();
        e->accept();
        return true;
    }
    if ( event->type() == QEvent::KeyPress )
    {
        QKeyEvent *e = static_cast<QKeyEvent *>( event );
        switch ( e->key() )
        {
#ifdef _DEBUGGING
        case Qt::Key_Asterisk: ///!!!!!! testing only
            if ( !VLC::hasDebugFile() ) break;
            if ( mw->isPlayerShown() )
            {
                if ( VLC::_player ) VLC::_player->stop();
            }
            else
            {
                mw->playMedia( VLC::defaultFile );
                DownloadInfo info;
                info.downloaded = 50000;
                info.total = 100000;
                mw->control()->updateDownloadStatus( info );
            }
            break;
#endif
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if ( !mw->isPlayerShown() ) return false;
             emit enterPressed();
            break;
        case Qt::Key_Left: emit leftPressed(); break;
        case Qt::Key_Right: emit rightPressed(); break;
        case Qt::Key_Up: emit upPressed(); break;
        case Qt::Key_Down: emit downPressed(); break;
        case Qt::Key_PageUp: mw->control()->showForPeriod( 30 ); break;
        case Qt::Key_PageDown: mw->control()->unshow(); break;
        case Qt::Key_Space:
            if ( !mw->isPlayerShown() ) return false; 
            emit pausePressed();
            break;
        case Qt::Key_Escape:
            if ( !mw->isPlayerShown() || !mw->isFullScreen() ) return false;
            mw->toggleMaximize();
            break;
        default:
            return false;
        }
        event->accept();
        return true;
    }

    if ( event->type() == QEvent::MouseMove )
    {
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        QPoint eventPoint = e->globalPos();
        QRect geom = mw->geometry();
        QPoint diff = eventPoint - m_ResizeClickCoords;

        switch ( m_ResizeState )
        {
        default:
            return false;
        case TopResize:
            geom.setTop( geom.top() + diff.y() );
            break;
        case BottomResize:
            geom.setBottom( geom.bottom() + diff.y() );
            break;
        case LeftResize:
            geom.setLeft( geom.left() + diff.x() );
            break;
        case RightResize:
            geom.setRight( geom.right() + diff.x() );
            break;
        case Drag:
            geom.translate( diff );
            break;
        case IdleResize:
            {
                switch ( getResizeState( e->globalPos() ) )
                {
                case TopResize:
                case BottomResize:
                    QApplication::setOverrideCursor( Qt::SizeVerCursor );
                    return true;
                case LeftResize:
                case RightResize:
                    QApplication::setOverrideCursor( Qt::SizeHorCursor );
                    return true;
                case Drag:
                    QApplication::setOverrideCursor( Qt::SizeAllCursor );
                    return true;
                case IdleResize:
                default:
                    QApplication::restoreOverrideCursor();
                    return false;
                }
            }
            break;
        }

        //do some checks
        if ( geom.size().boundedTo( mw->maximumSize() ).expandedTo( mw->minimumSize() ) == geom.size() )
        {
            mw->setGeometry( geom );
            m_ResizeClickCoords = eventPoint;
        }
        return true;
    }
//  if ( event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>( event )->key() == Qt::Key_Space ) // MouseButtonPress )
//  {
//      event->accept();
//      if ( mw->video() ) mw->stopPlayback();
//      else mw->playMedia( VLC::defaultFile );
//      return true;
//  }



// standard event processing
    return false;
}


ResizeDragEventHandler::ResizeState ResizeDragEventHandler::getResizeState( const QPoint& pos ) const
{
    const QRect r = mw->geometry();

    if ( mw->isMaximized() || mw->isFullScreen() ) return IdleResize;
    if ( QRect( r.topLeft(), QPoint( r.right(), r.top() + RESIZE_ZONE ) ).contains( pos ) ) return TopResize;
    if ( QRect( QPoint( r.left(), r.bottom() - RESIZE_ZONE ), r.bottomRight() ).contains( pos ) ) return BottomResize;
    if ( QRect( QPoint( r.right() - RESIZE_ZONE, r.top() ), r.bottomRight() ).contains( pos ) ) return RightResize;
    if ( QRect( r.topLeft(), QPoint( r.left() + RESIZE_ZONE, r.bottom() ) ).contains( pos ) ) return LeftResize;
    if ( QRect( r.topLeft(), QPoint( r.right() - DRAG_ZONE_RIGHT_MARGIN, r.top() + DRAG_ZONE_HEIGHT ) ).contains( pos ) ) return Drag;
    return IdleResize;
}

