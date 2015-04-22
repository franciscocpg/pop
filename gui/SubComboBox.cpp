#include <QBuffer>
#include <QFileInfo>
#include <QDir>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include "defaults.h"
#include "SubComboBox.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

SubComboBox::SubComboBox() : subsTempFile( 0 )
{
    // Sortable ComboBox
    QStandardItemModel* model = new QStandardItemModel(0, 1, this);
    QSortFilterProxyModel* proxy = new QSortFilterProxyModel(this);
    // may be enabled for dynamic sorting
//  proxy->setDynamicSortFilter(true);
    proxy->setSourceModel(model);
    setModel(proxy);

    setStyleSheet( "background-color:black; color:gray; padding: 2px 20px 2px 1px;" );
    this->setSizeAdjustPolicy( QComboBox::AdjustToContents );
//  this->setInsertPolicy( QComboBox::InsertAlphabetically );
    this->setEditable( false );
    connect( this, SIGNAL( activated( const QString& ) ), this, SLOT( subtitleSelected( const QString& ) ) );
}

void SubComboBox::sort(Qt::SortOrder sortOrder)
{
	model()->sort(-1); // required to reapply sorting
	model()->sort(0, sortOrder);
}

void SubComboBox::addSubtitle( SubtitleItem item )
{
//  qDebug() << "haveSubtitles " << item.langAbbr << ":" << item.language << item.zipData.size();
    if ( subs.isEmpty() )
    {
        this->clear();
        this->addItem( SUBTITLES_OFF );
        this->setFocusPolicy( Qt::StrongFocus );
        this->setEnabled( true );
    }
    subs.append( item );
    this->insertItem( 0, item.language );
    this->sort( Qt::AscendingOrder );
}

void SubComboBox::clearSubtitles()
{
    subs.clear();
    this->clear();
    this->addItem( SUBTITLES_NA );
    this->setFocusPolicy( Qt::NoFocus );
    this->setEnabled( false );
}

void SubComboBox::subtitleSelected( const QString& text )
{
    if ( text == SUBTITLES_OFF ) return;

    for ( int i = 0; i < subs.size(); ++i )
    {
        SubtitleItem item = subs.at( i );
        if ( item.language != text ) continue;
        QByteArray compressed = item.zipData;

        QBuffer buf( &compressed );
        QuaZip zip( &buf );
        zip.open( QuaZip::mdUnzip );
        QFileInfo fi( zip.getFileNameList().first() );

        QuaZipFile file( &zip );
        zip.goToFirstFile();
        file.open( QIODevice::ReadOnly );
        if ( subsTempFile ) delete subsTempFile;
        subsTempFile = new QFile( QDir::tempPath() + QDir::separator() + item.langAbbr + fi.fileName(), this );
        subsTempFile->open( QIODevice::WriteOnly | QIODevice::Text );
        subsTempFile->write( sDecoder.decodeToUtf8( item.langAbbr, file.readAll() ) );
        subsTempFile->close();
        file.close();
        zip.close();

        emit haveSubtitleFile( subsTempFile->fileName() );
        return;
    }
}


