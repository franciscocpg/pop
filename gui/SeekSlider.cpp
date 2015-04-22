#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>

#include <QDebug>
#include <QStyleOption>
#include <QPainter>


#include "SeekSlider.h"
#include "defaults.h"

#include <vlc-qt/MediaPlayer.h>

SeekControl::SeekControl( QWidget *parent )
   : QWidget( parent ), byteRateLabel( 0 )
{
    initWidgetSeek();
}

SeekControl::SeekControl( QLabel *rateLabel )
   : byteRateLabel( rateLabel )
{
    initWidgetSeek();
}

SeekControl::~SeekControl()
{
    delete _seek;
    delete _labelElapsed;
    delete _labelFull;
}

void SeekControl::initWidgetSeek()
{
    _lock = false;

    _seek = new SeekSlider( Qt::Horizontal, this );
    _seek->setMaximum( 1 );
    _seek->setMaximumHeight( 15 );
    _seek->setAttribute( Qt::WA_TransparentForMouseEvents );
    _seek->setStyleSheet(
       "QSlider::groove:horizontal {"
       " border: 1px solid #999999;"
       " height: 3px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */"
       " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);"
       " margin: 2px 0; }"
       "QSlider::handle:horizontal {"
       " background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);"
       " border: 1px solid #5c5c5c;"
       " width: 4px;"
       " margin: -6px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */"
       " border-radius: 2px;}"
       );

    _labelElapsed = new QLabel( this );
    _labelElapsed->setText( "00:00" );

    _labelFull = new QLabel( this );
    _labelFull->setText( "00:00" );

    _labelElapsed->setStyleSheet( "QLabel { color : white; }" );
    _labelFull->setStyleSheet( "QLabel { color : white; }" );

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget( _labelElapsed );
    layout->addWidget( _seek );
    layout->addWidget( _labelFull );
    setLayout( layout );
}

void SeekControl::mouseMoveEvent( QMouseEvent *event )
{
    event->ignore();

    if ( !_lock ) return;

    updateEvent( event->pos() );
}

void SeekControl::mousePressEvent( QMouseEvent *event )
{
    event->ignore();

    lock();
}

void SeekControl::mouseReleaseEvent( QMouseEvent *event )
{
    event->ignore();

    updateEvent( event->pos() );

    unlock();
}

//void SeekControl::wheelEvent( QWheelEvent *event )
//{
//    event->ignore();
//
//    if ( !_vlcMediaPlayer ) return;
//
//    if ( event->delta() > 0 ) _vlcMediaPlayer->setTime( _vlcMediaPlayer->time() + _vlcMediaPlayer->length() * 0.01 );
//    else _vlcMediaPlayer->setTime( _vlcMediaPlayer->time() - _vlcMediaPlayer->length() * 0.01 );
//}

void SeekControl::end()
{
    QTime time = QTime( 0, 0, 0, 0 );
    QString display = "mm:ss";

    _labelElapsed->setText( time.toString( display ) );
    _labelFull->setText( time.toString( display ) );
    _seek->setMaximum( 1 );
    _seek->setValue( 0 );
}

void SeekControl::updateEvent( const QPoint& pos )
{
    if ( pos.x() < _seek->pos().x() || pos.x() > _seek->pos().x() + _seek->width() ) return;

    int clickPos = pos.x() - _seek->pos().x();
    if ( clickPos * _seek->getFullSize() >= _seek->getCurrentSize() * _seek->width() ) return;
    float op = _seek->maximum() / _seek->width();
    float newValue = clickPos * op;

    emit sought( newValue );
    _seek->setValue( newValue );
}

void SeekControl::updateCurrentTime( int time )
{
    if ( _lock ) return;

    if ( !_seek->canStillPlay( time ) )
    {
        emit paused();
        emit updateStatusLabel( TK_NET_BUFFERING );
//      qDebug() << "paused due to ratio";
    }

    QTime currentTime = QTime( 0, 0, 0, 0 ).addMSecs( time );

    QString display = "mm:ss";
    if ( currentTime.hour() > 0 ) display = "hh:mm:ss";

    _labelElapsed->setText( currentTime.toString( display ) );
    _seek->setValue( time );
}


void SeekControl::updateFullTime( int time )
{
    if ( _lock ) return;

    QTime fullTime = QTime( 0, 0, 0, 0 ).addMSecs( time );

    QString display = "mm:ss";
    if ( fullTime.hour() > 0 ) display = "hh:mm:ss";

    _labelFull->setText( fullTime.toString( display ) );

    _seek->setMaximum( time ? time : 1 );
}

void SeekControl::downloadInfoChange( DownloadInfo info )
{
    _seek->updateCurrentSize( info.downloaded );
    _seek->updateFullSize( info.total );

    if (  !_seek->canStillPlay( _seek->value() ) ) emit updateStatusLabel( TK_NET_BUFFERING );
    else emit needTextUpdate();

    if ( byteRateLabel )
    {
        byteRateLabel->setText( "-" );
        byteRateLabel->setStyleSheet( "color:gray;" );
        if ( _seek->maximum() > 1 && info.total && info.downloaded )
        {
//          qDebug() << "size" << info.total << "time" << _seek->maximum() << "rate" << info.total / _seek->maximum() << "dlrate" << info.downloadRateBs;
            const int kByteRateMedia = info.total / _seek->maximum();
            const int kByteRateDownload = info.downloadRateBs / 1000;

            byteRateLabel->setText( QString( " %1 Kb/s" ).arg( kByteRateDownload ) );
//          byteRateLabel->setText( QString( " %1/%2" ).arg( kByteRateDownload ).arg( kByteRateMedia ) );
            if ( kByteRateDownload < kByteRateMedia ) byteRateLabel->setStyleSheet( "color:red;" );
            else byteRateLabel->setStyleSheet( "color:green;" );
        }
    }
}





SeekSlider::SeekSlider( Qt::Orientation orientation, QWidget *parent )
   : QSlider( orientation, parent )
{ }


void SeekSlider::updateCurrentSize( quint64 size )
{
    currentSize = size >> 8;
    if ( currentSize > fullSize ) currentSize = fullSize;
//  qDebug() << "updCurDl" << currentSize << "of" << fullSize;
}

void SeekSlider::updateFullSize( quint64 size )
{
    fullSize = size >> 8;
//  qDebug() << "updCurFull" << fullSize << "(" << currentSize << ")";
}

void SeekSlider::paintEvent( QPaintEvent *event )
{
    QSlider::paintEvent( event );
    QStyleOptionSlider opt;
    initStyleOption( &opt );
    const QRect gRect = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this );
    const QRect hRect = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this );

    int length = 1;
    if ( fullSize ) length = gRect.width() * currentSize / fullSize;
    if ( length < 1 ) length = 1;
    else if ( length > gRect.width() - 2 ) length = gRect.width() - 2;
    const int left = gRect.left() + 1;
    const int top = gRect.top() + gRect.height() / 2 - 1;
    const int height = 2 + gRect.height() % 2;

    QPainter p( this );
    QBrush brush = QBrush( QColor( 0x2C, 0x77, 0xE6 ) );  //  Qt::red );

    if ( hRect.left() >= left && hRect.left() < length )
    {
        p.fillRect( QRect( left, top, hRect.left() - 1, height ), brush );
        p.fillRect( QRect( hRect.left() + hRect.width(), top, length - hRect.left() + hRect.width(), height ), brush );
    }
    else p.fillRect( QRect( left, top, length, height ), brush );
}

bool SeekSlider::canStillPlay( int time )
{
    if ( !getCurrentSize() || time < DONT_PAUSE_IF_JUST_STARTED_TIME_MS || fullSize == currentSize ) return true;
    const float playRatio = fullSize / currentSize;
    const int downloadedTime = maximum() / playRatio;
    return downloadedTime > ( time + maximum() / 200 );
}

