#include "GlyphButton.h"

GlyphButton::GlyphButton( QString thePix, QString theHoverPix, QString thePressed, QString thePressedHover )
   : pix( thePix ), hoverPix( theHoverPix ), pPix( thePressed ), pHoverPix( thePressedHover )
{
    setStyleSheet( "QPushButton:flat { border: none; }" );
    setCheckable( thePressed.size() && thePressedHover.size() );
    setFlat( true );
    setIcon( QIcon( ":/" + pix ) );
    QPixmap p = QPixmap( ":/" + pix );
    setIconSize( p.size() );
    setGeometry( 0, 0, p.width(), p.height() );
    connect( this, SIGNAL( toggled( bool ) ), this, SLOT( changedState( bool ) ) );
}

void GlyphButton::enterEvent( QEvent *event )
{
    QPushButton::enterEvent( event );
    changedState( isChecked() );
}

void GlyphButton::leaveEvent( QEvent *event )
{
    QPushButton::leaveEvent( event );
    if ( isChecked() && pPix.size() ) setIcon( QIcon( ":/" + pPix ) );
    else setIcon( QIcon( ":/" + pix ) );
}

void GlyphButton::changedState( bool checked )
{
    if ( isCheckable() && checked && pHoverPix.size() ) setIcon( QIcon( ":/" + pHoverPix ) );
    else setIcon( QIcon( ":/" + hoverPix ) );
}


