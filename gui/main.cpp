#include <QApplication>
#include <QWebSettings>

#include "FramelessMainWindow.h"
#include "GraphicsBrowser.h"
#include "vlc.h"
#include "defaults.h"
#include "commontypes.h"

#include <QPushButton>
#include <QMessageBox>
#include <QThread>
#include <QDir>
#include <QStyleFactory>

#ifdef Q_OS_WIN
    #include <Windows.h>
    #include <QtDebug>
    #include <QtGlobal>

void MyMessageOutput( QtMsgType Type, const QMessageLogContext& Context, const QString& Message )
{
    (void) Type;
    (void) Context;
    const char symbols[] = { 'D', 'W', 'C', 'F' };
    QString output = QString( "[%1] %2" ).arg( symbols[Type] ).arg( Message );
    OutputDebugString( reinterpret_cast<const wchar_t *>( output.utf16() ) );
}
#endif

#ifdef Q_OS_MAC
    #include <syslog.h>
    #include <QtDebug>
void MyMessageOutput( QtMsgType Type, const QMessageLogContext& Context, const QString& Message )
{
    (void) Type;
    (void) Context;
    const char symbols[] = { 'D', 'W', 'C', 'F' };
    QString output = QString( "[%1] %2" ).arg( symbols[Type] ).arg( Message );
    syslog( LOG_NOTICE, "%s", output.toLocal8Bit().constData() );
}
#endif


int main( int argc, char **argv )
{
//  QDir dir( argv[0] );  // e.g. appdir/Contents/MacOS/appname
//  dir.cdUp();
//  dir.cdUp();
//  dir.cd( "PlugIns" );  // e.g. appdir/Contents/PlugIns
//  QCoreApplication::setLibraryPaths( QStringList( dir.absolutePath() ) );
//  printf( "after change, libraryPaths=(%s)\n", QCoreApplication::libraryPaths().join( "," ).toUtf8().data() );

    QApplication app( argc, argv );
    QApplication::setStyle( QStyleFactory::create( "Fusion" ) );
//#ifndef Q_OS_MAC
    qInstallMessageHandler( MyMessageOutput );
//#endif
//  qDebug() << "Start! LibraryPaths:" << QCoreApplication::libraryPaths();

    QWebSettings::globalSettings()->setLocalStoragePath( QDir::toNativeSeparators( QDir::tempPath() + QDir::separator() + "PopcornTime" ) );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::LocalStorageEnabled, true );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::JavascriptCanOpenWindows, true );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::JavascriptCanCloseWindows, true );
   
#ifdef _DEBUGGING
    QWebSettings::globalSettings()->setAttribute( QWebSettings::DeveloperExtrasEnabled, true );
#endif

    registerMetaTypes();

    VLC vlc;

    FramelessMainWindow win;

    win.browser()->webView()->load( QUrl( STARTUP_URL ) );

    win.resize( DEFAULT_WIDTH, DEFAULT_HEIGHT );
    win.show();

    return app.exec();
}

