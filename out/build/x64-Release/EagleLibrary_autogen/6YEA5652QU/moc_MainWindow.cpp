/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../include/MainWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSMainWindowENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSMainWindowENDCLASS = QtMocHelpers::stringData(
    "MainWindow",
    "onScanStart",
    "",
    "onScanFinished",
    "added",
    "skipped",
    "onBookFound",
    "Book",
    "book",
    "onScanProgress",
    "done",
    "total",
    "file",
    "onMetadataReady",
    "id",
    "updated",
    "onCoverReady",
    "path",
    "onCoverUrlsReady",
    "urls",
    "onMetadataFetchProgress",
    "completed",
    "currentFile",
    "stage",
    "onMetadataFetchError",
    "msg",
    "onCoverDownloadProgress",
    "currentLabel",
    "onCoverDownloadFailed",
    "reason",
    "onSmartRenameAll",
    "onSmartRenameSelected",
    "onRenameResult",
    "RenameResult",
    "r",
    "onRenameFinished",
    "changed",
    "openSettings",
    "openBookDetail",
    "QModelIndex",
    "index",
    "setGridView",
    "setListView",
    "searchChanged",
    "text",
    "filterByFormat",
    "fmt",
    "filterByCategory",
    "category",
    "sortChanged",
    "idx",
    "showFavourites",
    "on",
    "showNoCoverFilter",
    "showNoMetaFilter",
    "fetchAllMetadata",
    "enrichIncompleteBooksAction",
    "runQualityCheck",
    "findDuplicates",
    "extractMissingIsbns",
    "countPagesForLibrary",
    "generateMissingCovers",
    "normalizeCovers",
    "smartCategorizeLibrary",
    "removeSelectedBook",
    "cleanMissingFiles",
    "refreshLibrary",
    "exportLibrarySnapshot",
    "importLibrarySnapshot",
    "stopAllTasks",
    "consultDatabaseSummary",
    "diagnoseDatabaseText",
    "repairDatabaseText",
    "openExternalToolsDialog",
    "openDatabaseFolder",
    "openDatabaseEditor",
    "openAdvancedSearch",
    "showAbout",
    "switchLibrary",
    "libraryName",
    "applyShelf",
    "shelfName",
    "switchTheme",
    "name",
    "openCommandPalette",
    "searchSelectedOnGoogle",
    "searchSelectedOnWeb",
    "engine",
    "lookupSelectedOnGoodreads",
    "manageCollections",
    "createCollection",
    "openPluginManager",
    "openAdvancedSearchDialog",
    "loadSavedSearch",
    "saveCurrentSearch"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSMainWindowENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      64,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  398,    2, 0x08,    1 /* Private */,
       3,    2,  399,    2, 0x08,    2 /* Private */,
       6,    1,  404,    2, 0x08,    5 /* Private */,
       9,    3,  407,    2, 0x08,    7 /* Private */,
      13,    2,  414,    2, 0x08,   11 /* Private */,
      16,    2,  419,    2, 0x08,   14 /* Private */,
      18,    2,  424,    2, 0x08,   17 /* Private */,
      20,    4,  429,    2, 0x08,   20 /* Private */,
      24,    2,  438,    2, 0x08,   25 /* Private */,
      26,    3,  443,    2, 0x08,   28 /* Private */,
      28,    2,  450,    2, 0x08,   32 /* Private */,
      30,    0,  455,    2, 0x08,   35 /* Private */,
      31,    0,  456,    2, 0x08,   36 /* Private */,
      32,    1,  457,    2, 0x08,   37 /* Private */,
      35,    1,  460,    2, 0x08,   39 /* Private */,
      37,    0,  463,    2, 0x08,   41 /* Private */,
      38,    1,  464,    2, 0x08,   42 /* Private */,
      38,    0,  467,    2, 0x08,   44 /* Private */,
      41,    0,  468,    2, 0x08,   45 /* Private */,
      42,    0,  469,    2, 0x08,   46 /* Private */,
      43,    1,  470,    2, 0x08,   47 /* Private */,
      45,    1,  473,    2, 0x08,   49 /* Private */,
      47,    1,  476,    2, 0x08,   51 /* Private */,
      49,    1,  479,    2, 0x08,   53 /* Private */,
      51,    1,  482,    2, 0x08,   55 /* Private */,
      53,    1,  485,    2, 0x08,   57 /* Private */,
      54,    1,  488,    2, 0x08,   59 /* Private */,
      55,    0,  491,    2, 0x08,   61 /* Private */,
      56,    0,  492,    2, 0x08,   62 /* Private */,
      57,    0,  493,    2, 0x08,   63 /* Private */,
      58,    0,  494,    2, 0x08,   64 /* Private */,
      59,    0,  495,    2, 0x08,   65 /* Private */,
      60,    0,  496,    2, 0x08,   66 /* Private */,
      61,    0,  497,    2, 0x08,   67 /* Private */,
      62,    0,  498,    2, 0x08,   68 /* Private */,
      63,    0,  499,    2, 0x08,   69 /* Private */,
      64,    0,  500,    2, 0x08,   70 /* Private */,
      65,    0,  501,    2, 0x08,   71 /* Private */,
      66,    0,  502,    2, 0x08,   72 /* Private */,
      67,    0,  503,    2, 0x08,   73 /* Private */,
      68,    0,  504,    2, 0x08,   74 /* Private */,
      69,    0,  505,    2, 0x08,   75 /* Private */,
      70,    0,  506,    2, 0x08,   76 /* Private */,
      71,    0,  507,    2, 0x08,   77 /* Private */,
      72,    0,  508,    2, 0x08,   78 /* Private */,
      73,    0,  509,    2, 0x08,   79 /* Private */,
      74,    0,  510,    2, 0x08,   80 /* Private */,
      75,    0,  511,    2, 0x08,   81 /* Private */,
      76,    0,  512,    2, 0x08,   82 /* Private */,
      77,    0,  513,    2, 0x08,   83 /* Private */,
      78,    1,  514,    2, 0x08,   84 /* Private */,
      80,    1,  517,    2, 0x08,   86 /* Private */,
      82,    1,  520,    2, 0x08,   88 /* Private */,
      84,    0,  523,    2, 0x08,   90 /* Private */,
      85,    0,  524,    2, 0x08,   91 /* Private */,
      86,    1,  525,    2, 0x08,   92 /* Private */,
      86,    0,  528,    2, 0x28,   94 /* Private | MethodCloned */,
      88,    0,  529,    2, 0x08,   95 /* Private */,
      89,    0,  530,    2, 0x08,   96 /* Private */,
      90,    0,  531,    2, 0x08,   97 /* Private */,
      91,    0,  532,    2, 0x08,   98 /* Private */,
      92,    0,  533,    2, 0x08,   99 /* Private */,
      93,    1,  534,    2, 0x08,  100 /* Private */,
      94,    0,  537,    2, 0x08,  102 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    4,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,   10,   11,   12,
    QMetaType::Void, QMetaType::LongLong, 0x80000000 | 7,   14,   15,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,   14,   17,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QStringList,   14,   19,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString, QMetaType::QString,   21,   11,   22,   23,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,   14,   25,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,   21,   11,   27,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,   14,   29,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 33,   34,
    QMetaType::Void, QMetaType::Int,   36,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   44,
    QMetaType::Void, QMetaType::QString,   46,
    QMetaType::Void, QMetaType::QString,   48,
    QMetaType::Void, QMetaType::Int,   50,
    QMetaType::Void, QMetaType::Bool,   52,
    QMetaType::Void, QMetaType::Bool,   52,
    QMetaType::Void, QMetaType::Bool,   52,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   79,
    QMetaType::Void, QMetaType::QString,   81,
    QMetaType::Void, QMetaType::QString,   83,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   87,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_CLASSMainWindowENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSMainWindowENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSMainWindowENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'onScanStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onScanFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onBookFound'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<Book, std::false_type>,
        // method 'onScanProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onMetadataReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<Book, std::false_type>,
        // method 'onCoverReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onCoverUrlsReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        // method 'onMetadataFetchProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onMetadataFetchError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onCoverDownloadProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onCoverDownloadFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onSmartRenameAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSmartRenameSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRenameResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<RenameResult, std::false_type>,
        // method 'onRenameFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'openSettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openBookDetail'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        // method 'openBookDetail'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setGridView'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setListView'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'filterByFormat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'filterByCategory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sortChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'showFavourites'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'showNoCoverFilter'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'showNoMetaFilter'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'fetchAllMetadata'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enrichIncompleteBooksAction'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runQualityCheck'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'findDuplicates'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'extractMissingIsbns'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'countPagesForLibrary'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'generateMissingCovers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'normalizeCovers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'smartCategorizeLibrary'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'removeSelectedBook'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cleanMissingFiles'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshLibrary'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportLibrarySnapshot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'importLibrarySnapshot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopAllTasks'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'consultDatabaseSummary'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'diagnoseDatabaseText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'repairDatabaseText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openExternalToolsDialog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openDatabaseFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openDatabaseEditor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openAdvancedSearch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showAbout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'switchLibrary'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'applyShelf'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'switchTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'openCommandPalette'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchSelectedOnGoogle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'searchSelectedOnWeb'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'searchSelectedOnWeb'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lookupSelectedOnGoodreads'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'manageCollections'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createCollection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openPluginManager'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openAdvancedSearchDialog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadSavedSearch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'saveCurrentSearch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onScanStart(); break;
        case 1: _t->onScanFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->onBookFound((*reinterpret_cast< std::add_pointer_t<Book>>(_a[1]))); break;
        case 3: _t->onScanProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 4: _t->onMetadataReady((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Book>>(_a[2]))); break;
        case 5: _t->onCoverReady((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 6: _t->onCoverUrlsReady((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 7: _t->onMetadataFetchProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 8: _t->onMetadataFetchError((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 9: _t->onCoverDownloadProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 10: _t->onCoverDownloadFailed((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->onSmartRenameAll(); break;
        case 12: _t->onSmartRenameSelected(); break;
        case 13: _t->onRenameResult((*reinterpret_cast< std::add_pointer_t<RenameResult>>(_a[1]))); break;
        case 14: _t->onRenameFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->openSettings(); break;
        case 16: _t->openBookDetail((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 17: _t->openBookDetail(); break;
        case 18: _t->setGridView(); break;
        case 19: _t->setListView(); break;
        case 20: _t->searchChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: _t->filterByFormat((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 22: _t->filterByCategory((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 23: _t->sortChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 24: _t->showFavourites((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 25: _t->showNoCoverFilter((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 26: _t->showNoMetaFilter((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 27: _t->fetchAllMetadata(); break;
        case 28: _t->enrichIncompleteBooksAction(); break;
        case 29: _t->runQualityCheck(); break;
        case 30: _t->findDuplicates(); break;
        case 31: _t->extractMissingIsbns(); break;
        case 32: _t->countPagesForLibrary(); break;
        case 33: _t->generateMissingCovers(); break;
        case 34: _t->normalizeCovers(); break;
        case 35: _t->smartCategorizeLibrary(); break;
        case 36: _t->removeSelectedBook(); break;
        case 37: _t->cleanMissingFiles(); break;
        case 38: _t->refreshLibrary(); break;
        case 39: _t->exportLibrarySnapshot(); break;
        case 40: _t->importLibrarySnapshot(); break;
        case 41: _t->stopAllTasks(); break;
        case 42: _t->consultDatabaseSummary(); break;
        case 43: _t->diagnoseDatabaseText(); break;
        case 44: _t->repairDatabaseText(); break;
        case 45: _t->openExternalToolsDialog(); break;
        case 46: _t->openDatabaseFolder(); break;
        case 47: _t->openDatabaseEditor(); break;
        case 48: _t->openAdvancedSearch(); break;
        case 49: _t->showAbout(); break;
        case 50: _t->switchLibrary((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 51: _t->applyShelf((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 52: _t->switchTheme((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 53: _t->openCommandPalette(); break;
        case 54: _t->searchSelectedOnGoogle(); break;
        case 55: _t->searchSelectedOnWeb((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 56: _t->searchSelectedOnWeb(); break;
        case 57: _t->lookupSelectedOnGoodreads(); break;
        case 58: _t->manageCollections(); break;
        case 59: _t->createCollection(); break;
        case 60: _t->openPluginManager(); break;
        case 61: _t->openAdvancedSearchDialog(); break;
        case 62: _t->loadSavedSearch((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 63: _t->saveCurrentSearch(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSMainWindowENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 64)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 64;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 64)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 64;
    }
    return _id;
}
QT_WARNING_POP
