#ifndef __RESIZEDRAGEVENTHANDLER_H_INCL__
    #define __RESIZEDRAGEVENTHANDLER_H_INCL__

    #include <QObject.h>
    #include <QPoint.h>

class FramelessMainWindow;
class QWidget;

class ResizeDragEventHandler : public QObject
{
    Q_OBJECT
    enum ResizeState
    {
        IdleResize, TopResize, BottomResize, LeftResize, RightResize, Drag
    };

public:
    ResizeDragEventHandler( FramelessMainWindow& window, QObject& filteredWidget );
    ~ResizeDragEventHandler();

    signals:
    void enterPressed();
    void upPressed();
    void downPressed();
    void leftPressed();
    void rightPressed();
    void pausePressed();

protected:
    ResizeState getResizeState( const QPoint& pos ) const;
    bool eventFilter( QObject *obj, QEvent *event );

protected:
    ResizeState m_ResizeState;
    QPoint  m_ResizeClickCoords;
    FramelessMainWindow *mw;

};

#endif // __RESIZEDRAGEVENTHANDLER_H_INCL__
