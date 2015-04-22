#ifndef __SUBCOMBOBOX_H_INCL__
    #define __SUBCOMBOBOX_H_INCL__

    #include <QComboBox>
    #include "commontypes.h"
    #include "SubtitleDecoder.h"

class QFile;

class SubComboBox : public QComboBox
{
    Q_OBJECT

public:
    SubComboBox();

    public slots:
    void sort( Qt::SortOrder sortOrder = Qt::AscendingOrder );
    void addSubtitle( SubtitleItem item );
    void clearSubtitles();

    private slots:
    void subtitleSelected( const QString& text );

signals:
    void haveSubtitleFile( QString fileName );

protected:
    QList<SubtitleItem> subs;
    QFile *subsTempFile;
    SubtitleDecoder sDecoder;
};


#endif // __SUBCOMBOBOX_H_INCL__
