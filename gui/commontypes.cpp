#include "commontypes.h"
#include "lib.h"

DownloadInfo::DownloadInfo() :
   downloadRateBs( 0 ),
   downloaded( 0 ), total( 0 ),
   piecesDone( 0 ), piecesTotal( 0 ), pieceSize( 0 ),
   seeders( 0 ), peers( 0 )
{ }

DownloadInfo::DownloadInfo( const DownloadInfo& info ) :
   downloadRateBs( info.downloadRateBs ),
   downloaded( info.downloaded ), total( info.total ),
   piecesDone( info.piecesDone ), piecesTotal( info.piecesTotal ), pieceSize( info.pieceSize ),
   seeders( info.seeders ), peers( info.peers )
{ }

DownloadInfo::DownloadInfo( const dlInfo& info ) :
   downloadRateBs( info.downloadRateBs ),
   downloaded( info.downloaded ), total( info.total ),
   piecesDone( info.piecesDone ), piecesTotal( info.piecesTotal ), pieceSize( info.pieceSize ),
   seeders( info.seeders ), peers( info.peers )
{ }

void registerMetaTypes()
{
    qRegisterMetaType<DownloadInfo>( "DownloadInfo" );
    qRegisterMetaType<ProxySettings>( "ProxySettings" );
}
