#include "SubtitleDecoder.h"
#include <QString>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QLibrary>
#include <QThread>
#include <QApplication>
#include <QRegularExpression>

#ifdef Q_OS_MAC
#define LIBICONV_PLUG
#endif

#include "iconv.h"


QMap<QByteArray, QMap<QString, short> > dictionary;

//#define CREATE_FILTERED_OUTPUT    1

class DictionaryThread : public QThread
{
    void run()
    {
        connect( this, SIGNAL( finished() ), this, SLOT( deleteLater() ) );
        QFile dic( ":/Dictionary" );
        if ( dic.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            QTextStream txStream( &dic );

#ifdef CREATE_FILTERED_OUTPUT
            QFile outfile( "temp" );
            QTextStream os( &outfile );
            os.setCodec( "UTF-8" );
            os.setGenerateByteOrderMark( true );
            outfile.open( QIODevice::WriteOnly | QIODevice::Text );
#endif

            //  txStream.setCodec( "UTF-8" );
            bool ok;
            while ( !txStream.atEnd() )
            {
                QStringList data = txStream.readLine().split( "|" );
                if ( data.size() < 3 ) break;
#ifdef CREATE_FILTERED_OUTPUT
                if ( data[0].toLocal8Bit().size() != 2 ) continue;
                os << data[0].toLocal8Bit() << "|" << data[1] << "|" << data[2] << endl;
#endif
                short wcount = data[1].toShort( &ok );
                if ( !ok ) break;
                dictionary[data[0].toLocal8Bit()][data[2]] = wcount;
            }
//          qDebug() << "dic" << dictionary;
            dic.close();
#ifdef CREATE_FILTERED_OUTPUT
            outfile.close();
#endif
        }
    }
};




QByteArray SubtitleDecoder::decodeByBom( QByteArray data )
{
    if ( data.startsWith( QByteArray::fromHex( "EFBBBF" ) ) )
    {
//      qDebug( "UTF-8 by BOM" );
        return data;
    }

    QByteArray temp;

    if ( data.startsWith( QByteArray::fromHex( "C3AFC2BBC2BF" ) ) )
    {
//      qDebug( "UTF-8 recoded from ISO-8859-1 to UTF-8 by BOM" );
        if ( iconvRecodeToUtf8( "ISO-8859-1", data, temp ) ) return temp;
        return "";
    }

    QByteArray encoding;

    if ( data.startsWith( QByteArray::fromHex( "FEFF" ) ) ) encoding = "UTF-16BE";
    else if ( data.startsWith( QByteArray::fromHex( "FFFE" ) ) ) encoding = "UTF-16LE";
    else if ( data.startsWith( QByteArray::fromHex( "0000FEFF" ) ) ) encoding = "UTF-32BE";
    else if ( data.startsWith( QByteArray::fromHex( "FFFE0000" ) ) ) encoding = "UTF-32LE";
    else if ( data.startsWith( QByteArray::fromHex( "2B2F7638" ) ) ) encoding = "UTF-7";
    else if ( data.startsWith( QByteArray::fromHex( "2B2F7639" ) ) ) encoding = "UTF-7";
    else if ( data.startsWith( QByteArray::fromHex( "2B2F762B" ) ) ) encoding = "UTF-7";
    else if ( data.startsWith( QByteArray::fromHex( "2B2F762F" ) ) ) encoding = "UTF-7";
    else if ( data.startsWith( QByteArray::fromHex( "2B2F76382D" ) ) ) encoding = "UTF-7";
    else if ( data.startsWith( QByteArray::fromHex( "84319533" ) ) ) encoding = "GB18030";
//  else qDebug() << "BOM:" << data.left( 5 ).toHex();

    if ( encoding.size() )
    {
//      qDebug() << encoding << "by BOM";
        if ( iconvRecodeToUtf8( encoding, data, temp ) ) return temp;
        return "";
    }
    return "";
}

bool SubtitleDecoder::iconvRecodeToUtf8( const QByteArray codec, const QByteArray& src, QByteArray& dst )
{
    const iconv_t cd = iconv_open( "UTF-8", codec.constData() );
    if ( cd == reinterpret_cast<iconv_t>( -1 ) )
    {
        qDebug() << "no codec for " << codec;
        iconv_close( cd );
        return false;
    }
    dst.resize( src.size() * 5 );

    const char *iptr = src.constData();
    char *optr = dst.data();
    size_t inb = src.size();
    size_t outb = dst.size();
    errno = 0;
    int res = iconv( cd, &iptr, &inb, &optr, &outb );
    iconv_close( cd );
    if ( outb == 0 || res < 0 )
    {
//      QByteArray err;
//      switch ( errno )
//      {
//      case E2BIG: err = "There is not sufficient room at *outbuf."; break;
//      case EILSEQ: err = "An invalid multibyte sequence has been encountered in the input."; break;
//      case EINVAL: err = "An incomplete multibyte sequence has been encountered in the input."; break;
//      default: break;
//      }
//      qDebug() << "codec" << codec << "inb" << inb << "outb" << outb << "res" << res << "errno" << "err" << errno << err;
        return false;
    }
//  qDebug() << "codec" << codec << "outb" << outb;
    dst.truncate( outb );
    return true;
}


QByteArray SubtitleDecoder::decodeToUtf8( QByteArray langAbbr, QByteArray data )
{

    TEncodingList codecNames = languageToEncoding.value( langAbbr ) + languageToEncoding.value( "all" );
    codecNames.append( "" ); // direct
    if ( !codecNames.size() )
    {
//      qDebug() << "no codec names for " << langAbbr;
        return data;
    }

//  qDebug() << "Decoding data size " << data.size() << langAbbr <<  "inb";

    QByteArray temp = decodeByBom( data );
    if ( temp.size() ) return temp;

    QByteArray bestData;
    double bestScore = 0;


    foreach( QByteArray codecName, codecNames )
    {
        if ( codecName.isEmpty() )
        {
//          qDebug() << "direct";
            temp = data;
        }
        else if ( !iconvRecodeToUtf8( codecName, data, temp ) ) continue;
        int matchVal = match( langAbbr, temp );
        if ( matchVal > bestScore )
        {
            bestScore = matchVal;
            bestData = temp;
//          qDebug() << "selected as best" << bestScore;
        }
        if ( matchVal == -2 )
        {
//          qDebug() << "no dict to match, first taken";
            return temp;
        }
    }
    return bestData.size() ? bestData : data;
}


SubtitleDecoder::SubtitleDecoder()
{
    DictionaryThread *dt = new DictionaryThread;
    dt->start();

    QFile json( ":/LangEncodingMap" );

    json.open( QIODevice::ReadOnly | QIODevice::Text );
    QJsonArray jsonArr = QJsonDocument::fromJson( json.readAll() ).array();
    json.close();

    while ( jsonArr.size() )
    {
        QJsonObject item = jsonArr.takeAt( 0 ).toObject();
        QByteArray langAbbr = item.value( "id" ).toString().toLocal8Bit();
        QByteArray langs = item.value( "encodings" ).toString().toLocal8Bit();
        TEncodingList list;
        if ( langs.size() ) list = langs.split( ',' );

        languageToEncoding.insert( langAbbr, list );
//      qDebug() << "lang " << langAbbr << list;
    }


//  QThread *a;
//  while ( ( a = qobject_cast<QThread *>( dt )) && a->isRunning() )
//  {
//      qDebug() << "sleeping";
//      QThread::sleep( 1 );
//  }
//  QFile tmp( "fi.srt" );
//  tmp.open( QIODevice::ReadOnly | QIODevice::Text );
//  QByteArray data = tmp.readAll();
//  tmp.close();
//  qDebug() << "match" << match( "fi", data );
}

int SubtitleDecoder::match( const QByteArray langAbbr, QByteArray utf8Data )
{
    QMap<QByteArray, QMap<QString, short> >::const_iterator i = dictionary.find( langAbbr );
    if ( i == dictionary.end() ) return -2;

    QTextStream txStream( &utf8Data );
    txStream.setCodec( "UTF-8" );
    QString data = txStream.readAll();
//  qDebug() << "matching read";

    QStringList words = data.split( QRegularExpression( "\\W+|\\d+", QRegularExpression::UseUnicodePropertiesOption ), QString::SkipEmptyParts );
//  qDebug() << "matching split";

    int result = 0;
    while ( words.size() )
    {
        QString word = words.takeFirst();
        if ( word.size() >= 3 )
        {
            const int value = i.value().value( word, 0 );
            result += value;
//          qDebug() << word << value << result;
        }
    }
//  qDebug() << "matching end" << result;
    return result;
}

