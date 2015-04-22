#ifndef __COMMON_TYPES_H_INCL__
    #define __COMMON_TYPES_H_INCL__

    #include <QByteArray>
    #include <QString>
    #include <QMetaType>
    #include <QNetworkProxy>

class SubtitleItem
{
public:
    SubtitleItem() { }
    SubtitleItem( QString theLang, QByteArray theAbbr ) : language( theLang ), langAbbr( theAbbr ) { }
    SubtitleItem( const SubtitleItem& itm ) : language( itm.language ), langAbbr( itm.langAbbr ), zipData( itm.zipData ) { }
    QString language;
    QByteArray langAbbr;
    QByteArray zipData;
};

Q_DECLARE_METATYPE( SubtitleItem );


struct dlInfo;

struct DownloadInfo
{
    DownloadInfo();
    DownloadInfo( const DownloadInfo& info );
    DownloadInfo( const dlInfo& info );
    long long downloadRateBs;
    long long downloaded;
    long long total;
    long long piecesDone;
    long long piecesTotal;
    long long pieceSize;
    long long seeders;
    long long peers;
};

Q_DECLARE_METATYPE( DownloadInfo );


struct ProxySettings
{
    ProxySettings() : port( 0 ) { }
    bool isValid() { return host.size() && port > 0; }
    QNetworkProxy networkProxy()
    {
        if ( !isValid() ) return QNetworkProxy( QNetworkProxy::NoProxy );
        return QNetworkProxy( QNetworkProxy::Socks5Proxy, host, port, user, pass );
    }

    QByteArray host;
    int port;
    QByteArray user;
    QByteArray pass;
};

typedef QList<ProxySettings> TProxyList;


Q_DECLARE_METATYPE( ProxySettings );

void registerMetaTypes();

#endif
