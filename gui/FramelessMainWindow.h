#ifndef __FRAMELESSMAINWINDOW_H_INCL__
    #define __FRAMELESSMAINWINDOW_H_INCL__

    #include <QMainWindow>
    #include "ResizeDragEventHandler.h"
    #include <QGraphicsScene>
    #include <QGraphicsPixmapItem>
    #include <vlc-qt/WidgetVideo.h>

class GraphicsBrowser;
class QGraphicsScene;
class HostApp;
class VideoControl;
class QMainWindow;
class QFrame;
class QPushButton;

    #ifdef Q_OS_MAC
void fixNativeWindow( QMainWindow *window );
void minaturize( QMainWindow *window );
    #endif


class EventfulVidgetVideo : public VlcWidgetVideo
{
    Q_OBJECT

public:
    EventfulVidgetVideo( QWidget *parent = 0 ) : VlcWidgetVideo( parent ), _icon( 0 ) { }

    signals:
    void mouseEnter();
    void mouseLeave();

    public slots:
    void setImage( QString resourcePng );

protected:
    void enterEvent( QEvent *event );
    void leaveEvent( QEvent *event );
    QWidget *_icon;
};

class IntroScene : public QGraphicsScene
{
public:
    IntroScene() : m_intro() { addItem( m_intro ); }
    ~IntroScene() { delete m_intro; }
    QGraphicsPixmapItem *m_intro;
};

class FramelessMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    FramelessMainWindow( QWidget *parent = 0 );
    ~FramelessMainWindow();

    GraphicsBrowser* browser() { return m_browser; }
    QGraphicsScene* scene() { return m_scene; }
    VlcWidgetVideo* video() { return m_video; }
    VideoControl* control() { return m_control; }
    bool isPlayerShown();


    public slots:
    void playMedia( QString url );
    void toggleMaximize();
    void updateVideoSize();
    void reconnectMediaPlayer();

    private slots:
    void removeIntro();
    void playbackStarted();
    void playbackStopped();
    void minimize();

protected:
//  virtual void mousePressEvent( QMouseEvent *e );
//  virtual void mouseMoveEvent( QMouseEvent *e );
//  virtual void mouseReleaseEvent( QMouseEvent *e );
//  virtual void mouseDoubleClickEvent( QMouseEvent *e );
    void resizeEvent( QResizeEvent *evt );

    QGraphicsScene *m_scene;
    GraphicsBrowser *m_browser;
    ResizeDragEventHandler m_mouseFilter;
    EventfulVidgetVideo *m_video;
    QGraphicsScene *m_intro;
    VideoControl *m_control;
    QFrame *m_closeControl;
    QPushButton *m_closeButton;
    HostApp *m_hostApp;
};

#endif // __FRAMELESSMAINWINDOW_H_INCL__
