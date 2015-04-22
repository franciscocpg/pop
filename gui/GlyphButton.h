#ifndef __GLYPHBUTTON_H_INCL__
    #define __GLYPHBUTTON_H_INCL__

    #include <QPushButton>
    #include <QPixmap>

class GlyphButton : public QPushButton
{
    Q_OBJECT

public:
    GlyphButton( QString thePix, QString theHoverPix, QString thePressed = "", QString thePressedHover = "" );

    public slots:
    void changedState( bool checked );
    void check() { setChecked( true ); }
    void uncheck() { setChecked( false ); }
    void provide() { emit toggled( isChecked() ); }

protected:
    void enterEvent( QEvent *event );
    void leaveEvent( QEvent *event );

    QString pix;
    QString hoverPix;
    QString pPix;
    QString pHoverPix;
};

#endif // __GLYPHBUTTON_H_INCL__
