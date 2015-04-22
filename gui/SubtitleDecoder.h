#ifndef __SUBTITLEDECODER_H_INCL__
    #define __SUBTITLEDECODER_H_INCL__

    #include <QMap>
    #include <QByteArray>

class QLibrary;

typedef QList<QByteArray> TEncodingList;

class SubtitleDecoder
{
public:
    SubtitleDecoder();

    QByteArray decodeToUtf8( QByteArray langAbbr, QByteArray data );

protected:
    QByteArray decodeByBom( QByteArray data );
    bool iconvRecodeToUtf8( const QByteArray codec, const QByteArray& src, QByteArray& dst );
    int match( const QByteArray langAbbr, QByteArray utf8Data );
    QMap<QByteArray, TEncodingList> languageToEncoding;
    QLibrary *lib;
};


#endif // __SUBTITLEDECODER_H_INCL__
