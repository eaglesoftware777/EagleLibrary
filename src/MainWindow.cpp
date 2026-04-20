// ============================================================
//  Eagle Library -- MainWindow.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================
#include "MainWindow.h"
#include "BookDelegate.h"
#include "BookDetailDialog.h"
#include "SettingsDialog.h"
#include "Database.h"
#include "AppConfig.h"
#include "LanguageManager.h"
#include "ThemeManager.h"
#include "CommandPalette.h"
#include "PluginManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
#include <QDockWidget>
#include <QFrame>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QProgressBar>
#include <QStatusBar>
#include <QScrollArea>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QScrollBar>
#include <QShortcut>
#include <QKeySequence>
#include <QGuiApplication>
#include <QScreen>
#include <QIcon>
#include <QSignalBlocker>
#include <functional>
#include <QSet>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QRegularExpression>
#include <QSaveFile>
#include <QJsonDocument>
#include <QPushButton>
#include <QResizeEvent>
#include <QItemSelectionModel>
#include <QDateTime>
#include <QEventLoop>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QMap>
#include <QStyle>
#include <QTextStream>
#include <QUrlQuery>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <algorithm>

namespace {

QString trl(const QString& key, const QString& fallback)
{
    return LanguageManager::instance().text(key, fallback);
}

QString shelfLabel(const QString& id)
{
    if (id == QStringLiteral("all")) return trl("shelf.allItems", "All Items");
    if (id == QStringLiteral("books")) return trl("shelf.booksOnly", "Books Only");
    if (id == QStringLiteral("documents")) return trl("shelf.documentsOnly", "Documents Only");
    if (id == QStringLiteral("favourites")) return trl("shelf.favourites", "Favourites");
    if (id == QStringLiteral("recent")) return trl("shelf.recentlyAdded", "Recently Added");
    if (id == QStringLiteral("opened")) return trl("shelf.mostOpened", "Most Opened");
    if (id == QStringLiteral("missing_metadata")) return trl("shelf.missingMetadata", "Missing Metadata");
    if (id == QStringLiteral("no_cover")) return trl("shelf.noCover", "No Cover");
    return id;
}

QString normalizeShelfId(const QString& value)
{
    if (value == QStringLiteral("all") || value == QStringLiteral("All Items")) return QStringLiteral("all");
    if (value == QStringLiteral("books") || value == QStringLiteral("Books Only")) return QStringLiteral("books");
    if (value == QStringLiteral("documents") || value == QStringLiteral("Documents Only")) return QStringLiteral("documents");
    if (value == QStringLiteral("favourites") || value == QStringLiteral("Favourites")) return QStringLiteral("favourites");
    if (value == QStringLiteral("recent") || value == QStringLiteral("Recently Added")) return QStringLiteral("recent");
    if (value == QStringLiteral("opened") || value == QStringLiteral("Most Opened")) return QStringLiteral("opened");
    if (value == QStringLiteral("missing_metadata") || value == QStringLiteral("Missing Metadata")) return QStringLiteral("missing_metadata");
    if (value == QStringLiteral("no_cover") || value == QStringLiteral("No Cover")) return QStringLiteral("no_cover");
    return value.trimmed().isEmpty() ? QStringLiteral("all") : value;
}

Book mergeFetchedMetadata(const Book& current, const Book& fetched)
{
    Book merged = current;
    if (!fetched.title.isEmpty())       merged.title = fetched.title;
    if (!fetched.author.isEmpty())      merged.author = fetched.author;
    if (!fetched.publisher.isEmpty())   merged.publisher = fetched.publisher;
    if (!fetched.isbn.isEmpty())        merged.isbn = fetched.isbn;
    if (!fetched.language.isEmpty())    merged.language = fetched.language;
    if (fetched.year > 0)               merged.year = fetched.year;
    if (fetched.pages > 0)              merged.pages = fetched.pages;
    if (fetched.rating > 0)             merged.rating = fetched.rating;
    if (!fetched.description.isEmpty()) merged.description = fetched.description;
    if (!fetched.subjects.isEmpty())    merged.subjects = fetched.subjects;
    return merged;
}

QString normalizedText(QString value)
{
    value = value.toLower();
    value.remove(QRegularExpression("[^\\p{L}\\p{N}]+"));
    return value;
}

bool isValidIsbn10(const QString& isbn)
{
    if (isbn.size() != 10)
        return false;
    int sum = 0;
    for (int i = 0; i < 9; ++i) {
        if (!isbn.at(i).isDigit())
            return false;
        sum += (10 - i) * isbn.at(i).digitValue();
    }
    const QChar last = isbn.at(9).toUpper();
    sum += (last == 'X') ? 10 : (last.isDigit() ? last.digitValue() : -1000);
    return sum % 11 == 0;
}

bool isValidIsbn13(const QString& isbn)
{
    if (isbn.size() != 13)
        return false;
    int sum = 0;
    for (int i = 0; i < 12; ++i) {
        if (!isbn.at(i).isDigit())
            return false;
        sum += isbn.at(i).digitValue() * ((i % 2 == 0) ? 1 : 3);
    }
    if (!isbn.at(12).isDigit())
        return false;
    const int check = (10 - (sum % 10)) % 10;
    return isbn.at(12).digitValue() == check;
}

QString sanitizeIsbn(const QString& value)
{
    QString out;
    for (const QChar ch : value) {
        if (ch.isDigit() || ch.toUpper() == 'X')
            out.append(ch.toUpper());
    }
    return out;
}

QString extractIsbnFromText(const QString& text)
{
    static const QRegularExpression re(QStringLiteral(R"((?:97[89][-\s]?)?\d[-\s\d]{8,20}[\dXx])"));
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        const QString isbn = sanitizeIsbn(it.next().captured(0));
        if (isbn.size() == 13 && isValidIsbn13(isbn))
            return isbn;
        if (isbn.size() == 10 && isValidIsbn10(isbn))
            return isbn;
    }
    return {};
}

QString readBookProbeText(const Book& book)
{
    QFile file(book.filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QByteArray bytes = file.read(1024 * 1024);
    if (file.size() > 1024 * 1024 && file.seek(qMax<qint64>(0, file.size() - 256 * 1024)))
        bytes += '\n' + file.read(256 * 1024);

    QString text = QString::fromUtf8(bytes);
    if (text.contains(QChar::ReplacementCharacter))
        text = QString::fromLatin1(bytes);
    return text;
}

QString runCommandCapture(const QString& program, const QStringList& arguments, int timeoutMs = 20000)
{
    QProcess proc;
    proc.start(program, arguments);
    if (!proc.waitForStarted(3000))
        return {};
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(1000);
        return {};
    }

    const QByteArray stdoutBytes = proc.readAllStandardOutput();
    const QByteArray stderrBytes = proc.readAllStandardError();
    QString text = QString::fromUtf8(stdoutBytes);
    if (text.contains(QChar::ReplacementCharacter))
        text = QString::fromLocal8Bit(stdoutBytes);
    if (text.trimmed().isEmpty())
        text = QString::fromLocal8Bit(stderrBytes);
    return text.trimmed();
}

QString extractTextWithPdftotext(const Book& book)
{
    const QString tool = QStandardPaths::findExecutable("pdftotext");
    if (tool.isEmpty())
        return {};

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
        return {};

    const QString outputPath = tempDir.filePath("probe.txt");
    QProcess proc;
    proc.start(tool, QStringList() << "-enc" << "UTF-8" << "-f" << "1" << "-l" << "5" << book.filePath << outputPath);
    if (!proc.waitForStarted(3000))
        return {};
    if (!proc.waitForFinished(25000)) {
        proc.kill();
        proc.waitForFinished(1000);
        return {};
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::ReadOnly))
        return {};
    QString text = QString::fromUtf8(outFile.readAll());
    return text.trimmed();
}

QString ocrPdfWithExternalTools(const Book& book)
{
    const QString pdftoppm = QStandardPaths::findExecutable("pdftoppm");
    const QString tesseract = QStandardPaths::findExecutable("tesseract");
    if (pdftoppm.isEmpty() || tesseract.isEmpty())
        return {};

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
        return {};

    const QString prefix = tempDir.filePath("page");
    QProcess renderProc;
    renderProc.start(pdftoppm, QStringList() << "-png" << "-f" << "1" << "-l" << "2" << book.filePath << prefix);
    if (!renderProc.waitForStarted(3000))
        return {};
    if (!renderProc.waitForFinished(30000)) {
        renderProc.kill();
        renderProc.waitForFinished(1000);
        return {};
    }

    QString combined;
    const QStringList images = QDir(tempDir.path()).entryList(QStringList() << "page-*.png", QDir::Files, QDir::Name);
    for (const QString& imageName : images) {
        const QString imagePath = tempDir.filePath(imageName);
        const QString text = runCommandCapture(tesseract, QStringList() << imagePath << "stdout" << "-l" << "eng", 25000);
        if (!text.isEmpty()) {
            if (!combined.isEmpty())
                combined += '\n';
            combined += text;
        }
    }

    return combined.trimmed();
}

QString ocrImageArchiveWithTesseract(const Book& book)
{
    const QString tesseract = QStandardPaths::findExecutable("tesseract");
    if (tesseract.isEmpty())
        return {};

    const QString lowerFormat = book.format.toLower();
    if (lowerFormat != "cbz" && lowerFormat != "cbr")
        return {};

    return {};
}

QString ocrExistingCoverWithTesseract(const Book& book)
{
    const QString tesseract = QStandardPaths::findExecutable("tesseract");
    if (tesseract.isEmpty() || book.coverPath.isEmpty() || !QFileInfo::exists(book.coverPath))
        return {};

    return runCommandCapture(tesseract, QStringList() << book.coverPath << "stdout" << "-l" << "eng", 20000);
}

QStringList uniqueCaseInsensitive(QStringList values)
{
    QStringList out;
    QSet<QString> seen;
    for (QString value : values) {
        value = value.trimmed();
        if (value.isEmpty())
            continue;
        const QString key = value.toLower();
        if (!seen.contains(key)) {
            seen.insert(key);
            out << value;
        }
    }
    return out;
}

QJsonObject loadLibraryProfiles(const QSettings& settings)
{
    const QJsonDocument doc = QJsonDocument::fromJson(settings.value("library/profiles").toByteArray());
    if (doc.isObject() && !doc.object().isEmpty())
        return doc.object();

    QJsonObject fallback;
    QJsonArray folders;
    const QStringList legacyFolders = settings.value("library/folders").toStringList();
    for (const QString& folder : legacyFolders)
        folders.append(folder);
    fallback.insert(settings.value("library/currentName", "Main Library").toString(), folders);
    return fallback;
}

QStringList foldersForLibrary(const QJsonObject& profiles, const QString& libraryName)
{
    QStringList folders;
    for (const QJsonValue& value : profiles.value(libraryName).toArray())
        folders << value.toString();
    folders.removeDuplicates();
    folders.sort(Qt::CaseInsensitive);
    return folders;
}

QString buildLookupQuery(const Book& book)
{
    QStringList parts;
    if (!book.displayTitle().trimmed().isEmpty())
        parts << book.displayTitle().trimmed();
    if (!book.author.trimmed().isEmpty())
        parts << book.author.trimmed();
    if (!book.isbn.trimmed().isEmpty())
        parts << book.isbn.trimmed();
    return parts.join(' ');
}

QUrl searchUrl(const QString& baseUrl, const QString& queryKey, const QString& term)
{
    QUrl url(baseUrl);
    QUrlQuery query;
    query.addQueryItem(queryKey, term);
    url.setQuery(query);
    return url;
}

QString csvEscape(QString value)
{
    value.replace('"', "\"\"");
    if (value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r'))
        value = '"' + value + '"';
    return value;
}

bool hasControlCharacters(const QString& value)
{
    for (const QChar ch : value) {
        if (ch.category() == QChar::Other_Control && ch != QChar('\n') && ch != QChar('\r') && ch != QChar('\t'))
            return true;
    }
    return false;
}

bool hasReplacementGlyphs(const QString& value)
{
    return value.contains(QChar::ReplacementCharacter)
        || value.contains(QStringLiteral("\u25A1"))
        || value.contains(QStringLiteral("\u25A0"))
        || value.contains(QStringLiteral("\uFFFD"));
}

bool looksLikeMojibake(const QString& value)
{
    static const QStringList markers = {
        QStringLiteral("Ã"), QStringLiteral("Â"), QStringLiteral("Ð"),
        QStringLiteral("Ñ"), QStringLiteral("â€"), QStringLiteral("â€œ"),
        QStringLiteral("â€"), QStringLiteral("â€“"), QStringLiteral("â€”"),
        QStringLiteral("ï»¿")
    };
    for (const QString& marker : markers) {
        if (value.contains(marker))
            return true;
    }
    return false;
}

bool looksLikePdfTitleNoise(const QString& value)
{
    const QString lowered = value.toLower();
    static const QStringList blockedTokens = {
        QStringLiteral("/filter"),
        QStringLiteral("/flatedecode"),
        QStringLiteral("/ascii85decode"),
        QStringLiteral("/lzwdecode"),
        QStringLiteral("/length"),
        QStringLiteral("/type"),
        QStringLiteral("/catalog"),
        QStringLiteral("/pages"),
        QStringLiteral("/metadata"),
        QStringLiteral("endstream"),
        QStringLiteral("endobj"),
        QStringLiteral("xref"),
        QStringLiteral("linearized"),
        QStringLiteral(" obj<<"),
        QStringLiteral(" obj <")
    };
    for (const QString& token : blockedTokens) {
        if (lowered.contains(token))
            return true;
    }

    static const QRegularExpression objRefRe(QStringLiteral(R"(\b\d+\s+\d+\s+obj\b)"),
                                             QRegularExpression::CaseInsensitiveOption);
    return objRefRe.match(lowered).hasMatch();
}

bool looksLikeBinaryishText(const QString& value)
{
    if (value.size() < 8)
        return false;

    int letters = 0;
    int digits = 0;
    int spaces = 0;
    int symbols = 0;
    int controls = 0;
    for (const QChar ch : value) {
        if (ch.isLetter())
            ++letters;
        else if (ch.isDigit())
            ++digits;
        else if (ch.isSpace())
            ++spaces;
        else if (ch.category() == QChar::Other_Control)
            ++controls;
        else
            ++symbols;
    }

    return controls > 0
        || (letters < 3 && symbols >= value.size() / 3)
        || ((letters + digits + spaces) * 2 < value.size());
}

QStringList suspiciousTextReasons(const Book& book)
{
    QStringList reasons;
    const QString title = book.title.trimmed();
    const QString author = book.author.trimmed();
    const QString publisher = book.publisher.trimmed();
    const QString fileBase = QFileInfo(book.filePath).completeBaseName().trimmed();

    const auto inspect = [&](const QString& fieldName, const QString& value) {
        if (value.isEmpty())
            return;
        if (hasReplacementGlyphs(value))
            reasons << QString("%1 has replacement/square characters").arg(fieldName);
        if (hasControlCharacters(value))
            reasons << QString("%1 has control characters").arg(fieldName);
        if (looksLikeMojibake(value))
            reasons << QString("%1 looks mojibake-encoded").arg(fieldName);
        if (fieldName == "Title" && looksLikePdfTitleNoise(value))
            reasons << "Title contains PDF structure tokens";
        if (fieldName == "Title" && looksLikeBinaryishText(value))
            reasons << "Title looks like binary/noise data";
    };

    inspect("Title", title);
    inspect("Author", author);
    inspect("Publisher", publisher);

    if (title.isEmpty())
        reasons << "Title is empty";
    else if (!fileBase.isEmpty() && title.compare(fileBase, Qt::CaseInsensitive) == 0)
        reasons << "Title still equals raw filename";
    else if (title.size() <= 2)
        reasons << "Title is suspiciously short";

    if (!title.isEmpty()) {
        int punctuationCount = 0;
        for (const QChar ch : title) {
            if (!ch.isLetterOrNumber() && !ch.isSpace())
                ++punctuationCount;
        }
        if (title.size() >= 6 && punctuationCount > title.size() / 2)
            reasons << "Title has too much punctuation/noise";
    }

    if (!author.isEmpty() && author.size() <= 1)
        reasons << "Author is suspiciously short";

    reasons.removeDuplicates();
    return reasons;
}

struct ReferenceTarget {
    QString label;
    QUrl url;
};

bool isTrustedReferenceUrl(const QUrl& url)
{
    if (!url.isValid() || url.scheme() != "https")
        return false;

    static const QHash<QString, QStringList> allowedHosts = {
        { "en.wikipedia.org", { "/w/index.php" } },
        { "www.amazon.com", { "/s" } },
        { "openlibrary.org", { "/search" } },
        { "books.google.com", { "/books" } },
        { "search.worldcat.org", { "/search" } },
        { "catalog.loc.gov", { "/vwebv/search" } }
    };

    const QString host = url.host().toLower();
    if (!allowedHosts.contains(host))
        return false;

    const QString path = url.path();
    for (const QString& allowedPath : allowedHosts.value(host)) {
        if (path == allowedPath)
            return true;
    }
    return false;
}

QVector<ReferenceTarget> trustedReferenceTargets(const Book& book)
{
    const QString query = buildLookupQuery(book).trimmed();
    if (query.isEmpty())
        return {};

    const QVector<ReferenceTarget> candidates = {
        { "Wikipedia", searchUrl("https://en.wikipedia.org/w/index.php", "search", query) },
        { "Amazon Books", searchUrl("https://www.amazon.com/s", "k", query) },
        { "Open Library", searchUrl("https://openlibrary.org/search", "q", query) },
        { "Google Books", searchUrl("https://books.google.com/books", "q", query) },
        { "WorldCat", searchUrl("https://search.worldcat.org/search", "q", query) },
        { "Library of Congress", searchUrl("https://catalog.loc.gov/vwebv/search", "searchArg", query) }
    };

    QVector<ReferenceTarget> valid;
    for (const ReferenceTarget& candidate : candidates) {
        if (isTrustedReferenceUrl(candidate.url))
            valid.push_back(candidate);
    }
    return valid;
}

QString verifiedContentIsbn(const Book& book)
{
    if (book.format.compare("PDF", Qt::CaseInsensitive) == 0) {
        QString text = extractTextWithPdftotext(book);
        if (text.trimmed().isEmpty())
            text = ocrPdfWithExternalTools(book);
        return extractIsbnFromText(text);
    }

    return extractIsbnFromText(readBookProbeText(book));
}

bool exportBooksToCsv(const QString& path, const QVector<Book>& books)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "title,author,publisher,year,format,isbn,language,pages,rating,category,tags,file_path\n";
    for (const Book& book : books) {
        out << csvEscape(book.displayTitle()) << ','
            << csvEscape(book.author) << ','
            << csvEscape(book.publisher) << ','
            << csvEscape(book.year > 0 ? QString::number(book.year) : QString()) << ','
            << csvEscape(book.format) << ','
            << csvEscape(book.isbn) << ','
            << csvEscape(book.language) << ','
            << csvEscape(book.pages > 0 ? QString::number(book.pages) : QString()) << ','
            << csvEscape(book.rating > 0 ? QString::number(book.rating, 'f', 2) : QString()) << ','
            << csvEscape(book.classificationTag()) << ','
            << csvEscape(book.tags.join("; ")) << ','
            << csvEscape(book.filePath) << '\n';
    }
    return file.commit();
}

bool exportBooksToBibTex(const QString& path, const QVector<Book>& books)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    int index = 1;
    for (const Book& book : books) {
        const QString entryKey = QString("%1%2%3")
            .arg(normalizedText(book.author).left(12).isEmpty() ? QStringLiteral("eagle") : normalizedText(book.author).left(12))
            .arg(book.year > 0 ? QString::number(book.year) : QString::number(index))
            .arg(normalizedText(book.displayTitle()).left(18).isEmpty() ? QString::number(index) : normalizedText(book.displayTitle()).left(18));
        out << "@book{" << entryKey << ",\n";
        out << "  title = {" << book.displayTitle() << "},\n";
        if (!book.author.isEmpty()) out << "  author = {" << book.author << "},\n";
        if (!book.publisher.isEmpty()) out << "  publisher = {" << book.publisher << "},\n";
        if (book.year > 0) out << "  year = {" << book.year << "},\n";
        if (!book.isbn.isEmpty()) out << "  isbn = {" << book.isbn << "},\n";
        if (!book.language.isEmpty()) out << "  language = {" << book.language << "},\n";
        out << "  note = {" << book.filePath << "}\n";
        out << "}\n\n";
        ++index;
    }
    return file.commit();
}

bool exportBooksToRis(const QString& path, const QVector<Book>& books)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    for (const Book& book : books) {
        out << "TY  - " << (book.isLikelyDocument() ? "GEN" : "BOOK") << '\n';
        out << "TI  - " << book.displayTitle() << '\n';
        if (!book.author.isEmpty()) out << "AU  - " << book.author << '\n';
        if (!book.publisher.isEmpty()) out << "PB  - " << book.publisher << '\n';
        if (book.year > 0) out << "PY  - " << book.year << '\n';
        if (!book.isbn.isEmpty()) out << "SN  - " << book.isbn << '\n';
        if (!book.language.isEmpty()) out << "LA  - " << book.language << '\n';
        out << "N1  - " << book.filePath << '\n';
        out << "ER  - \n\n";
    }
    return file.commit();
}

QStringList inferSmartTags(const Book& book)
{
    struct CategoryRule {
        QString label;
        QStringList keywords;
    };

    static const QVector<CategoryRule> rules = {
        { "Programming", {"programming","software","developer","algorithm","coding","source code","api","object oriented","design pattern","c++","java","python","c#","rust","go "} },
        { "Web", {"html","css","javascript","typescript","react","frontend","backend","node.js","php","web development","http"} },
        { "Databases", {"database","sql","postgres","mysql","sqlite","oracle","query optimization","nosql"} },
        { "Data Science", {"machine learning","predictive modeling","neural","data science","deep learning","nlp","statistics","pandas","tensorflow"} },
        { "Networking", {"network","tcp","ip","routing","switching","wireless","dns","http/2","socket"} },
        { "Security", {"security","cryptography","penetration","malware","forensics","exploit","vulnerability","authentication"} },
        { "DevOps", {"docker","kubernetes","devops","ci/cd","terraform","ansible","observability","sre","linux administration"} },
        { "Math", {"mathematics","calculus","algebra","geometry","probability","equation","theorem"} },
        { "Science", {"physics","chemistry","biology","scientific","laboratory","astronomy"} },
        { "Business", {"business","management","leadership","strategy","startup","entrepreneur"} },
        { "Finance", {"finance","investing","accounting","trading","economics","portfolio","valuation"} },
        { "History", {"history","historical","civilization","empire","war","biography"} },
        { "Philosophy", {"philosophy","ethics","metaphysics","logic","epistemology"} },
        { "Design", {"design","typography","ux","ui","graphic design","visual design"} },
        { "Fiction", {"novel","fiction","chapter one","prologue","fantasy","mystery","thriller","romance","science fiction"} }
    };

    QString probe = book.title + '\n' + book.author + '\n' + book.publisher + '\n'
        + book.description + '\n' + book.subjects.join(' ') + '\n';
    probe += readBookProbeText(book).left(240000);
    const QString haystack = probe.toLower();

    QMap<int, QStringList> scored;
    for (const CategoryRule& rule : rules) {
        int hits = 0;
        for (const QString& keyword : rule.keywords) {
            if (haystack.contains(keyword.toLower()))
                ++hits;
        }
        if (hits > 0)
            scored[hits] << rule.label;
    }

    QStringList tags = book.tags;
    QList<int> scoreKeys = scored.keys();
    std::sort(scoreKeys.begin(), scoreKeys.end(), std::greater<int>());
    for (int score : scoreKeys) {
        QStringList labels = scored.value(score);
        std::sort(labels.begin(), labels.end());
        for (const QString& label : labels) {
            tags << label;
            if (uniqueCaseInsensitive(tags).size() >= 6)
                return uniqueCaseInsensitive(tags);
        }
    }

    if (tags.isEmpty()) {
        if (book.format.compare("PDF", Qt::CaseInsensitive) == 0)
            tags << "Reference";
        else if (book.format.compare("EPUB", Qt::CaseInsensitive) == 0)
            tags << "Reading";
    }
    return uniqueCaseInsensitive(tags);
}

QString extractIsbnFromBookContent(const Book& book)
{
    QStringList textCandidates;
    const QString directText = readBookProbeText(book);
    if (!directText.isEmpty())
        textCandidates << directText;

    if (book.format.compare("PDF", Qt::CaseInsensitive) == 0) {
        const QString pdftotext = extractTextWithPdftotext(book);
        if (!pdftotext.isEmpty())
            textCandidates << pdftotext;
    }

    for (const QString& candidateText : textCandidates) {
        const QString isbn = extractIsbnFromText(candidateText);
        if (!isbn.isEmpty())
            return isbn;
    }

    if (book.format.compare("PDF", Qt::CaseInsensitive) == 0) {
        const QString ocrText = ocrPdfWithExternalTools(book);
        const QString isbn = extractIsbnFromText(ocrText);
        if (!isbn.isEmpty())
            return isbn;
    }

    const QString archiveOcr = ocrImageArchiveWithTesseract(book);
    const QString archiveIsbn = extractIsbnFromText(archiveOcr);
    if (!archiveIsbn.isEmpty())
        return archiveIsbn;

    const QString coverOcr = ocrExistingCoverWithTesseract(book);
    const QString coverIsbn = extractIsbnFromText(coverOcr);
    if (!coverIsbn.isEmpty())
        return coverIsbn;

    return {};
}

int estimatePagesForBook(const Book& book)
{
    if (book.format.compare("PDF", Qt::CaseInsensitive) == 0) {
        const QString text = readBookProbeText(book);
        const int pages = text.count(QRegularExpression("/Type\\s*/Page\\b"));
        return pages > 0 ? pages : 0;
    }

    const QString text = readBookProbeText(book);
    if (text.isEmpty())
        return 0;
    const int words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
    if (words <= 0)
        return 0;
    return qMax(1, words / 300);
}

bool saveGeneratedCover(const Book& book, const QString& path)
{
    if (!AppConfig::isManagedCoverPath(path))
        return false;

    QImage image(900, 1350, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor("#10233b"));
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    QLinearGradient bg(0, 0, 900, 1350);
    bg.setColorAt(0.0, QColor("#17314d"));
    bg.setColorAt(0.55, QColor("#10233b"));
    bg.setColorAt(1.0, QColor("#244d75"));
    p.fillRect(image.rect(), bg);

    p.fillRect(QRect(64, 64, 22, 1220), QColor(255, 255, 255, 28));
    p.setPen(QColor("#f0d88e"));
    QFont titleFont = QApplication::font();
    titleFont.setPointSize(26);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(QRect(120, 150, 660, 680), Qt::TextWordWrap, book.displayTitle());

    QFont authorFont = QApplication::font();
    authorFont.setPointSize(16);
    p.setFont(authorFont);
    p.setPen(QColor("#e7dcc0"));
    p.drawText(QRect(120, 900, 660, 120), Qt::TextWordWrap, book.displayAuthor());

    p.setPen(QColor(255, 255, 255, 130));
    QFont metaFont = QApplication::font();
    metaFont.setPointSize(12);
    p.setFont(metaFont);
    p.drawText(QRect(120, 1140, 660, 120), Qt::TextWordWrap,
               QString("%1  %2").arg(book.format, book.year > 0 ? QString::number(book.year) : QString()));
    p.end();

    QDir().mkpath(QFileInfo(path).absolutePath());
    return image.save(path, "JPG", 92);
}

bool normalizeCoverFile(const QString& path)
{
    if (path.isEmpty() || !QFileInfo::exists(path) || !AppConfig::isManagedCoverPath(path))
        return false;

    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull())
        return false;

    const QSize target(600, 900);
    image = image.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    if (image.width() > target.width() || image.height() > target.height()) {
        const QRect crop((image.width() - target.width()) / 2,
                         (image.height() - target.height()) / 2,
                         target.width(),
                         target.height());
        image = image.copy(crop);
    }
    return image.save(path, QFileInfo(path).suffix().toUpper().toLatin1().constData(), 92);
}

QVector<Book> selectedBooks(const BookModel* model, const BookFilterModel* filter, QListView* view)
{
    QVector<Book> books;
    QSet<qint64> seen;
    const QModelIndexList selection = view->selectionModel() ? view->selectionModel()->selectedIndexes() : QModelIndexList{};
    for (const QModelIndex& proxyIdx : selection) {
        const QModelIndex srcIdx = filter->mapToSource(proxyIdx);
        if (!srcIdx.isValid())
            continue;
        const Book& book = model->bookAt(srcIdx.row());
        if (!seen.contains(book.id)) {
            seen.insert(book.id);
            books << book;
        }
    }
    return books;
}

class AdvancedSearchDialog : public QDialog
{
public:
    explicit AdvancedSearchDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Advanced Search");
        resize(460, 280);

        auto* layout = new QVBoxLayout(this);
        auto* form = new QFormLayout;

        m_text = new QLineEdit(this);
        m_text->setPlaceholderText("Use tokens like author:, title:, tag:, category:, isbn:, path:");
        form->addRow("Search text", m_text);

        m_author = new QLineEdit(this);
        form->addRow("Author", m_author);

        m_format = new QComboBox(this);
        m_format->addItem("Any format");
        m_format->addItems(Database::instance().allFormats());
        form->addRow("Format", m_format);

        m_category = new QComboBox(this);
        m_category->addItem("Any category");
        m_category->addItem("Book");
        m_category->addItem("Document");
        m_category->addItems(Database::instance().allTags());
        form->addRow("Category", m_category);

        auto* years = new QWidget(this);
        auto* yearsLayout = new QHBoxLayout(years);
        yearsLayout->setContentsMargins(0, 0, 0, 0);
        yearsLayout->setSpacing(8);
        m_yearFrom = new QSpinBox(years);
        m_yearFrom->setRange(0, 3000);
        m_yearFrom->setPrefix("From ");
        m_yearTo = new QSpinBox(years);
        m_yearTo->setRange(0, 3000);
        m_yearTo->setPrefix("To ");
        yearsLayout->addWidget(m_yearFrom);
        yearsLayout->addWidget(m_yearTo);
        form->addRow("Year range", years);

        m_favourites = new QCheckBox("Favourites only", this);
        m_noCover = new QCheckBox("Missing cover only", this);
        m_noMeta = new QCheckBox("Weak metadata only", this);
        form->addRow("Flags", m_favourites);
        form->addRow("", m_noCover);
        form->addRow("", m_noMeta);

        layout->addLayout(form);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttons->button(QDialogButtonBox::Reset), &QAbstractButton::clicked, this, [this]() {
            m_text->clear();
            m_author->clear();
            m_format->setCurrentIndex(0);
            m_category->setCurrentIndex(0);
            m_yearFrom->setValue(0);
            m_yearTo->setValue(0);
            m_favourites->setChecked(false);
            m_noCover->setChecked(false);
            m_noMeta->setChecked(false);
        });
        layout->addWidget(buttons);
    }

    QString text() const { return m_text->text().trimmed(); }
    QString author() const { return m_author->text().trimmed(); }
    QString format() const { return m_format->currentIndex() == 0 ? QString() : m_format->currentText(); }
    QString category() const { return m_category->currentIndex() == 0 ? QString() : m_category->currentText(); }
    int yearFrom() const { return m_yearFrom->value(); }
    int yearTo() const { return m_yearTo->value(); }
    bool favouritesOnly() const { return m_favourites->isChecked(); }
    bool noCoverOnly() const { return m_noCover->isChecked(); }
    bool noMetaOnly() const { return m_noMeta->isChecked(); }

private:
    QLineEdit* m_text = nullptr;
    QLineEdit* m_author = nullptr;
    QComboBox* m_format = nullptr;
    QComboBox* m_category = nullptr;
    QSpinBox* m_yearFrom = nullptr;
    QSpinBox* m_yearTo = nullptr;
    QCheckBox* m_favourites = nullptr;
    QCheckBox* m_noCover = nullptr;
    QCheckBox* m_noMeta = nullptr;
};

class DatabaseEditorDialog : public QDialog
{
public:
    explicit DatabaseEditorDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Database Editor");
        resize(1380, 820);
        setMinimumSize(980, 620);

        auto* layout = new QVBoxLayout(this);
        auto* topRow = new QHBoxLayout;
        auto* hint = new QLabel("Edit library records directly. Save writes the changed rows back to the SQLite database.", this);
        hint->setWordWrap(true);
        topRow->addWidget(hint, 1);
        m_search = new QLineEdit(this);
        m_search->setPlaceholderText("Filter rows by title, author, category, ISBN, tags, or path...");
        m_search->setMinimumWidth(320);
        topRow->addWidget(m_search);
        layout->addLayout(topRow);

        m_table = new QTableWidget(this);
        m_table->setColumnCount(11);
        m_table->setHorizontalHeaderLabels({"ID", "Category", "Title", "Author", "Publisher", "Year", "Format", "ISBN", "Language", "Tags", "File Path"});
        m_table->setAlternatingRowColors(true);
        m_table->setWordWrap(false);
        m_table->setTextElideMode(Qt::ElideMiddle);
        m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_table->setCornerButtonEnabled(true);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        m_table->horizontalHeader()->setSectionsMovable(true);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        m_table->verticalHeader()->setDefaultSectionSize(28);
        layout->addWidget(m_table, 1);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
        QPushButton* reloadBtn = buttons->addButton("Reload", QDialogButtonBox::ActionRole);
        QPushButton* deleteBtn = buttons->addButton("Delete Selected", QDialogButtonBox::DestructiveRole);
        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            saveEdits();
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(reloadBtn, &QPushButton::clicked, this, [this]() { loadRows(); });
        connect(deleteBtn, &QPushButton::clicked, this, [this]() { deleteSelected(); });
        connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) { applyFilter(text); });
        layout->addWidget(buttons);

        loadRows();
    }

private:
    void loadRows()
    {
        m_books = Database::instance().allBooks(SortField::Title, SortOrder::Asc);
        m_table->clearContents();
        m_table->setRowCount(m_books.size());
        for (int row = 0; row < m_books.size(); ++row) {
            const Book& b = m_books.at(row);
            const QStringList values = {
                QString::number(b.id),
                b.classificationTag(),
                b.title,
                b.author,
                b.publisher,
                b.year > 0 ? QString::number(b.year) : QString(),
                b.format,
                b.isbn,
                b.language,
                b.tags.join(", "),
                b.filePath
            };
            for (int col = 0; col < values.size(); ++col) {
                auto* item = new QTableWidgetItem(values.at(col));
                if (col == 0 || col == 1 || col == 6 || col == 10)
                    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                m_table->setItem(row, col, item);
            }
            if (b.isLikelyDocument()) {
                for (int col = 0; col < m_table->columnCount(); ++col)
                    m_table->item(row, col)->setBackground(QColor(103, 49, 38, 70));
            }
        }
        m_table->setColumnWidth(0, 72);
        m_table->setColumnWidth(1, 92);
        m_table->setColumnWidth(2, 280);
        m_table->setColumnWidth(3, 220);
        m_table->setColumnWidth(4, 180);
        m_table->setColumnWidth(5, 70);
        m_table->setColumnWidth(6, 84);
        m_table->setColumnWidth(7, 150);
        m_table->setColumnWidth(8, 100);
        m_table->setColumnWidth(9, 220);
        m_table->setColumnWidth(10, 420);
        applyFilter(m_search ? m_search->text() : QString());
    }

    void applyFilter(const QString& text)
    {
        const QString needle = text.trimmed();
        for (int row = 0; row < m_table->rowCount(); ++row) {
            bool visible = needle.isEmpty();
            if (!visible) {
                QString blob;
                for (int col = 0; col < m_table->columnCount(); ++col) {
                    QTableWidgetItem* item = m_table->item(row, col);
                    if (item)
                        blob += item->text() + '\n';
                }
                visible = blob.contains(needle, Qt::CaseInsensitive);
            }
            m_table->setRowHidden(row, !visible);
        }
    }

    void saveEdits()
    {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            const qint64 id = m_table->item(row, 0)->text().toLongLong();
            Book book = Database::instance().bookById(id);
            if (book.id == 0)
                continue;
            book.title = m_table->item(row, 2)->text().trimmed();
            book.author = m_table->item(row, 3)->text().trimmed();
            book.publisher = m_table->item(row, 4)->text().trimmed();
            book.year = m_table->item(row, 5)->text().trimmed().toInt();
            book.isbn = m_table->item(row, 7)->text().trimmed();
            book.language = m_table->item(row, 8)->text().trimmed();
            book.tags = m_table->item(row, 9)->text().split(",", Qt::SkipEmptyParts);
            for (QString& tag : book.tags)
                tag = tag.trimmed();
            Database::instance().updateBook(book);
        }
    }

    void deleteSelected()
    {
        const auto selected = m_table->selectionModel()->selectedRows();
        if (selected.isEmpty())
            return;
        if (QMessageBox::question(this, "Delete Records", QString("Delete %1 selected database rows?").arg(selected.size()),
                                  QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes) {
            return;
        }
        for (const QModelIndex& index : selected) {
            const qint64 id = m_table->item(index.row(), 0)->text().toLongLong();
            Database::instance().removeBook(id);
        }
        loadRows();
    }

    QVector<Book> m_books;
    QLineEdit* m_search = nullptr;
    QTableWidget* m_table = nullptr;
};

class ReferenceLookupDialog : public QDialog
{
public:
    explicit ReferenceLookupDialog(const Book& book, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Related References");
        resize(640, 360);

        const QVector<ReferenceTarget> links = trustedReferenceTargets(book);

        auto* layout = new QVBoxLayout(this);
        auto* title = new QLabel(QString("<b>%1</b><br>%2")
                                     .arg(book.displayTitle().toHtmlEscaped(),
                                          book.displayAuthor().toHtmlEscaped()), this);
        title->setWordWrap(true);
        layout->addWidget(title);

        auto* hint = new QLabel("Open trusted reference searches for the current book or document. Only validated whitelisted reference URLs are shown.", this);
        hint->setWordWrap(true);
        layout->addWidget(hint);

        auto* grid = new QGridLayout;
        int row = 0;
        for (const auto& link : links) {
            auto* name = new QLabel(link.label, this);
            auto* value = new QLabel(QString("<a href=\"%1\">%1</a>").arg(link.url.toString().toHtmlEscaped()), this);
            value->setOpenExternalLinks(true);
            value->setTextInteractionFlags(Qt::TextBrowserInteraction);
            auto* openButton = new QPushButton("Open", this);
            connect(openButton, &QPushButton::clicked, this, [url = link.url]() {
                QDesktopServices::openUrl(url);
            });
            grid->addWidget(name, row, 0);
            grid->addWidget(value, row, 1);
            grid->addWidget(openButton, row, 2);
            ++row;
        }
        if (links.isEmpty()) {
            auto* none = new QLabel("No safe reference URLs could be built for this item yet. Add a title, author, or ISBN first.", this);
            none->setWordWrap(true);
            layout->addWidget(none);
        } else {
            grid->setColumnStretch(1, 1);
            layout->addLayout(grid);
        }

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }
};

class ExternalToolsDialog : public QDialog
{
public:
    explicit ExternalToolsDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("PDF and OCR Tools");
        resize(700, 420);

        auto* layout = new QVBoxLayout(this);

        auto toolRow = [this, layout](const QString& title,
                                      const QString& description,
                                      const QString& executable,
                                      const QString& installLabel,
                                      std::function<void()> installAction,
                                      const QString& siteLabel,
                                      const QUrl& siteUrl) {
            auto* box = new QGroupBox(title, this);
            auto* boxLayout = new QVBoxLayout(box);
            auto* status = new QLabel(box);
            const QString found = QStandardPaths::findExecutable(executable);
            status->setText(found.isEmpty()
                                ? QString("Status: not detected in PATH (%1)").arg(executable)
                                : QString("Status: detected at %1").arg(QDir::toNativeSeparators(found)));
            status->setWordWrap(true);
            auto* desc = new QLabel(description, box);
            desc->setWordWrap(true);

            auto* buttons = new QHBoxLayout;
            auto* installBtn = new QPushButton(installLabel, box);
            connect(installBtn, &QPushButton::clicked, box, [installAction]() { installAction(); });
            auto* siteBtn = new QPushButton(siteLabel, box);
            connect(siteBtn, &QPushButton::clicked, box, [siteUrl]() { QDesktopServices::openUrl(siteUrl); });
            buttons->addWidget(installBtn);
            buttons->addWidget(siteBtn);
            buttons->addStretch();

            boxLayout->addWidget(status);
            boxLayout->addWidget(desc);
            boxLayout->addLayout(buttons);
            layout->addWidget(box);
        };

        auto startDetached = [this](const QString& program, const QStringList& arguments, const QString& failureText) {
            if (!QProcess::startDetached(program, arguments))
                QMessageBox::warning(this, "External Tools", failureText);
        };

        toolRow(
            "PDF text extraction (pdftotext / pdftoppm)",
            "Eagle uses Poppler tools for fast PDF text extraction and OCR page rendering. Poppler is not guaranteed to exist in winget, so Eagle opens the trusted release page for Windows builds.",
            "pdftotext",
            "Open Poppler Download",
            [this]() {
                QDesktopServices::openUrl(QUrl("https://github.com/oschwartz10612/poppler-windows/releases/"));
            },
            "Open Install Guide",
            QUrl("https://github.com/UB-Mannheim/zotero-ocr/wiki/Install-pdftoppm"));

        toolRow(
            "OCR engine (Tesseract)",
            "Eagle can call Tesseract for OCR when embedded metadata and direct text extraction are not enough. The install command opens in Windows Terminal or Command Prompt so you can approve it normally.",
            "tesseract",
            "Install with winget",
            [startDetached, this]() {
                if (QMessageBox::question(this,
                                          "Install Tesseract",
                                          "Launch a Windows terminal to install Tesseract with winget?",
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::Yes) != QMessageBox::Yes) {
                    return;
                }
                startDetached("cmd.exe",
                              QStringList() << "/c"
                                            << "start"
                                            << ""
                                            << "cmd.exe"
                                            << "/k"
                                            << "winget install --id tesseract-ocr.tesseract -e --accept-package-agreements --accept-source-agreements || winget install --id UB-Mannheim.TesseractOCR -e --accept-package-agreements --accept-source-agreements",
                              "Could not launch the winget install command.");
            },
            "Open Tesseract Project",
            QUrl("https://github.com/tesseract-ocr/tesseract"));

        auto* note = new QLabel("After installing tools, restart Eagle Library or run the same command in a new scan/repair action so the updated PATH is detected.", this);
        note->setWordWrap(true);
        layout->addWidget(note);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }
};

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(trl("app.windowTitle", "Eagle Library | Professional Workspace"));
    setMinimumSize(1100, 680);

    // Set application icon
    QIcon appIcon;
    appIcon.addFile(":/eagle_logo.png");
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
        qApp->setWindowIcon(appIcon);
    }

    // Centre on screen
    if (QScreen* sc = QGuiApplication::primaryScreen()) {
        QRect sg = sc->availableGeometry();
        resize(qMin(1400, sg.width() - 80), qMin(880, sg.height() - 80));
        move(sg.center() - QPoint(width()/2, height()/2));
    }

    // Backend
    m_model        = new BookModel(this);
    m_filterModel  = new BookFilterModel(this);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setSortRole(TitleRole);
    m_filterModel->sort(0);

    m_scanner         = new LibraryScanner(this);
    m_metaFetcher     = new MetadataFetcher(this);
    m_coverDownloader = new CoverDownloader(AppConfig::coversDir(), this);
    m_libraryWatcher  = new QFileSystemWatcher(this);
    m_libraryChangeTimer = new QTimer(this);
    m_libraryChangeTimer->setSingleShot(true);
    m_libraryChangeTimer->setInterval(1400);

    // UI
    setupMenuBar();
    setupToolBar();
    setupViews();
    setupSidebar();
    setupStatusBar();
    setupCommandPalette();
    applyStyles();
    loadSettings();

    // Apply saved theme
    {
        QSettings s;
        const QString savedTheme = s.value("ui/theme", "Blue Pro").toString();
        ThemeManager::instance().applyTheme(savedTheme);
        if (m_themeBlueAct && m_themeWhiteAct && m_themeMacAct) {
            if (savedTheme == "Pure White") m_themeWhiteAct->setChecked(true);
            else if (savedTheme == "macOS") m_themeMacAct->setChecked(true);
            else m_themeBlueAct->setChecked(true);
        }
    }

    // Initialize plugin manager
    PluginManager::instance().setMainWindow(this);
    PluginManager::instance().setPluginMenu(m_pluginMenu);
    PluginManager::instance().setToolBar(m_mainToolBar);
    PluginManager::instance().loadAll(AppConfig::pluginsDir());
    connect(&PluginManager::instance(), &PluginManager::statusMessage,
            this, [this](const QString& msg) {
                if (!m_isClosing && m_statusLabel)
                    m_statusLabel->setText(msg);
            });

    // Signals
    connect(m_scanner, &LibraryScanner::bookFound,    this, &MainWindow::onBookFound);
    connect(m_scanner, &LibraryScanner::progress,     this, &MainWindow::onScanProgress);
    connect(m_scanner, &LibraryScanner::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_metaFetcher,     &MetadataFetcher::metadataReady, this, &MainWindow::onMetadataReady);
    connect(m_metaFetcher,     &MetadataFetcher::coverUrlsReady, this, &MainWindow::onCoverUrlsReady);
    connect(m_metaFetcher,     &MetadataFetcher::fetchProgress, this, &MainWindow::onMetadataFetchProgress);
    connect(m_metaFetcher,     &MetadataFetcher::fetchError, this, &MainWindow::onMetadataFetchError);
    connect(m_coverDownloader, &CoverDownloader::coverReady,    this, &MainWindow::onCoverReady);
    connect(m_coverDownloader, &CoverDownloader::downloadProgress, this, &MainWindow::onCoverDownloadProgress);
    connect(m_coverDownloader, &CoverDownloader::coverFailed, this, &MainWindow::onCoverDownloadFailed);
    connect(m_libraryWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
        scheduleIncrementalScan(path);
    });
    connect(m_libraryChangeTimer, &QTimer::timeout, this, [this]() {
        if (m_isClosing || !m_statusLabel)
            return;
        if (m_watchFolders.isEmpty())
            return;
        if (m_scanner && m_scanner->isRunning()) {
            m_incrementalScanPending = true;
            return;
        }
        m_incrementalScanPending = false;
        m_statusLabel->setText("Folder changes detected. Running incremental scan...");
        m_scanner->startScan(m_watchFolders, m_scanThreads, true);
    });

    // Ctrl+F -> search
    auto* sch = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(sch, &QShortcut::activated, m_searchBox, qOverload<>(&QLineEdit::setFocus));

    // Ctrl+P -> command palette
    auto* cpSch = new QShortcut(QKeySequence("Ctrl+P"), this);
    connect(cpSch, &QShortcut::activated, this, &MainWindow::openCommandPalette);

    refreshLibrarySelector();
    refreshShelfOptions();
    reloadCurrentLibrary();
    refreshLibraryWatcher();
    updateStatusCount();
    refreshCategoryOptions();
    updateWorkspaceHeader();

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, [this]() {
        retranslateUi();
        if (m_languageActionGroup) {
            const QString currentCode = LanguageManager::instance().currentLanguage();
            for (QAction* action : m_languageActionGroup->actions()) {
                if (action)
                    action->setChecked(action->data().toString() == currentCode);
            }
        }
    });
}

MainWindow::~MainWindow() { saveSettings(); }

// ── Menu bar ─────────────────────────────────────────────────
void MainWindow::setupMenuBar()
{
    menuBar()->setNativeMenuBar(false);

    // ── File ──────────────────────────────────────────────────
    QMenu* fileMenu = menuBar()->addMenu(trl("menu.file", "&File"));

    QAction* settingsAct = new QAction(trl("action.settings", "Preferences..."), this);
    settingsAct->setShortcut(QKeySequence("Ctrl+,"));
    settingsAct->setStatusTip("Open application preferences");
    connect(settingsAct, &QAction::triggered, this, &MainWindow::openSettings);
    fileMenu->addAction(settingsAct);

    fileMenu->addSeparator();

    QAction* exportAct = new QAction("Export Library Snapshot...", this);
    exportAct->setStatusTip("Export library metadata to JSON");
    connect(exportAct, &QAction::triggered, this, &MainWindow::exportLibrarySnapshot);
    fileMenu->addAction(exportAct);

    QAction* importAct = new QAction("Import Library Snapshot...", this);
    importAct->setStatusTip("Import library metadata from JSON");
    connect(importAct, &QAction::triggered, this, &MainWindow::importLibrarySnapshot);
    fileMenu->addAction(importAct);

    fileMenu->addSeparator();

    QAction* stopTasksAct = new QAction("Stop All Tasks", this);
    stopTasksAct->setShortcut(QKeySequence("Ctrl+."));
    stopTasksAct->setStatusTip("Cancel all running background tasks");
    connect(stopTasksAct, &QAction::triggered, this, &MainWindow::stopAllTasks);
    fileMenu->addAction(stopTasksAct);

    fileMenu->addSeparator();

    QAction* quitAct = new QAction("Quit Eagle Library", this);
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quitAct);

    // ── Library ───────────────────────────────────────────────
    QMenu* libMenu = menuBar()->addMenu("&Library");

    QAction* scanAct = new QAction("Scan Folders", this);
    scanAct->setShortcut(QKeySequence("F5"));
    scanAct->setStatusTip("Scan watched folders for new files");
    connect(scanAct, &QAction::triggered, this, &MainWindow::onScanStart);
    libMenu->addAction(scanAct);

    QAction* refreshAct2 = new QAction("Refresh View", this);
    refreshAct2->setShortcut(QKeySequence("F5"));
    connect(refreshAct2, &QAction::triggered, this, &MainWindow::refreshLibrary);
    // (scan already has F5, so we just connect refresh without shortcut duplicate)
    refreshAct2->setShortcut(QKeySequence());

    libMenu->addSeparator();

    // Search submenu
    QMenu* searchMenu = libMenu->addMenu("Search");
    QAction* advancedSearchAct = new QAction("Advanced Search...", this);
    advancedSearchAct->setShortcut(QKeySequence("Ctrl+Shift+F"));
    advancedSearchAct->setStatusTip("Open powerful multi-field search dialog");
    connect(advancedSearchAct, &QAction::triggered, this, &MainWindow::openAdvancedSearch);
    searchMenu->addAction(advancedSearchAct);

    QAction* saveSearchAct = new QAction("Save Current Search...", this);
    saveSearchAct->setStatusTip("Save the current search query for quick access");
    connect(saveSearchAct, &QAction::triggered, this, &MainWindow::saveCurrentSearch);
    searchMenu->addAction(saveSearchAct);

    QAction* openSavedSearchAct = new QAction("Open Saved Search...", this);
    openSavedSearchAct->setStatusTip("Open and apply a previously saved search");
    connect(openSavedSearchAct, &QAction::triggered, this, [this]() {
        const QVector<SavedSearch> searches = Database::instance().savedSearches();
        if (searches.isEmpty()) {
            QMessageBox::information(this, "Saved Searches", "No saved searches are available yet.");
            return;
        }

        QStringList items;
        for (const SavedSearch& search : searches)
            items << QString("%1  [%2]").arg(search.name, search.query);

        bool ok = false;
        const QString selected = QInputDialog::getItem(this, "Saved Searches", "Choose a saved search:", items, 0, false, &ok);
        if (!ok || selected.isEmpty())
            return;

        const int index = items.indexOf(selected);
        if (index >= 0)
            loadSavedSearch(searches.at(index).id);
    });
    searchMenu->addAction(openSavedSearchAct);

    libMenu->addSeparator();

    // Metadata submenu
    QMenu* metaMenu = libMenu->addMenu("Metadata");

    QAction* fetchAct = new QAction("Fetch All Metadata", this);
    fetchAct->setStatusTip("Download metadata for all books from Google Books and OpenLibrary");
    connect(fetchAct, &QAction::triggered, this, &MainWindow::fetchAllMetadata);
    metaMenu->addAction(fetchAct);

    QAction* metaManagerAct = new QAction("Metadata Manager...", this);
    metaManagerAct->setStatusTip("Choose metadata scope, overwrite mode, and source providers");
    connect(metaManagerAct, &QAction::triggered, this, &MainWindow::openMetadataManager);
    metaMenu->addAction(metaManagerAct);

    QAction* enrichIncompleteAct = new QAction("Enrich Incomplete Books", this);
    enrichIncompleteAct->setStatusTip("Fetch metadata only for books with incomplete information");
    connect(enrichIncompleteAct, &QAction::triggered, this, &MainWindow::enrichIncompleteBooksAction);
    metaMenu->addAction(enrichIncompleteAct);

    QAction* isbnAct = new QAction("Extract ISBNs \u2192 Fetch Metadata", this);
    isbnAct->setStatusTip("Extract ISBN numbers from file content and fetch metadata");
    connect(isbnAct, &QAction::triggered, this, &MainWindow::extractMissingIsbns);
    metaMenu->addAction(isbnAct);

    metaMenu->addSeparator();

    QAction* genCoversAct = new QAction("Generate Missing Covers", this);
    connect(genCoversAct, &QAction::triggered, this, &MainWindow::generateMissingCovers);
    metaMenu->addAction(genCoversAct);

    QAction* normCoversAct = new QAction("Normalize Covers", this);
    connect(normCoversAct, &QAction::triggered, this, &MainWindow::normalizeCovers);
    metaMenu->addAction(normCoversAct);

    QAction* pagesAct = new QAction("Count Pages", this);
    connect(pagesAct, &QAction::triggered, this, &MainWindow::countPagesForLibrary);
    metaMenu->addAction(pagesAct);

    // Tools submenu
    QMenu* toolsMenu = libMenu->addMenu("Tools");

    QAction* qualityAct = new QAction("Quality Check", this);
    qualityAct->setStatusTip("Detect encoding issues and suspicious metadata");
    connect(qualityAct, &QAction::triggered, this, &MainWindow::runQualityCheck);
    toolsMenu->addAction(qualityAct);

    QAction* dupesAct = new QAction("Find Duplicates", this);
    dupesAct->setStatusTip("Detect duplicate files by hash");
    connect(dupesAct, &QAction::triggered, this, &MainWindow::findDuplicates);
    toolsMenu->addAction(dupesAct);

    QAction* categorizeAct = new QAction("Smart Categorize", this);
    categorizeAct->setStatusTip("Auto-classify books vs documents");
    connect(categorizeAct, &QAction::triggered, this, &MainWindow::smartCategorizeLibrary);
    toolsMenu->addAction(categorizeAct);

    toolsMenu->addSeparator();

    QAction* renameMenuAct = new QAction("Smart Rename All...", this);
    renameMenuAct->setShortcut(QKeySequence("Ctrl+R"));
    connect(renameMenuAct, &QAction::triggered, this, &MainWindow::onSmartRenameAll);
    toolsMenu->addAction(renameMenuAct);

    QAction* renameSelectedAct2 = new QAction("Smart Rename Selected...", this);
    renameSelectedAct2->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(renameSelectedAct2, &QAction::triggered, this, &MainWindow::onSmartRenameSelected);
    toolsMenu->addAction(renameSelectedAct2);

    // Collections
    libMenu->addSeparator();
    QAction* collectionsAct = new QAction("Manage Collections...", this);
    collectionsAct->setStatusTip("Create and manage virtual book collections");
    connect(collectionsAct, &QAction::triggered, this, &MainWindow::manageCollections);
    libMenu->addAction(collectionsAct);

    // Web Research
    libMenu->addSeparator();
    QMenu* researchMenu = libMenu->addMenu("Research");

    QMenu* referenceMenu = researchMenu->addMenu("Reference Lookup");
    QAction* referenceDialogAct = new QAction("Current Selection...", this);
    connect(referenceDialogAct, &QAction::triggered, this, [this]() {
        QListView* view = currentView();
        const QModelIndexList selected = view && view->selectionModel() ? view->selectionModel()->selectedIndexes() : QModelIndexList{};
        if (selected.isEmpty()) {
            m_statusLabel->setText("Select a book first.");
            return;
        }
        const QModelIndex srcIdx = m_filterModel->mapToSource(selected.first());
        if (srcIdx.isValid())
            showReferenceLookup(m_model->bookAt(srcIdx.row()));
    });
    referenceMenu->addAction(referenceDialogAct);

    QAction* googleAct = new QAction("Search on Google...", this);
    googleAct->setShortcut(QKeySequence("Ctrl+Shift+G"));
    googleAct->setStatusTip("Search selected book on Google");
    connect(googleAct, &QAction::triggered, this, &MainWindow::searchSelectedOnGoogle);
    researchMenu->addAction(googleAct);

    libMenu->addSeparator();

    QAction* cleanAct = new QAction("Remove Missing Files", this);
    cleanAct->setStatusTip("Clean up records for files that no longer exist");
    connect(cleanAct, &QAction::triggered, this, &MainWindow::cleanMissingFiles);
    libMenu->addAction(cleanAct);

    QAction* removeAct = new QAction("Remove Selected from Library", this);
    removeAct->setShortcut(QKeySequence::Delete);
    connect(removeAct, &QAction::triggered, this, &MainWindow::removeSelectedBook);
    libMenu->addAction(removeAct);

    // ── View ──────────────────────────────────────────────────
    QMenu* viewMenu = menuBar()->addMenu("&View");

    QAction* gridAct = new QAction("Grid View", this);
    gridAct->setShortcut(QKeySequence("Ctrl+G"));
    gridAct->setCheckable(true);
    gridAct->setChecked(true);
    connect(gridAct, &QAction::triggered, this, &MainWindow::setGridView);
    viewMenu->addAction(gridAct);

    QAction* listAct = new QAction("List View", this);
    listAct->setShortcut(QKeySequence("Ctrl+L"));
    listAct->setCheckable(true);
    connect(listAct, &QAction::triggered, this, &MainWindow::setListView);
    viewMenu->addAction(listAct);

    viewMenu->addSeparator();

    QAction* cmdPaletteAct = new QAction("Command Palette", this);
    cmdPaletteAct->setShortcut(QKeySequence("Ctrl+P"));
    cmdPaletteAct->setStatusTip("Open VS Code-style command launcher");
    connect(cmdPaletteAct, &QAction::triggered, this, &MainWindow::openCommandPalette);
    viewMenu->addAction(cmdPaletteAct);

    viewMenu->addSeparator();

    // Theme submenu
    QMenu* themeMenu = viewMenu->addMenu("Appearance");

    auto* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    m_themeBlueAct = new QAction("Blue Pro (Dark)", themeGroup);
    m_themeBlueAct->setCheckable(true);
    m_themeBlueAct->setChecked(true);
    connect(m_themeBlueAct, &QAction::triggered, this, [this]() { switchTheme("Blue Pro"); });
    themeMenu->addAction(m_themeBlueAct);

    m_themeWhiteAct = new QAction("Pure White (Light)", themeGroup);
    m_themeWhiteAct->setCheckable(true);
    connect(m_themeWhiteAct, &QAction::triggered, this, [this]() { switchTheme("Pure White"); });
    themeMenu->addAction(m_themeWhiteAct);

    m_themeMacAct = new QAction("macOS Style", themeGroup);
    m_themeMacAct->setCheckable(true);
    connect(m_themeMacAct, &QAction::triggered, this, [this]() { switchTheme("macOS"); });
    themeMenu->addAction(m_themeMacAct);

    viewMenu->addSeparator();

    QMenu* languageMenu = viewMenu->addMenu("Language");
    m_languageActionGroup = new QActionGroup(this);
    m_languageActionGroup->setExclusive(true);
    const QVector<LanguageInfo> languages = LanguageManager::instance().availableLanguages();
    for (const LanguageInfo& info : languages) {
        QAction* action = new QAction(QString("%1 (%2)").arg(info.nativeName, info.name), m_languageActionGroup);
        action->setData(info.code);
        action->setCheckable(true);
        action->setChecked(info.code == LanguageManager::instance().currentLanguage());
        connect(action, &QAction::triggered, this, [this, code = info.code]() {
            if (LanguageManager::instance().applyLanguage(code)) {
                QSettings s("Eagle Software", "Eagle Library");
                s.setValue("ui/language", code);
                if (m_statusLabel)
                    m_statusLabel->setText(QString("Language: %1").arg(code));
            }
        });
        languageMenu->addAction(action);
    }

    viewMenu->addSeparator();

    QAction* refreshAct = new QAction("Refresh", this);
    refreshAct->setShortcut(QKeySequence("F5"));
    connect(refreshAct, &QAction::triggered, this, &MainWindow::refreshLibrary);
    viewMenu->addAction(refreshAct);

    // ── Plugins ───────────────────────────────────────────────
    m_pluginMenu = menuBar()->addMenu("&Plugins");

    QAction* pluginManagerAct = new QAction("Manage Plugins...", this);
    pluginManagerAct->setStatusTip("View, enable, and disable installed plugins");
    connect(pluginManagerAct, &QAction::triggered, this, &MainWindow::openPluginManager);
    m_pluginMenu->addAction(pluginManagerAct);

    QAction* pluginFolderAct = new QAction("Open Plugins Folder", this);
    connect(pluginFolderAct, &QAction::triggered, this, [this]() {
        const QString dir = AppConfig::pluginsDir();
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
    m_pluginMenu->addAction(pluginFolderAct);

    m_pluginMenu->addSeparator();
    // Plugin-contributed items will appear below this separator

    // ── Database ──────────────────────────────────────────────
    QMenu* dbMenu = menuBar()->addMenu("&Database");

    QAction* dbSummaryAct = new QAction("Database Summary", this);
    connect(dbSummaryAct, &QAction::triggered, this, &MainWindow::consultDatabaseSummary);
    dbMenu->addAction(dbSummaryAct);

    dbMenu->addSeparator();

    QAction* dbDiagnoseAct = new QAction("Diagnose Text Issues", this);
    connect(dbDiagnoseAct, &QAction::triggered, this, &MainWindow::diagnoseDatabaseText);
    dbMenu->addAction(dbDiagnoseAct);

    QAction* dbRepairAct = new QAction("Repair Text Issues...", this);
    connect(dbRepairAct, &QAction::triggered, this, &MainWindow::repairDatabaseText);
    dbMenu->addAction(dbRepairAct);

    QAction* rebuildSearchAct = new QAction("Rebuild Search Index", this);
    connect(rebuildSearchAct, &QAction::triggered, this, [this]() {
        showTaskProgress("Search Index", "Rebuilding the metadata search index...", 0, 0);
        Database::instance().rebuildSearchIndex();
        hideTaskProgress("Search index rebuilt.");
    });
    dbMenu->addAction(rebuildSearchAct);

    dbMenu->addSeparator();

    QAction* dbFolderAct = new QAction("Open Database Folder", this);
    connect(dbFolderAct, &QAction::triggered, this, &MainWindow::openDatabaseFolder);
    dbMenu->addAction(dbFolderAct);

    QAction* dbEditorAct = new QAction("Database Editor...", this);
    connect(dbEditorAct, &QAction::triggered, this, &MainWindow::openDatabaseEditor);
    dbMenu->addAction(dbEditorAct);

    QAction* remapPathsAct = new QAction("Remap Drive / Folder Paths...", this);
    connect(remapPathsAct, &QAction::triggered, this, &MainWindow::remapLibraryPaths);
    dbMenu->addAction(remapPathsAct);

    dbMenu->addSeparator();

    QAction* optimizeAct = new QAction("Optimize", this);
    connect(optimizeAct, &QAction::triggered, this, [this]() {
        showTaskProgress("Optimizing Database", "Running SQLite optimize...", 0, 0);
        Database::instance().optimize();
        hideTaskProgress("Optimization complete.");
    });
    dbMenu->addAction(optimizeAct);

    QAction* vacuumAct = new QAction("Vacuum", this);
    connect(vacuumAct, &QAction::triggered, this, [this]() {
        Database::instance().vacuum();
        m_statusLabel->setText("Vacuum complete.");
    });
    dbMenu->addAction(vacuumAct);

    QAction* reindexAct = new QAction("Reindex", this);
    connect(reindexAct, &QAction::triggered, this, [this]() {
        Database::instance().reindex();
        m_statusLabel->setText("Reindex complete.");
    });
    dbMenu->addAction(reindexAct);

    // ── Help ──────────────────────────────────────────────────
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    QAction* aboutAct = new QAction("About Eagle Library", this);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAct);

    helpMenu->addSeparator();

    QAction* guideAct = new QAction("User Guide", this);
    guideAct->setShortcut(QKeySequence::HelpContents);
    connect(guideAct, &QAction::triggered, this, [this]() {
        const QDir appDir(QCoreApplication::applicationDirPath());
        const QStringList candidates = {
            appDir.absoluteFilePath("help/EagleLibrary.chm"),
            appDir.absoluteFilePath("help/index.html"),
            appDir.absoluteFilePath("../../docs/help/index.html"),
            appDir.absoluteFilePath("../../../docs/help/index.html"),
            QDir::current().absoluteFilePath("docs/help/index.html")
        };
        for (const QString& candidate : candidates) {
            if (QFileInfo::exists(candidate)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(candidate));
                return;
            }
        }
        QMessageBox::information(this, "User Guide", "Help file not found. See the documentation folder.");
    });
    helpMenu->addAction(guideAct);

    QAction* toolsAct = new QAction("Install PDF/OCR Tools...", this);
    connect(toolsAct, &QAction::triggered, this, &MainWindow::openExternalToolsDialog);
    helpMenu->addAction(toolsAct);

    helpMenu->addSeparator();

    QAction* webAct = new QAction("Eagle Software Website", this);
    connect(webAct, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://eaglesoftware.biz/"));
    });
    helpMenu->addAction(webAct);
}

// ── Toolbar ──────────────────────────────────────────────────
void MainWindow::setupToolBar()
{
    m_mainToolBar = addToolBar("Main Toolbar");
    QToolBar* tb = m_mainToolBar;
    tb->setObjectName("mainToolBar");
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->setIconSize(QSize(16, 16));

    // Brand widget
    auto* brand = new QWidget(tb);
    brand->setObjectName("toolbarBrand");
    auto* brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(6, 0, 14, 0);
    brandLayout->setSpacing(7);
    auto* brandIcon = new QLabel(brand);
    brandIcon->setPixmap(QIcon(":/eagle_mark.svg").pixmap(24, 24));
    auto* brandText = new QLabel("Eagle Library", brand);
    brandText->setObjectName("toolbarBrandText");
    brandLayout->addWidget(brandIcon);
    brandLayout->addWidget(brandText);
    tb->addWidget(brand);
    tb->addSeparator();

    // Library selector
    m_libraryCombo = new QComboBox(tb);
    m_libraryCombo->setMinimumWidth(160);
    m_libraryCombo->setMaximumWidth(200);
    m_libraryCombo->setToolTip("Active library profile");
    connect(m_libraryCombo, &QComboBox::currentTextChanged, this, &MainWindow::switchLibrary);
    tb->addWidget(m_libraryCombo);
    tb->addSeparator();

    // Scan
    QAction* scanAct = new QAction("Scan", this);
    scanAct->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    scanAct->setToolTip("Scan folders for new files  (F5)");
    connect(scanAct, &QAction::triggered, this, &MainWindow::onScanStart);
    tb->addAction(scanAct);

    tb->addSeparator();

    // View toggle
    m_gridAction = new QAction("Grid", this);
    m_gridAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    m_gridAction->setCheckable(true);
    m_gridAction->setChecked(true);
    m_gridAction->setToolTip("Cover grid view  (Ctrl+G)");
    connect(m_gridAction, &QAction::triggered, this, &MainWindow::setGridView);
    tb->addAction(m_gridAction);

    m_listAction = new QAction("List", this);
    m_listAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    m_listAction->setCheckable(true);
    m_listAction->setToolTip("Compact list view  (Ctrl+L)");
    connect(m_listAction, &QAction::triggered, this, &MainWindow::setListView);
    tb->addAction(m_listAction);

    tb->addSeparator();

    // Favourites
    m_favAction = new QAction(this);
    m_favAction->setText("\u2665  Favourites");
    m_favAction->setCheckable(true);
    m_favAction->setToolTip("Show only favourites");
    connect(m_favAction, &QAction::triggered, this, &MainWindow::showFavourites);
    tb->addAction(m_favAction);

    tb->addSeparator();

    // Search box (prominent)
    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText("Search title, author, ISBN, tags...");
    m_searchBox->setMinimumWidth(240);
    m_searchBox->setMaximumWidth(360);
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setToolTip("Search across title, author, ISBN, tags, description  (Ctrl+F)");
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::searchChanged);
    tb->addWidget(m_searchBox);

    tb->addSeparator();

    // Shelf selector
    m_shelfCombo = new QComboBox(tb);
    m_shelfCombo->setMinimumWidth(150);
    m_shelfCombo->setMaximumWidth(180);
    m_shelfCombo->setToolTip("Virtual shelf / filter view");
    connect(m_shelfCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0)
            applyShelf(m_shelfCombo->itemData(index).toString());
    });
    tb->addWidget(m_shelfCombo);

    tb->addSeparator();

    // Format filter
    m_formatCombo = new QComboBox;
    m_formatCombo->setMinimumWidth(80);
    m_formatCombo->setMaximumWidth(100);
    m_formatCombo->setToolTip("Filter by format");
    m_formatCombo->addItem("All Formats");
    for (const char* fmt : {"PDF","EPUB","MOBI","AZW","AZW3","DjVu","CBZ","CBR","FB2","TXT","DOCX"})
        m_formatCombo->addItem(fmt);
    connect(m_formatCombo, &QComboBox::currentTextChanged, this, &MainWindow::filterByFormat);
    tb->addWidget(m_formatCombo);

    // Category filter
    m_categoryCombo = new QComboBox;
    m_categoryCombo->setMinimumWidth(130);
    m_categoryCombo->setMaximumWidth(160);
    m_categoryCombo->setToolTip("Filter by category or tag");
    connect(m_categoryCombo, &QComboBox::currentTextChanged, this, &MainWindow::filterByCategory);
    tb->addWidget(m_categoryCombo);

    tb->addSeparator();

    // Sort
    m_sortCombo = new QComboBox;
    m_sortCombo->setMinimumWidth(100);
    m_sortCombo->setMaximumWidth(130);
    m_sortCombo->setToolTip("Sort order");
    m_sortCombo->addItem("Title",      (int)SortField::Title);
    m_sortCombo->addItem("Author",     (int)SortField::Author);
    m_sortCombo->addItem("Year",       (int)SortField::Year);
    m_sortCombo->addItem("Format",     (int)SortField::Format);
    m_sortCombo->addItem("Date Added", (int)SortField::DateAdded);
    m_sortCombo->addItem("Rating",     (int)SortField::Rating);
    m_sortCombo->addItem("File Size",  (int)SortField::FileSize);
    m_sortCombo->addItem("Series",     (int)SortField::Series);
    connect(m_sortCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::sortChanged);
    tb->addWidget(m_sortCombo);

    m_sortAscAct = new QAction("\u2191 Asc", this);
    m_sortAscAct->setCheckable(true);
    m_sortAscAct->setChecked(true);
    m_sortAscAct->setToolTip("Toggle sort direction");
    connect(m_sortAscAct, &QAction::triggered, this, [this](bool checked){
        m_sortOrder = checked ? SortOrder::Asc : SortOrder::Desc;
        m_sortAscAct->setText(checked ? "\u2191 Asc" : "\u2193 Desc");
        sortChanged(m_sortCombo->currentIndex());
    });
    tb->addAction(m_sortAscAct);

    // Spacer
    auto* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);

    // Command Palette button
    QAction* cpAct = new QAction("\u2318  Commands", this);
    cpAct->setToolTip("Open command palette  (Ctrl+P)");
    connect(cpAct, &QAction::triggered, this, &MainWindow::openCommandPalette);
    tb->addAction(cpAct);

    tb->addSeparator();

    // Settings
    QAction* settAct = new QAction(this);
    settAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
    settAct->setToolTip("Preferences  (Ctrl+,)");
    connect(settAct, &QAction::triggered, this, &MainWindow::openSettings);
    tb->addAction(settAct);
}

// ── Sidebar ──────────────────────────────────────────────────
void MainWindow::setupSidebar()
{
    m_sidebarDock = new QDockWidget(trl("sidebar.browse", "Browse"), this);
    auto* dock = m_sidebarDock;
    dock->setObjectName("sidebarDock");
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setMinimumWidth(170);
    dock->setMaximumWidth(240);

    m_sidebarContent = new QWidget;
    auto* sideWidget = m_sidebarContent;
    sideWidget->setObjectName("sidebarContainer");
    sideWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_sidebarLayout = new QVBoxLayout(sideWidget);
    auto* sideLayout = m_sidebarLayout;
    sideLayout->setContentsMargins(4, 8, 4, 8);
    sideLayout->setSpacing(4);
    sideLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    auto makeHeader = [&](const QString& title) {
        auto* lbl = new QLabel(title);
        lbl->setObjectName("sectionLabel");
        sideLayout->addWidget(lbl);
    };

    auto makeBtn = [&](const QString& text, std::function<void()> fn, const QString& shelfId = QString()) {
        auto* btn = new QPushButton(text);
        btn->setObjectName("sidebarButton");
        btn->setFlat(true);
        btn->setCheckable(!shelfId.isEmpty());
        btn->setProperty("shelfId", shelfId);
        connect(btn, &QPushButton::clicked, fn);
        sideLayout->addWidget(btn);
    };

    makeHeader(trl("sidebar.library", "LIBRARY"));
    makeBtn(trl("sidebar.allBooks", "All Books"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("all")));
        else applyShelf(QStringLiteral("all"));
    }, QStringLiteral("all"));
    makeBtn(trl("sidebar.favourites", "Favourites"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("favourites")));
        else applyShelf(QStringLiteral("favourites"));
    }, QStringLiteral("favourites"));
    makeBtn(trl("sidebar.recentlyAdded", "Recently Added"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("recent")));
        else applyShelf(QStringLiteral("recent"));
    }, QStringLiteral("recent"));
    makeBtn(trl("sidebar.mostOpened", "Most Opened"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("opened")));
        else applyShelf(QStringLiteral("opened"));
    }, QStringLiteral("opened"));

    sideLayout->addSpacing(8);
    makeHeader(trl("sidebar.smartViews", "SMART VIEWS"));
    makeBtn(trl("sidebar.noCoverArt", "No Cover Art"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("no_cover")));
        else applyShelf(QStringLiteral("no_cover"));
    }, QStringLiteral("no_cover"));
    makeBtn(trl("sidebar.missingMetadata", "Missing Metadata"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("missing_metadata")));
        else applyShelf(QStringLiteral("missing_metadata"));
    }, QStringLiteral("missing_metadata"));
    makeBtn(trl("sidebar.booksOnly", "Books Only"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("books")));
        else applyShelf(QStringLiteral("books"));
    }, QStringLiteral("books"));
    makeBtn(trl("sidebar.documentsOnly", "Documents Only"), [this]() {
        if (m_shelfCombo) m_shelfCombo->setCurrentIndex(m_shelfCombo->findData(QStringLiteral("documents")));
        else applyShelf(QStringLiteral("documents"));
    }, QStringLiteral("documents"));

    sideLayout->addSpacing(8);
    m_smartCategorySection = new QWidget(sideWidget);
    auto* smartSectionLayout = new QVBoxLayout(m_smartCategorySection);
    smartSectionLayout->setContentsMargins(0, 0, 0, 0);
    smartSectionLayout->setSpacing(4);
    auto* smartHeader = new QLabel(trl("sidebar.smartCategories", "SMART CATEGORIES"));
    smartHeader->setObjectName("sectionLabel");
    smartSectionLayout->addWidget(smartHeader);
    m_smartCategoryButtonsLayout = new QVBoxLayout;
    m_smartCategoryButtonsLayout->setContentsMargins(0, 0, 0, 0);
    m_smartCategoryButtonsLayout->setSpacing(2);
    smartSectionLayout->addLayout(m_smartCategoryButtonsLayout);
    sideLayout->addWidget(m_smartCategorySection);

    sideLayout->addSpacing(8);
    m_savedSearchSection = new QWidget(sideWidget);
    auto* savedSearchLayout = new QVBoxLayout(m_savedSearchSection);
    savedSearchLayout->setContentsMargins(0, 0, 0, 0);
    savedSearchLayout->setSpacing(4);
    auto* savedSearchHeader = new QLabel(trl("sidebar.savedSearches", "SAVED SEARCHES"));
    savedSearchHeader->setObjectName("sectionLabel");
    savedSearchLayout->addWidget(savedSearchHeader);
    m_savedSearchButtonsLayout = new QVBoxLayout;
    m_savedSearchButtonsLayout->setContentsMargins(0, 0, 0, 0);
    m_savedSearchButtonsLayout->setSpacing(2);
    savedSearchLayout->addLayout(m_savedSearchButtonsLayout);
    sideLayout->addWidget(m_savedSearchSection);

    sideLayout->addSpacing(8);
    makeHeader(trl("sidebar.format", "FORMAT"));
    for (const char* fmt : {"PDF","EPUB","MOBI","AZW","CBZ","DjVu","TXT"}) {
        QString f = fmt;
        makeBtn(f, [this, f]() {
            m_filterModel->setFilterFormat(f);
            m_formatCombo->setCurrentText(f);
        });
    }

    sideLayout->addStretch();

    m_sidebarStatsLabel = new QLabel;
    m_sidebarStatsLabel->setObjectName("statsLabel");
    m_sidebarStatsLabel->setAlignment(Qt::AlignCenter);
    m_sidebarStatsLabel->setWordWrap(true);
    connect(m_model, &QAbstractItemModel::modelReset, this, [this]() {
        if (m_sidebarStatsLabel)
            m_sidebarStatsLabel->setText(trl("status.itemsInLibrary", "%1 items in library").arg(m_model->rowCount()));
    });
    sideLayout->addWidget(m_sidebarStatsLabel);

    m_sidebarScrollArea = new QScrollArea(dock);
    m_sidebarScrollArea->setWidgetResizable(true);
    m_sidebarScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sidebarScrollArea->setFrameShape(QFrame::NoFrame);
    m_sidebarScrollArea->setWidget(sideWidget);

    dock->setWidget(m_sidebarScrollArea);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    rebuildSmartCategorySidebar();
    rebuildSavedSearchSidebar();
}

// ── Central views ────────────────────────────────────────────
void MainWindow::setupViews()
{
    auto* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    m_workspaceHeader = new QWidget(central);
    m_workspaceHeader->setObjectName("workspaceHeader");
    auto* headerLayout = new QVBoxLayout(m_workspaceHeader);
    headerLayout->setContentsMargins(20, 18, 20, 16);
    headerLayout->setSpacing(10);

    auto* headerTopLayout = new QHBoxLayout;
    headerTopLayout->setSpacing(14);

    auto* headingLayout = new QVBoxLayout;
    headingLayout->setSpacing(4);
    m_workspaceTitleLabel = new QLabel(trl("workspace.titleDefault", "Library Workspace"), m_workspaceHeader);
    m_workspaceTitleLabel->setObjectName("workspaceTitle");
    m_workspaceMetaLabel = new QLabel(m_workspaceHeader);
    m_workspaceMetaLabel->setObjectName("workspaceMeta");
    m_workspaceMetaLabel->setWordWrap(true);
    headingLayout->addWidget(m_workspaceTitleLabel);
    headingLayout->addWidget(m_workspaceMetaLabel);
    headerTopLayout->addLayout(headingLayout, 1);

    m_workspaceHintLabel = new QLabel(m_workspaceHeader);
    m_workspaceHintLabel->setObjectName("workspaceHint");
    m_workspaceHintLabel->setWordWrap(true);
    m_workspaceHintLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_workspaceHintLabel->hide();
    headerTopLayout->addWidget(m_workspaceHintLabel, 1);
    headerLayout->addLayout(headerTopLayout);

    auto* chipsLayout = new QHBoxLayout;
    chipsLayout->setSpacing(8);
    m_workspaceLibraryChip = new QLabel(m_workspaceHeader);
    m_workspaceLibraryChip->setObjectName("workspaceChip");
    m_workspaceViewChip = new QLabel(m_workspaceHeader);
    m_workspaceViewChip->setObjectName("workspaceChip");
    m_workspaceActionChip = new QLabel(m_workspaceHeader);
    m_workspaceActionChip->setObjectName("workspaceChip");
    chipsLayout->addWidget(m_workspaceLibraryChip);
    chipsLayout->addWidget(m_workspaceViewChip);
    chipsLayout->addWidget(m_workspaceActionChip);
    chipsLayout->addStretch();
    headerLayout->addLayout(chipsLayout);
    centralLayout->addWidget(m_workspaceHeader);

    m_stack = new QStackedWidget(central);
    centralLayout->addWidget(m_stack, 1);
    setCentralWidget(central);

    // Grid view
    m_gridView = new QListView;
    m_gridView->setObjectName("gridView");
    m_gridView->setModel(m_filterModel);
    m_gridView->setViewMode(QListView::IconMode);
    m_gridView->setResizeMode(QListView::Adjust);
    m_gridView->setMovement(QListView::Static);
    m_gridView->setSpacing(14);
    m_gridView->setUniformItemSizes(true);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setMouseTracking(true);
    m_gridView->setLayoutMode(QListView::Batched);
    m_gridView->setBatchSize(48);
    m_gridView->setGridSize(QSize(180, 264));
    m_gridView->setWrapping(true);
    m_gridView->setSpacing(12);
    m_gridView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_gridDelegate = new BookDelegate(m_gridView);
    m_gridDelegate->setGridMode(true);
    m_gridView->setItemDelegate(m_gridDelegate);

    connect(m_gridView, &QListView::doubleClicked,
            this, [this](const QModelIndex& idx) { openBookFile(idx); });

    // List view
    m_listView = new QListView;
    m_listView->setObjectName("listView");
    m_listView->setModel(m_filterModel);
    m_listView->setViewMode(QListView::ListMode);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setMouseTracking(true);
    m_listView->setUniformItemSizes(true);
    m_listView->setLayoutMode(QListView::Batched);
    m_listView->setBatchSize(80);
    m_listView->setSpacing(6);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    m_listDelegate = new BookDelegate(m_listView);
    m_listDelegate->setGridMode(false);
    m_listView->setItemDelegate(m_listDelegate);

    connect(m_listView, &QListView::doubleClicked,
            this, [this](const QModelIndex& idx) { openBookFile(idx); });

    m_stack->addWidget(m_gridView);
    m_stack->addWidget(m_listView);
    m_stack->setCurrentWidget(m_gridView);

    if (m_gridView->selectionModel()) {
        connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, [this]() { updateWorkspaceHeader(); });
    }
    if (m_listView->selectionModel()) {
        connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, [this]() { updateWorkspaceHeader(); });
    }

    // Context menu for both views
    for (QListView* v : {m_gridView, m_listView}) {
        v->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(v, &QListView::customContextMenuRequested,
                this, [this](const QPoint& pos) {
            QListView* view = currentView();
            QModelIndex idx = view->indexAt(pos);
            QMenu menu;
            menu.setStyleSheet(styleSheet());
            if (idx.isValid()) {
                QAction* openAct = menu.addAction("Open");
                connect(openAct, &QAction::triggered, this, [this, idx]() { openBookFile(idx); });
                QAction* editAct = menu.addAction("Properties...");
                connect(editAct, &QAction::triggered, this, [this, idx]() { openBookDetail(idx); });
                menu.addSeparator();
                QAction* renameSelectedAct = menu.addAction("Smart Rename Selected");
                connect(renameSelectedAct, &QAction::triggered, this, &MainWindow::onSmartRenameSelected);
                QAction* fetchAct = menu.addAction("Fetch Metadata");
                connect(fetchAct, &QAction::triggered, this, [this, idx]() {
                    QModelIndex src = m_filterModel->mapToSource(idx);
                    const Book& b = m_model->bookAt(src.row());
                    scheduleMetadataFetch(b);
                });
                QAction* refsAct = menu.addAction("Related References...");
                connect(refsAct, &QAction::triggered, this, [this, idx]() {
                    const QModelIndex src = m_filterModel->mapToSource(idx);
                    if (src.isValid())
                        showReferenceLookup(m_model->bookAt(src.row()));
                });
                menu.addSeparator();
                QAction* removeAct = menu.addAction("Remove from Library");
                connect(removeAct, &QAction::triggered, this, &MainWindow::removeSelectedBook);
            }
            if (!menu.isEmpty())
                menu.exec(view->viewport()->mapToGlobal(pos));
        });
    }
}

// ── Status bar ───────────────────────────────────────────────
void MainWindow::setupStatusBar()
{
    // Main status message
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setObjectName("statusPrimary");
    m_statusLabel->setMinimumWidth(200);
    statusBar()->addWidget(m_statusLabel, 1);

    // Background task label
    m_scanLabel = new QLabel;
    m_scanLabel->setObjectName("statusSecondary");
    m_scanLabel->setMinimumWidth(180);
    statusBar()->addWidget(m_scanLabel, 1);

    // Progress bar (compact, only visible during tasks)
    m_progressBar = new QProgressBar;
    m_progressBar->setObjectName("statusProgress");
    m_progressBar->setRange(0, 100);
    m_progressBar->setFixedWidth(180);
    m_progressBar->setFixedHeight(5);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);
    statusBar()->addWidget(m_progressBar);

    // Permanent: library stats pill
    m_libraryStatsLabel = new QLabel;
    m_libraryStatsLabel->setObjectName("infoChip");
    statusBar()->addPermanentWidget(m_libraryStatsLabel);

    // Permanent: version badge
    auto* versionLabel = new QLabel("v" + AppConfig::version());
    versionLabel->setObjectName("infoChip");
    versionLabel->setToolTip("Eagle Library " + AppConfig::version());
    statusBar()->addPermanentWidget(versionLabel);

    statusBar()->setSizeGripEnabled(false);
}

void MainWindow::retranslateUi()
{
    const QString currentLibrary = m_libraryCombo ? m_libraryCombo->currentText() : m_currentLibraryName;
    const QString currentShelf = normalizeShelfId(m_activeShelfName);
    const QString currentFormat = m_formatCombo ? m_formatCombo->currentText() : QStringLiteral("All");
    const QString currentCategory = m_categoryCombo ? m_categoryCombo->currentData().toString() : QString();
    const int currentSortField = m_sortCombo ? m_sortCombo->currentData().toInt() : static_cast<int>(SortField::Title);
    const bool favChecked = m_favAction ? m_favAction->isChecked() : false;
    const QString searchText = m_searchBox ? m_searchBox->text() : QString();
    const bool sidebarVisible = m_sidebarDock ? m_sidebarDock->isVisible() : false;

    setWindowTitle(trl("app.windowTitle", "Eagle Library | Professional Workspace"));
    menuBar()->clear();
    setupMenuBar();
    PluginManager::instance().setPluginMenu(m_pluginMenu);
    PluginManager::instance().syncPluginMenu();

    if (m_mainToolBar) {
        QToolBar* oldToolBar = m_mainToolBar;
        removeToolBar(oldToolBar);
        m_mainToolBar = nullptr;
        delete oldToolBar;
    }
    setupToolBar();
    refreshLibrarySelector();
    if (m_libraryCombo) {
        const int libIndex = m_libraryCombo->findText(currentLibrary);
        m_libraryCombo->setCurrentIndex(libIndex >= 0 ? libIndex : 0);
    }
    refreshShelfOptions();
    if (m_shelfCombo) {
        const int shelfIndex = m_shelfCombo->findData(currentShelf);
        m_shelfCombo->setCurrentIndex(shelfIndex >= 0 ? shelfIndex : 0);
    }
    if (m_formatCombo)
        m_formatCombo->setCurrentText(currentFormat);
    refreshCategoryOptions();
    if (m_categoryCombo) {
        const int categoryIndex = m_categoryCombo->findData(currentCategory);
        m_categoryCombo->setCurrentIndex(categoryIndex >= 0 ? categoryIndex : 0);
    }
    if (m_sortCombo) {
        const int sortIndex = m_sortCombo->findData(currentSortField);
        m_sortCombo->setCurrentIndex(sortIndex >= 0 ? sortIndex : 0);
    }
    if (m_favAction)
        m_favAction->setChecked(favChecked);
    if (m_searchBox)
        m_searchBox->setText(searchText);

    if (m_sidebarDock) {
        QDockWidget* oldSidebarDock = m_sidebarDock;
        removeDockWidget(oldSidebarDock);
        m_sidebarDock = nullptr;
        m_sidebarScrollArea = nullptr;
        m_sidebarContent = nullptr;
        m_sidebarLayout = nullptr;
        m_smartCategorySection = nullptr;
        m_smartCategoryButtonsLayout = nullptr;
        m_savedSearchSection = nullptr;
        m_savedSearchButtonsLayout = nullptr;
        m_sidebarStatsLabel = nullptr;
        delete oldSidebarDock;
    }
    setupSidebar();
    if (m_sidebarDock)
        m_sidebarDock->setVisible(sidebarVisible);

    if (m_statusLabel && (m_statusLabel->text().trimmed().isEmpty() || m_statusLabel->text() == QStringLiteral("Ready")))
        m_statusLabel->setText(trl("status.ready", "Ready"));
    if (m_scanLabel && (m_scanLabel->text().trimmed().isEmpty() || m_scanLabel->text() == QStringLiteral("Idle")))
        m_scanLabel->setText(trl("status.idle", "Idle"));

    updateStatusCount();
    updateWorkspaceHeader();
}

void MainWindow::showTaskProgress(const QString& title, const QString& status, int current, int total, const QString& detail)
{
    if (m_isClosing || !m_statusLabel || !m_scanLabel || !m_progressBar)
        return;

    const QString detailLine = detail.trimmed().isEmpty()
        ? QString()
        : QString("\n%1").arg(detail.left(120));

    m_statusLabel->setText(QString("%1  %2").arg(title, status));
    m_scanLabel->setText(detail.left(50));
    if (total > 0) {
        m_progressBar->setRange(0, 100);
        m_progressBar->setVisible(true);
        m_progressBar->setValue((current * 100) / qMax(1, total));
    } else {
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0);
    }
}

void MainWindow::hideTaskProgress(const QString& finalStatus)
{
    if (!m_progressBar || !m_scanLabel || !m_statusLabel)
        return;
    if (m_progressBar->maximum() > 0)
        m_progressBar->setValue(100);
    m_progressBar->setRange(0, 100);
    m_progressBar->setVisible(false);
    m_scanLabel->setText(trl("status.idle", "Idle"));
    if (!finalStatus.isEmpty())
        m_statusLabel->setText(finalStatus);
}

// ── Slots ─────────────────────────────────────────────────────
void MainWindow::onScanStart()
{
    if (m_watchFolders.isEmpty()) { openSettings(); return; }
    m_recentlyAddedBooks.clear();
    const QString modeLabel = m_fastScanMode ? "Fast scan mode enabled" : "Deep metadata scan enabled";
    showTaskProgress("Scanning Library", "Scanning library folders...", 0, 0, modeLabel);
    m_scanner->startScan(m_watchFolders, m_scanThreads, m_fastScanMode); // 0 = auto-detect CPU count
}

void MainWindow::onScanProgress(int done, int total, const QString& file)
{
    showTaskProgress("Scanning Library",
                     QString("Scanning %1/%2").arg(done).arg(total),
                     done,
                     total,
                     file);
}

void MainWindow::onBookFound(Book book)
{
    m_model->addBook(book);
    m_recentlyAddedBooks << book;
    PluginManager::instance().notifyBookAdded(book.id);
    QSettings s("Eagle Software", "Eagle Library");
    if (!m_autoEnrichAfterScan && s.value("options/autoMeta", true).toBool())
        scheduleMetadataFetch(book);
}

void MainWindow::onScanFinished(int added, int skipped)
{
    hideTaskProgress(QString("Scan complete -- %1 new, %2 already indexed").arg(added).arg(skipped));
    refreshLibraryWatcher();
    updateStatusCount();
    if (m_autoEnrichAfterScan && !m_recentlyAddedBooks.isEmpty())
        enrichIncompleteBooks(m_recentlyAddedBooks, "Post-Scan Enrichment");
    if (m_incrementalScanPending)
        m_libraryChangeTimer->start();
}

void MainWindow::onMetadataReady(qint64 id, Book updated)
{
    if (m_isClosing || !m_model)
        return;
    const Book* current = m_model->bookById(id);
    if (!current) return;

    Book merged = mergeFetchedMetadata(*current, updated);
    m_model->updateBook(merged);
    Database::instance().updateBook(merged);
    PluginManager::instance().notifyBookUpdated(id);
}

void MainWindow::onCoverUrlsReady(qint64 id, const QStringList& urls)
{
    if (m_isClosing || !m_coverDownloader)
        return;
    QSettings s("Eagle Software", "Eagle Library");
    const bool forceCover = m_forceCoverFetchIds.remove(id);
    if (!forceCover && !s.value("options/autoCover", true).toBool())
        return;

    const Book* book = m_model->bookById(id);
    m_coverDownloader->enqueue(id, urls, book ? book->displayTitle() : QString::number(id));
}

void MainWindow::onCoverReady(qint64 id, const QString& path)
{
    if (m_isClosing || !m_model)
        return;
    m_model->updateCover(id, path);
    Database::instance().updateCoverPath(id, path);
}

void MainWindow::onMetadataFetchProgress(int completed, int total, const QString& currentFile, const QString& stage)
{
    if (m_isClosing)
        return;
    showTaskProgress("Fetching Metadata",
                     QString("Metadata %1/%2").arg(completed).arg(total),
                     completed,
                     total,
                     QString("%1  %2").arg(stage, currentFile));

    if (completed >= total && total > 0)
        hideTaskProgress(QString("Metadata fetch complete for %1 books.").arg(total));
}

void MainWindow::onMetadataFetchError(qint64 id, const QString& msg)
{
    if (m_isClosing || !m_statusLabel || !m_model)
        return;
    m_forceCoverFetchIds.remove(id);
    const Book* book = m_model->bookById(id);
    m_statusLabel->setText(QString("Metadata skipped for %1: %2")
                           .arg(book ? book->displayTitle() : QString::number(id), msg));
}

void MainWindow::onCoverDownloadProgress(int completed, int total, const QString& currentLabel)
{
    if (m_isClosing)
        return;
    if (total <= 0) return;
    showTaskProgress("Downloading Covers",
                     QString("Covers %1/%2").arg(completed).arg(total),
                     completed,
                     total,
                     currentLabel);

    if (completed >= total)
        hideTaskProgress(QString("Cover download complete for %1 books.").arg(total));
}

void MainWindow::onCoverDownloadFailed(qint64 id, const QString& reason)
{
    if (m_isClosing || !m_statusLabel || !m_model)
        return;
    const Book* book = m_model->bookById(id);
    m_statusLabel->setText(QString("No cover found for %1: %2")
                           .arg(book ? book->displayTitle() : QString::number(id), reason));
}

void MainWindow::openSettings()
{
    const QStringList previousFolders = m_watchFolders;
    const QString previousLibrary = m_currentLibraryName;
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        loadSettings();
        applyResponsiveLayout();
        reloadCurrentLibrary();

        // Remove books from folders that were dropped from the watch list
        QStringList removedFolders;
        for (const QString& f : previousFolders)
            if (!m_watchFolders.contains(f, Qt::CaseInsensitive))
                removedFolders << f;
        if (!removedFolders.isEmpty()) {
            const int removed = Database::instance().removeBooksForFolders(removedFolders);
            if (removed > 0) {
                reloadCurrentLibrary();
                m_statusLabel->setText(QString("Removed %1 books from dropped folders.").arg(removed));
            }
        }
        // Delete the whole library DB if no folders remain at all
        if (m_watchFolders.isEmpty()) {
            Database::instance().removeBooksForFolders(previousFolders);
            reloadCurrentLibrary();
        }

        if (!m_watchFolders.isEmpty())
            QTimer::singleShot(200, this, &MainWindow::onScanStart);

        if (previousLibrary != m_currentLibraryName)
            m_statusLabel->setText(QString("Switched to library \"%1\".").arg(m_currentLibraryName));
        else if (previousFolders != m_watchFolders)
            m_statusLabel->setText(QString("Updated watched folders for \"%1\".").arg(m_currentLibraryName));
        else
            updateStatusCount();
    }
}

void MainWindow::openBookDetail(const QModelIndex& index)
{
    QModelIndex srcIdx = m_filterModel->mapToSource(index);
    if (!srcIdx.isValid()) return;
    const Book& b = m_model->bookAt(srcIdx.row());
    BookDetailDialog dlg(b, this);
    int result = dlg.exec();
    if (result == QDialog::Accepted) {
        Book edited = dlg.editedBook();
        m_model->updateBook(edited);
        Database::instance().updateBook(edited);
        PluginManager::instance().notifyBookUpdated(edited.id);
    } else if (result == 2) {
        scheduleMetadataFetch(b);
    }
}

void MainWindow::openBookDetail()
{
    QListView* v = currentView();
    QModelIndexList sel = v->selectionModel()->selectedIndexes();
    if (!sel.isEmpty()) openBookDetail(sel.first());
}

void MainWindow::openBookFile(const QModelIndex& index)
{
    QModelIndex srcIdx = m_filterModel->mapToSource(index);
    if (!srcIdx.isValid()) return;

    const Book& b = m_model->bookAt(srcIdx.row());
    QDesktopServices::openUrl(QUrl::fromLocalFile(b.filePath));
    Database::instance().markOpened(b.id);
}

void MainWindow::openBookFile()
{
    QListView* v = currentView();
    QModelIndexList sel = v->selectionModel()->selectedIndexes();
    if (!sel.isEmpty()) openBookFile(sel.first());
}

void MainWindow::setGridView()
{
    m_isGridView = true;
    m_stack->setCurrentWidget(m_gridView);
    m_gridAction->setChecked(true);
    m_listAction->setChecked(false);
    updateWorkspaceHeader();
}

void MainWindow::setListView()
{
    m_isGridView = false;
    m_stack->setCurrentWidget(m_listView);
    m_listAction->setChecked(true);
    m_gridAction->setChecked(false);
    updateWorkspaceHeader();
}


void MainWindow::searchChanged(const QString& text)
{
    m_filterModel->setFilterText(text);
    updateStatusCount();
}

void MainWindow::filterByFormat(const QString& fmt)
{
    m_filterModel->setFilterFormat(fmt == "All" ? QString() : fmt);
    updateStatusCount();
}

void MainWindow::filterByCategory(const QString& category)
{
    if (!m_categoryCombo)
        return;
    QString value = m_categoryCombo->currentData().toString();
    if (value.isEmpty() && category != trl("category.all", "All Categories"))
        value = category;
    m_filterModel->setFilterCategory(value);
    updateStatusCount();
}

void MainWindow::showFavourites(bool on)
{
    m_filterModel->setFilterFavourites(on);
    updateStatusCount();
}

void MainWindow::showNoCoverFilter(bool on)
{
    m_filterModel->setFilterNoCover(on);
    updateStatusCount();
}

void MainWindow::showNoMetaFilter(bool on)
{
    m_filterModel->setFilterNoMeta(on);
    updateStatusCount();
}

void MainWindow::openAdvancedSearch()
{
    AdvancedSearchDialog dlg(this);
    dlg.setStyleSheet(styleSheet());
    if (dlg.exec() != QDialog::Accepted)
        return;

    m_searchBox->setText(dlg.text());
    m_filterModel->setFilterText(dlg.text());
    m_filterModel->setFilterAuthor(dlg.author());
    m_filterModel->setFilterYear(dlg.yearFrom(), dlg.yearTo());
    m_filterModel->setFilterFavourites(dlg.favouritesOnly());
    m_filterModel->setFilterNoCover(dlg.noCoverOnly());
    m_filterModel->setFilterNoMeta(dlg.noMetaOnly());

    const QString format = dlg.format();
    if (m_formatCombo)
        m_formatCombo->setCurrentText(format.isEmpty() ? "All" : format);
    m_filterModel->setFilterFormat(format);

    const QString category = dlg.category();
    if (m_categoryCombo)
        m_categoryCombo->setCurrentText(category.isEmpty() ? "All Categories" : category);
    m_filterModel->setFilterCategory(category);

    updateStatusCount();
    m_statusLabel->setText("Advanced search applied.");
}

void MainWindow::sortChanged(int idx)
{
    SortField sf = (SortField)m_sortCombo->itemData(idx).toInt();
    m_filterModel->sortBy(sf, m_sortOrder);
    // Also reload model from DB with same sort for consistency
    m_model->reload(m_model->currentFilter(), sf, m_sortOrder);
    updateStatusCount();
}

// ── Smart Rename ─────────────────────────────────────────────
void MainWindow::onSmartRenameAll()
{
    QVector<Book> books = currentLibraryBooks();
    if (books.isEmpty()) {
        m_statusLabel->setText("No books in library to rename.");
        return;
    }

    startSmartRename(
        books,
        "Smart Rename",
        QString("Analyse %1 books and auto-rename those whose title\n"
                "matches their filename (i.e. no real metadata yet)?\n\n"
                "Books with good metadata are NOT touched.\n"
                "Embedded PDF metadata is used when available.\n"
                "This reads filename patterns like:\n"
                "  Author - Title (Year)\n"
                "  Title - Author\n"
                "  Title (Year)").arg(books.size()));
}

void MainWindow::onSmartRenameSelected()
{
    QListView* v = currentView();
    QModelIndexList sel = v->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) {
        m_statusLabel->setText("No selected books to rename.");
        return;
    }

    QVector<Book> books;
    QSet<qint64> seen;
    for (const QModelIndex& proxyIdx : sel) {
        QModelIndex srcIdx = m_filterModel->mapToSource(proxyIdx);
        if (!srcIdx.isValid())
            continue;
        const Book& book = m_model->bookAt(srcIdx.row());
        if (!seen.contains(book.id)) {
            seen.insert(book.id);
            books << book;
        }
    }

    if (books.isEmpty()) {
        m_statusLabel->setText("No selected books to rename.");
        return;
    }

    startSmartRename(
        books,
        "Smart Rename Selected",
        QString("Analyse %1 selected books and update only their metadata in the library database?\n\n"
                "Files on disk will not be renamed or moved.\n"
                "Books with good metadata are not touched.\n"
                "Embedded PDF metadata and filename patterns will be used when available.")
            .arg(books.size()));
}

void MainWindow::startSmartRename(const QVector<Book>& books, const QString& title, const QString& prompt)
{
    int reply = QMessageBox::question(this, title, prompt, QMessageBox::Yes | QMessageBox::Cancel);
    if (reply != QMessageBox::Yes) return;

    // Run renamer on background thread
    if (m_renameThread && m_renameThread->isRunning()) {
        m_statusLabel->setText("Smart rename is already running.");
        return;
    }

    showTaskProgress(title, "Preparing smart rename...", 0, books.size(), "Analyzing selected metadata");

    m_renameThread = new QThread(this);
    m_activeRenamer = new SmartRenamer;
    m_activeRenamer->moveToThread(m_renameThread);

    connect(m_activeRenamer, &SmartRenamer::renamed, this, &MainWindow::onRenameResult, Qt::QueuedConnection);
    connect(m_activeRenamer, &SmartRenamer::progress, this, [this](int done, int total, const QString& currentFile, const QString& detail) {
        showTaskProgress("Smart Rename",
                         QString("Smart rename %1/%2").arg(done).arg(total),
                         done,
                         total,
                         QString("%1  %2").arg(detail, currentFile));
    }, Qt::QueuedConnection);
    connect(m_activeRenamer, &SmartRenamer::finished, this, &MainWindow::onRenameFinished, Qt::QueuedConnection);

    connect(m_renameThread, &QThread::started, m_activeRenamer, [this, books]() {
        if (m_activeRenamer)
            m_activeRenamer->renameAll(books);
    }, Qt::QueuedConnection);
    connect(m_renameThread, &QThread::finished, m_activeRenamer, &QObject::deleteLater);
    connect(m_renameThread, &QThread::finished, m_renameThread, &QObject::deleteLater);
    m_renameThread->start(QThread::LowPriority);
}

QVector<Book> MainWindow::chooseBooksScope(const QString& featureName)
{
    const QVector<Book> selected = selectedBooks(m_model, m_filterModel, currentView());
    if (selected.isEmpty())
        return currentLibraryBooks();

    QMessageBox box(this);
    box.setWindowTitle(featureName);
    box.setText(QString("Choose the scope for %1.").arg(featureName));
    box.setInformativeText(QString("%1 books are currently selected.").arg(selected.size()));
    QPushButton* selectedBtn = box.addButton("Selected Books", QMessageBox::AcceptRole);
    QPushButton* allBtn = box.addButton("All Books", QMessageBox::ActionRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(selectedBtn);
    box.setStyleSheet(styleSheet());
    box.exec();

    if (box.clickedButton() == selectedBtn)
        return selected;
    if (box.clickedButton() == allBtn)
        return currentLibraryBooks();
    return {};
}

QString MainWindow::saveJsonArtifact(const QString& baseName, const QJsonObject& payload) const
{
    QDir().mkpath(AppConfig::jsonDir());
    const QString fileName = QString("%1/%2-%3.json")
        .arg(AppConfig::jsonDir(),
             baseName,
             QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return {};

    const QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size() || !file.commit())
        return {};

    return fileName;
}

void MainWindow::onRenameResult(RenameResult r)
{
    // Apply to DB
    Database::instance().batchUpdateTitle(r.bookId, r.newTitle, r.newAuthor, r.newPublisher, r.newYear);

    // Update model in place
    const Book* b = m_model->bookById(r.bookId);
    if (b) {
        Book updated = *b;
        updated.title  = r.newTitle;
        updated.author = r.newAuthor.isEmpty() ? updated.author : r.newAuthor;
        updated.publisher = r.newPublisher.isEmpty() ? updated.publisher : r.newPublisher;
        if (r.newYear > 0) updated.year = r.newYear;
        m_model->updateBook(updated);
    }
    PluginManager::instance().notifyBookUpdated(r.bookId);
}

void MainWindow::onRenameFinished(int changed)
{
    if (m_renameThread) {
        m_renameThread->quit();
        m_renameThread->wait(3000);
        m_renameThread = nullptr;
    }
    m_activeRenamer = nullptr;
    hideTaskProgress(QString("Smart rename complete - %1 books updated.").arg(changed));
    if (changed > 0) reloadCurrentLibrary();
}

void MainWindow::cleanMissingFiles()
{
    showTaskProgress("Cleaning Library", "Removing missing file records...", 0, 0);
    int removed = Database::instance().removeMissingFiles();
    reloadCurrentLibrary();
    hideTaskProgress(QString("Cleaned %1 missing file records.").arg(removed));
    updateStatusCount();
}

void MainWindow::fetchAllMetadata()
{
    QVector<Book> books = currentLibraryBooks();
    int count = 0;
    for (const auto& b : books) {
        const bool missingCoreMeta = b.title.isEmpty() || b.author.isEmpty()
            || b.publisher.isEmpty() || b.isbn.isEmpty() || b.year == 0;
        if (missingCoreMeta) {
            scheduleMetadataFetch(b);
            ++count;
        }
    }
    if (count == 0)
        m_statusLabel->setText("No books need metadata fetching.");
    else
        m_statusLabel->setText(QString("Queued metadata fetch for %1 books...").arg(count));
}

void MainWindow::openMetadataManager()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Metadata Manager");
    dlg.setMinimumWidth(520);

    auto* layout = new QVBoxLayout(&dlg);

    auto* intro = new QLabel(
        "<b>Metadata Manager</b><br>"
        "Choose which books to update, whether to refresh all metadata or only incomplete records, "
        "and which metadata sources should be used.");
    intro->setWordWrap(true);
    intro->setTextFormat(Qt::RichText);
    layout->addWidget(intro);

    auto* scopeBox = new QGroupBox("Scope");
    auto* scopeLayout = new QVBoxLayout(scopeBox);
    auto* missingOnlyRadio = new QRadioButton("Only books with missing metadata");
    auto* selectedRadio = new QRadioButton("Selected books only");
    auto* allRadio = new QRadioButton("All books in the active library");
    missingOnlyRadio->setChecked(true);
    scopeLayout->addWidget(missingOnlyRadio);
    scopeLayout->addWidget(selectedRadio);
    scopeLayout->addWidget(allRadio);
    layout->addWidget(scopeBox);

    auto* sourceBox = new QGroupBox("Sources");
    auto* sourceLayout = new QVBoxLayout(sourceBox);
    auto* embeddedCheck = new QCheckBox("Embedded file metadata");
    auto* googleCheck = new QCheckBox("Google Books");
    auto* openLibCheck = new QCheckBox("Open Library");
    embeddedCheck->setChecked(true);
    googleCheck->setChecked(true);
    openLibCheck->setChecked(true);
    sourceLayout->addWidget(embeddedCheck);
    sourceLayout->addWidget(googleCheck);
    sourceLayout->addWidget(openLibCheck);
    layout->addWidget(sourceBox);

    auto* optionsBox = new QGroupBox("Options");
    auto* optionsLayout = new QVBoxLayout(optionsBox);
    auto* forceCoverCheck = new QCheckBox("Force cover refresh too");
    auto* rereadEmbeddedOnlyCheck = new QCheckBox("Embedded-only reread mode");
    optionsLayout->addWidget(forceCoverCheck);
    optionsLayout->addWidget(rereadEmbeddedOnlyCheck);
    layout->addWidget(optionsBox);

    connect(rereadEmbeddedOnlyCheck, &QCheckBox::toggled, &dlg, [=](bool on) {
        googleCheck->setEnabled(!on);
        openLibCheck->setEnabled(!on);
        if (on) {
            embeddedCheck->setChecked(true);
            googleCheck->setChecked(false);
            openLibCheck->setChecked(false);
        }
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    if (!embeddedCheck->isChecked() && !googleCheck->isChecked() && !openLibCheck->isChecked()) {
        QMessageBox::warning(this, "Metadata Manager", "Select at least one metadata source.");
        return;
    }

    QVector<Book> books;
    if (selectedRadio->isChecked())
        books = selectedBooks(m_model, m_filterModel, currentView());
    else
        books = currentLibraryBooks();
    if (books.isEmpty()) {
        m_statusLabel->setText("No books matched the chosen metadata scope.");
        return;
    }

    int queued = 0;
    for (const Book& book : books) {
        const bool missingCoreMeta = book.title.isEmpty() || book.author.isEmpty()
            || book.publisher.isEmpty() || book.isbn.isEmpty() || book.year == 0;
        if (missingOnlyRadio->isChecked() && !missingCoreMeta)
            continue;

        scheduleMetadataFetch(book,
                              forceCoverCheck->isChecked(),
                              embeddedCheck->isChecked(),
                              googleCheck->isChecked(),
                              openLibCheck->isChecked());
        ++queued;
    }

    if (queued == 0)
        m_statusLabel->setText("No books needed metadata updates for the selected mode.");
    else
        m_statusLabel->setText(QString("Queued metadata updates for %1 books.").arg(queued));
}

void MainWindow::enrichIncompleteBooksAction()
{
    const QVector<Book> scoped = chooseBooksScope("Enrich Incomplete Books");
    if (scoped.isEmpty())
        return;
    enrichIncompleteBooks(scoped, "Enrich Incomplete Books");
}

void MainWindow::runQualityCheck()
{
    const QVector<Book> books = chooseBooksScope("Quality Check");
    if (books.isEmpty())
        return;
    int missingAuthor = 0, missingCover = 0, missingIsbn = 0, weakTitle = 0, badIsbn = 0, suspectPublisher = 0;
    int index = 0;
    for (const Book& book : books) {
        ++index;
        showTaskProgress("Quality Check",
                         QString("Checking %1/%2 books").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());
        if (book.author.trimmed().isEmpty()) ++missingAuthor;
        if (!book.hasCover || book.coverPath.isEmpty()) ++missingCover;
        if (book.isbn.trimmed().isEmpty()) ++missingIsbn;
        if (book.title.trimmed().isEmpty() || book.title.compare(QFileInfo(book.filePath).completeBaseName(), Qt::CaseInsensitive) == 0)
            ++weakTitle;
        const QString isbn = sanitizeIsbn(book.isbn);
        if (!isbn.isEmpty() && !(isValidIsbn10(isbn) || isValidIsbn13(isbn))) ++badIsbn;
        const QString publisher = book.publisher.toLower();
        if (publisher.contains("tcpdf") || publisher.contains("acrobat") || publisher.contains("libreoffice"))
            ++suspectPublisher;
    }

    QJsonObject report;
    report["feature"] = "quality_check";
    report["checked_books"] = books.size();
    report["weak_title"] = weakTitle;
    report["missing_author"] = missingAuthor;
    report["missing_isbn"] = missingIsbn;
    report["missing_cover"] = missingCover;
    report["malformed_isbn"] = badIsbn;
    report["suspicious_publisher"] = suspectPublisher;
    const QString reportPath = saveJsonArtifact("quality-check", report);
    hideTaskProgress(QString("Quality check complete for %1 books.").arg(books.size()));

    QMessageBox::information(
        this,
        "Quality Check",
        QString("Checked %1 books.\n\n"
                "Weak title metadata: %2\n"
                "Missing author: %3\n"
                "Missing ISBN: %4\n"
                "Missing cover: %5\n"
                "Malformed ISBN: %6\n"
                "Suspicious generated publisher metadata: %7\n\n"
                "JSON report: %8")
            .arg(books.size()).arg(weakTitle).arg(missingAuthor).arg(missingIsbn).arg(missingCover).arg(badIsbn).arg(suspectPublisher)
            .arg(reportPath.isEmpty() ? "not saved" : QDir::toNativeSeparators(reportPath)));
}

void MainWindow::findDuplicates()
{
    const QVector<Book> books = chooseBooksScope("Find Duplicates");
    if (books.isEmpty())
        return;
    QHash<QString, QStringList> groups;
    int index = 0;
    for (const Book& book : books) {
        ++index;
        showTaskProgress("Finding Duplicates",
                         QString("Analyzing %1/%2 books").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());
        const QStringList locations = Database::instance().fileLocationsForBook(book.id);
        const QString detail = QString("%1\n    %2")
            .arg(book.displayTitle(),
                 (locations.isEmpty() ? QDir::toNativeSeparators(book.filePath)
                                      : QDir::toNativeSeparators(locations.first())));
        if (!book.fileHash.isEmpty())
            groups["sha256:" + book.fileHash] << detail;
        if (!book.isbn.trimmed().isEmpty())
            groups["isbn:" + sanitizeIsbn(book.isbn)] << detail;
        groups["meta:" + normalizedText(book.displayTitle()) + "|" + normalizedText(book.displayAuthor())] << detail;
    }

    QStringList hits;
    QJsonArray groupsJson;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        QStringList titles = it.value();
        titles.removeDuplicates();
        if (titles.size() > 1) {
            hits << QString("%1\n  %2").arg(it.key(), titles.join("\n  "));
            QJsonObject group;
            group["key"] = it.key();
            group["titles"] = QJsonArray::fromStringList(titles);
            groupsJson.append(group);
        }
    }

    QJsonObject report;
    report["feature"] = "find_duplicates";
    report["checked_books"] = books.size();
    report["duplicate_groups"] = groupsJson;
    const QString reportPath = saveJsonArtifact("duplicate-report", report);
    hideTaskProgress(QString("Duplicate analysis complete for %1 books.").arg(books.size()));

    QMessageBox::information(
        this,
        "Duplicate Finder",
        hits.isEmpty() ? QString("No duplicate groups found.\n\nJSON report: %1")
                              .arg(reportPath.isEmpty() ? "not saved" : QDir::toNativeSeparators(reportPath))
                       : QString("Potential duplicate groups:\n\n%1\n\nJSON report: %2")
                              .arg(hits.mid(0, 20).join("\n\n"),
                                   reportPath.isEmpty() ? "not saved" : QDir::toNativeSeparators(reportPath)));
}

void MainWindow::extractMissingIsbns()
{
    QVector<Book> books = chooseBooksScope("Extract Missing ISBNs");
    if (books.isEmpty())
        return;
    int updated = 0;
    int queuedMetadata = 0;
    int invalidDetected = 0;
    QJsonArray updates;
    int index = 0;
    for (Book book : books) {
        ++index;
        showTaskProgress("Extracting ISBNs",
                         QString("Scanning %1/%2 books").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());
        const QString existingIsbn = sanitizeIsbn(book.isbn);
        if (!existingIsbn.isEmpty()) {
            if (!(isValidIsbn10(existingIsbn) || isValidIsbn13(existingIsbn)))
                ++invalidDetected;
            continue;
        }
        const QString isbn = extractIsbnFromBookContent(book);
        if (isbn.isEmpty())
            continue;
        const QString cleanIsbn = sanitizeIsbn(isbn);
        if (!(isValidIsbn10(cleanIsbn) || isValidIsbn13(cleanIsbn))) {
            ++invalidDetected;
            continue;
        }
        book.isbn = cleanIsbn;
        Database::instance().updateBook(book);
        m_model->updateBook(book);
        scheduleMetadataFetch(book, true);
        ++queuedMetadata;
        QJsonObject item;
        item["title"] = book.displayTitle();
        item["file_path"] = book.filePath;
        item["isbn"] = cleanIsbn;
        item["metadata_queued"] = true;
        updates.append(item);
        ++updated;
    }
    QJsonObject report;
    report["feature"] = "extract_missing_isbns";
    report["updated_books"] = updated;
    report["queued_metadata_fetches"] = queuedMetadata;
    report["invalid_isbn_candidates"] = invalidDetected;
    report["items"] = updates;
    saveJsonArtifact("isbn-extraction", report);
    hideTaskProgress(QString("ISBN extraction complete. Updated %1 books and queued %2 metadata lookups.").arg(updated).arg(queuedMetadata));
}

void MainWindow::countPagesForLibrary()
{
    QVector<Book> books = chooseBooksScope("Count Pages");
    if (books.isEmpty())
        return;
    int updated = 0;
    QJsonArray updates;
    int index = 0;
    for (Book book : books) {
        ++index;
        showTaskProgress("Counting Pages",
                         QString("Estimating %1/%2 books").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());
        if (book.pages > 0)
            continue;
        const int pages = estimatePagesForBook(book);
        if (pages <= 0)
            continue;
        book.pages = pages;
        Database::instance().updateBook(book);
        m_model->updateBook(book);
        QJsonObject item;
        item["title"] = book.displayTitle();
        item["file_path"] = book.filePath;
        item["pages"] = pages;
        updates.append(item);
        ++updated;
    }
    QJsonObject report;
    report["feature"] = "count_pages";
    report["updated_books"] = updated;
    report["items"] = updates;
    saveJsonArtifact("page-count", report);
    hideTaskProgress(QString("Page counting complete. Updated %1 books.").arg(updated));
}

void MainWindow::generateMissingCovers()
{
    QVector<Book> books = chooseBooksScope("Generate Missing Covers");
    if (books.isEmpty())
        return;
    int generated = 0;
    int index = 0;
    for (Book book : books) {
        ++index;
        showTaskProgress("Generating Covers",
                         QString("Creating %1/%2 covers").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());
        if (book.hasCover && !book.coverPath.isEmpty() && QFileInfo::exists(book.coverPath))
            continue;
        const QString path = AppConfig::coversDir() + "/" + QString::number(book.id) + "_generated.jpg";
        if (!saveGeneratedCover(book, path))
            continue;
        book.coverPath = path;
        book.hasCover = true;
        Database::instance().updateBook(book);
        m_model->updateBook(book);
        ++generated;
    }
    hideTaskProgress(QString("Generated covers for %1 books.").arg(generated));
}

void MainWindow::normalizeCovers()
{
    QVector<Book> books = chooseBooksScope("Normalize Covers");
    if (books.isEmpty())
        return;
    int normalized = 0;
    int index = 0;
    for (const Book& book : books) {
        ++index;
        showTaskProgress("Normalizing Covers",
                         QString("Improving %1/%2 covers").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());
        if (normalizeCoverFile(book.coverPath))
            ++normalized;
    }
    if (normalized > 0)
        reloadCurrentLibrary();
    hideTaskProgress(QString("Normalized %1 managed covers. Original book files were not modified.").arg(normalized));
}

void MainWindow::smartCategorizeLibrary()
{
    QVector<Book> books = chooseBooksScope("Smart Categorize");
    if (books.isEmpty())
        return;

    int updated = 0;
    QJsonArray items;
    int index = 0;
    for (Book book : books) {
        ++index;
        showTaskProgress("Smart Categorize",
                         QString("Categorizing %1/%2 books").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());

        const QStringList inferredTags = inferSmartTags(book);
        const QStringList mergedTags = uniqueCaseInsensitive(book.tags + inferredTags);
        if (mergedTags == book.tags)
            continue;

        book.tags = mergedTags;
        if (book.subjects.isEmpty())
            book.subjects = mergedTags.mid(0, qMin(3, mergedTags.size()));

        Database::instance().updateBook(book);
        m_model->updateBook(book);

        QJsonObject item;
        item["title"] = book.displayTitle();
        item["file_path"] = book.filePath;
        item["tags"] = QJsonArray::fromStringList(book.tags);
        items.append(item);
        ++updated;
    }

    QJsonObject report;
    report["feature"] = "smart_categorize";
    report["updated_books"] = updated;
    report["items"] = items;
    saveJsonArtifact("smart-categorize", report);
    rebuildSmartCategorySidebar();
    hideTaskProgress(QString("Smart categorization complete. Updated %1 books.").arg(updated));
}

void MainWindow::scheduleMetadataFetch(const Book& book,
                                       bool forceCover,
                                       bool useEmbedded,
                                       bool useGoogle,
                                       bool useOpenLibrary)
{
    if (forceCover)
        m_forceCoverFetchIds.insert(book.id);

    FetchRequest req;
    req.bookId = book.id;
    req.title  = book.title;
    req.author = book.author;
    req.isbn   = book.isbn;
    req.filePath = book.filePath;
    req.format = book.format;
    req.useEmbedded = useEmbedded;
    req.useGoogle = useGoogle;
    req.useOpenLibrary = useOpenLibrary;
    m_metaFetcher->enqueue(req);
}

void MainWindow::removeSelectedBook()
{
    QListView* v = currentView();
    QModelIndexList sel = v->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;
    int res = QMessageBox::question(this, "Remove Books",
        QString("Remove %1 book(s) from library?\n(Files will NOT be deleted.)").arg(sel.size()),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (res != QMessageBox::Yes) return;
    for (const auto& proxyIdx : sel) {
        QModelIndex srcIdx = m_filterModel->mapToSource(proxyIdx);
        qint64 id = m_model->bookAt(srcIdx.row()).id;
        Database::instance().removeBook(id);
        m_model->removeBook(id);
        PluginManager::instance().notifyBookRemoved(id);
    }
    updateStatusCount();
}

void MainWindow::refreshLibrary()
{
    reloadCurrentLibrary();
    updateStatusCount();
    updateWorkspaceHeader();
    rebuildSmartCategorySidebar();
}

void MainWindow::exportLibrarySnapshot()
{
    const QVector<Book> books = currentLibraryBooks();
    if (books.isEmpty()) {
        QMessageBox::information(this, "Export Library", "The active library has no books to export.");
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Export Library",
        QDir::homePath() + "/" + m_currentLibraryName.toLower().replace(' ', '-') + "-export.json",
        "Eagle Library Export (*.json);;CSV Spreadsheet (*.csv);;BibTeX (*.bib);;RIS (*.ris)");
    if (path.isEmpty())
        return;

    bool ok = false;
    if (path.endsWith(".csv", Qt::CaseInsensitive))
        ok = exportBooksToCsv(path, books);
    else if (path.endsWith(".bib", Qt::CaseInsensitive))
        ok = exportBooksToBibTex(path, books);
    else if (path.endsWith(".ris", Qt::CaseInsensitive))
        ok = exportBooksToRis(path, books);
    else
        ok = Database::instance().exportLibrary(path, m_watchFolders);

    if (ok)
        m_statusLabel->setText(QString("Library exported to %1").arg(QFileInfo(path).fileName()));
    else
        QMessageBox::warning(this, "Export Library", "Failed to export the active library.");
}

void MainWindow::importLibrarySnapshot()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Import Library",
        QDir::homePath(),
        "Eagle Library Export (*.json)");
    if (path.isEmpty())
        return;

    QStringList importedFolders;
    const int imported = Database::instance().importLibrary(path, &importedFolders);
    if (imported <= 0) {
        QMessageBox::warning(this, "Import Library", "No books were imported from this file.");
        return;
    }

    if (!importedFolders.isEmpty()) {
        QSettings s("Eagle Software", "Eagle Library");
        m_libraryProfiles = loadLibraryProfiles(s);
        const QStringList existingFolders = foldersForLibrary(m_libraryProfiles, m_currentLibraryName);
        QSet<QString> merged(existingFolders.begin(), existingFolders.end());
        for (const QString& folder : importedFolders)
            merged.insert(folder);
        m_watchFolders = QStringList(merged.begin(), merged.end());
        m_watchFolders.sort(Qt::CaseInsensitive);
        QJsonArray foldersJson;
        for (const QString& folder : m_watchFolders)
            foldersJson.append(folder);
        m_libraryProfiles[m_currentLibraryName] = foldersJson;
        s.setValue("library/profiles", QJsonDocument(m_libraryProfiles).toJson(QJsonDocument::Compact));
        s.setValue("library/folders", m_watchFolders);
    }

    reloadCurrentLibrary();
    updateStatusCount();
    m_statusLabel->setText(QString("Imported %1 books from %2").arg(imported).arg(QFileInfo(path).fileName()));
}

void MainWindow::stopAllTasks()
{
    m_recentlyAddedBooks.clear();
    m_forceCoverFetchIds.clear();

    if (m_scanner && m_scanner->isRunning())
        m_scanner->cancel();

    if (m_activeRenamer)
        m_activeRenamer->cancel();

    if (m_renameThread && m_renameThread->isRunning()) {
        m_renameThread->quit();
        m_renameThread->wait(3000);
        m_renameThread = nullptr;
    }
    m_activeRenamer = nullptr;

    if (m_metaFetcher)
        m_metaFetcher->cancelAll();
    if (m_coverDownloader)
        m_coverDownloader->cancelAll();

    hideTaskProgress(m_isClosing ? QString() : "All running tasks were stopped safely.");
}

void MainWindow::consultDatabaseSummary()
{
    const QVector<Book> books = currentLibraryBooks();
    QSet<QString> formats;
    QSet<QString> tags;
    QSet<QString> authors;
    QSet<QString> languages;
    QMap<QString, int> formatCounts;
    for (const Book& book : books) {
        if (!book.format.isEmpty()) {
            formats.insert(book.format);
            formatCounts[book.format] += 1;
        }
        if (!book.author.isEmpty())
            authors.insert(book.author);
        if (!book.language.isEmpty())
            languages.insert(book.language);
        for (const QString& tag : book.tags)
            if (!tag.trimmed().isEmpty())
                tags.insert(tag.trimmed());
    }

    QStringList lines;
    lines << QString("Database file:\n%1").arg(QDir::toNativeSeparators(AppConfig::dbPath()));
    lines << QString("Active library: %1").arg(m_currentLibraryName);
    lines << QString("Books in active library: %1").arg(books.size());
    lines << QString("Formats indexed: %1").arg(formats.size());
    lines << QString("Authors indexed: %1").arg(authors.size());
    lines << QString("Languages indexed: %1").arg(languages.size());
    lines << QString("Tags indexed: %1").arg(tags.size());

    QStringList formatPreview;
    QStringList formatKeys = formatCounts.keys();
    formatKeys.sort(Qt::CaseInsensitive);
    for (const QString& fmt : formatKeys.mid(0, 8))
        formatPreview << QString("%1 (%2)").arg(fmt).arg(formatCounts.value(fmt));
    if (!formatPreview.isEmpty())
        lines << QString("Top formats: %1").arg(formatPreview.join(", "));

    QMessageBox box(this);
    box.setWindowTitle("Database Summary");
    box.setText(lines.join("\n\n"));
    box.setStyleSheet(styleSheet());
    box.exec();
}

void MainWindow::diagnoseDatabaseText()
{
    const QVector<Book> books = currentLibraryBooks();
    if (books.isEmpty()) {
        QMessageBox::information(this, "Diagnose Text Problems", "The active library has no records to inspect.");
        return;
    }

    int suspiciousCount = 0;
    int emptyTitleCount = 0;
    int filenameTitleCount = 0;
    int replacementCount = 0;
    int mojibakeCount = 0;
    int controlCharCount = 0;
    QJsonArray items;
    QStringList previewLines;

    int index = 0;
    for (const Book& book : books) {
        ++index;
        showTaskProgress("Diagnosing Database Text",
                         QString("Inspecting %1/%2 records").arg(index).arg(books.size()),
                         index,
                         books.size(),
                         book.displayTitle());

        const QStringList reasons = suspiciousTextReasons(book);
        if (reasons.isEmpty())
            continue;

        ++suspiciousCount;
        for (const QString& reason : reasons) {
            if (reason.contains("empty", Qt::CaseInsensitive))
                ++emptyTitleCount;
            if (reason.contains("filename", Qt::CaseInsensitive))
                ++filenameTitleCount;
            if (reason.contains("replacement", Qt::CaseInsensitive) || reason.contains("square", Qt::CaseInsensitive))
                ++replacementCount;
            if (reason.contains("mojibake", Qt::CaseInsensitive))
                ++mojibakeCount;
            if (reason.contains("control characters", Qt::CaseInsensitive))
                ++controlCharCount;
        }

        QJsonObject item;
        item["id"] = QString::number(book.id);
        item["title"] = book.title;
        item["author"] = book.author;
        item["publisher"] = book.publisher;
        item["file_path"] = book.filePath;
        item["reasons"] = QJsonArray::fromStringList(reasons);
        items.append(item);

        if (previewLines.size() < 12)
            previewLines << QString("%1\n  %2").arg(book.displayTitle(), reasons.join(", "));
    }

    QJsonObject report;
    report["feature"] = "diagnose_database_text";
    report["active_library"] = m_currentLibraryName;
    report["checked_records"] = books.size();
    report["suspicious_records"] = suspiciousCount;
    report["empty_title_hits"] = emptyTitleCount;
    report["filename_title_hits"] = filenameTitleCount;
    report["replacement_character_hits"] = replacementCount;
    report["mojibake_hits"] = mojibakeCount;
    report["control_character_hits"] = controlCharCount;
    report["items"] = items;
    const QString reportPath = saveJsonArtifact("database-text-diagnosis", report);

    hideTaskProgress(QString("Database text diagnosis complete. %1 suspicious records found.").arg(suspiciousCount));

    QString message = QString("Checked %1 records in \"%2\".\n\n"
                              "Suspicious records: %3\n"
                              "Empty titles: %4\n"
                              "Filename-based titles: %5\n"
                              "Replacement or square characters: %6\n"
                              "Mojibake-like text: %7\n"
                              "Control-character issues: %8")
        .arg(books.size())
        .arg(m_currentLibraryName)
        .arg(suspiciousCount)
        .arg(emptyTitleCount)
        .arg(filenameTitleCount)
        .arg(replacementCount)
        .arg(mojibakeCount)
        .arg(controlCharCount);

    if (!previewLines.isEmpty())
        message += QString("\n\nExamples:\n%1").arg(previewLines.join("\n\n"));
    if (!reportPath.isEmpty())
        message += QString("\n\nJSON report: %1").arg(QDir::toNativeSeparators(reportPath));

    QMessageBox box(this);
    box.setWindowTitle("Diagnose Text Problems");
    box.setText(message);
    box.setStyleSheet(styleSheet());
    box.exec();
}

void MainWindow::repairDatabaseText()
{
    const QVector<Book> books = currentLibraryBooks();
    if (books.isEmpty()) {
        QMessageBox::information(this, "Repair Text Problems", "The active library has no records to repair.");
        return;
    }

    QVector<Book> targets;
    for (const Book& book : books) {
        if (!suspiciousTextReasons(book).isEmpty())
            targets.push_back(book);
    }

    if (targets.isEmpty()) {
        QMessageBox::information(this, "Repair Text Problems", "No suspicious records were found in the active library.");
        return;
    }

    const int answer = QMessageBox::question(
        this,
        "Repair Text Problems",
        QString("Repair %1 suspicious records in \"%2\"?\n\n"
                "This will rescan suspicious titles and verify PDF ISBNs from extracted text before updating the database.")
            .arg(targets.size())
            .arg(m_currentLibraryName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    if (answer != QMessageBox::Yes)
        return;

    int repairedCount = 0;
    int titleCount = 0;
    int authorCount = 0;
    int publisherCount = 0;
    int yearCount = 0;
    int isbnClearedCount = 0;
    int isbnReplacedCount = 0;
    QJsonArray items;
    QStringList previewLines;

    for (int index = 0; index < targets.size(); ++index) {
        const Book& original = targets.at(index);
        showTaskProgress("Repairing Database Text",
                         QString("Repairing %1/%2 records").arg(index + 1).arg(targets.size()),
                         index + 1,
                         targets.size(),
                         original.displayTitle());

        Book repaired = original;
        QStringList changes;

        const RenameResult analysis = SmartRenamer::analyseBook(original);
        if (!analysis.newTitle.trimmed().isEmpty() && analysis.newTitle != original.title) {
            repaired.title = analysis.newTitle.trimmed();
            ++titleCount;
            changes << "title";
        }
        if (!analysis.newAuthor.trimmed().isEmpty() && analysis.newAuthor != original.author) {
            repaired.author = analysis.newAuthor.trimmed();
            ++authorCount;
            changes << "author";
        }
        if (!analysis.newPublisher.trimmed().isEmpty() && analysis.newPublisher != original.publisher) {
            repaired.publisher = analysis.newPublisher.trimmed();
            ++publisherCount;
            changes << "publisher";
        }
        if (analysis.newYear > 0 && analysis.newYear != original.year) {
            repaired.year = analysis.newYear;
            ++yearCount;
            changes << "year";
        }

        if (!original.isbn.trimmed().isEmpty()) {
            const QString verifiedIsbn = verifiedContentIsbn(original);
            if (!verifiedIsbn.isEmpty() && verifiedIsbn != original.isbn) {
                repaired.isbn = verifiedIsbn;
                ++isbnReplacedCount;
                changes << "isbn replaced";
            } else if (verifiedIsbn.isEmpty()
                       && original.format.compare("PDF", Qt::CaseInsensitive) == 0
                       && !suspiciousTextReasons(original).isEmpty()) {
                repaired.isbn.clear();
                ++isbnClearedCount;
                changes << "isbn cleared";
            }
        }

        if (changes.isEmpty())
            continue;

        Database::instance().updateBook(repaired);
        m_model->updateBook(repaired);
        ++repairedCount;

        QJsonObject item;
        item["id"] = QString::number(repaired.id);
        item["file_path"] = repaired.filePath;
        item["before_title"] = original.title;
        item["after_title"] = repaired.title;
        item["before_author"] = original.author;
        item["after_author"] = repaired.author;
        item["before_publisher"] = original.publisher;
        item["after_publisher"] = repaired.publisher;
        item["before_isbn"] = original.isbn;
        item["after_isbn"] = repaired.isbn;
        item["changes"] = QJsonArray::fromStringList(changes);
        items.append(item);

        if (previewLines.size() < 10)
            previewLines << QString("%1\n  %2").arg(repaired.displayTitle(), changes.join(", "));
    }

    reloadCurrentLibrary();
    rebuildSmartCategorySidebar();
    refreshCategoryOptions();
    updateStatusCount();

    QJsonObject report;
    report["feature"] = "repair_database_text";
    report["active_library"] = m_currentLibraryName;
    report["checked_records"] = books.size();
    report["targeted_records"] = targets.size();
    report["repaired_records"] = repairedCount;
    report["title_updates"] = titleCount;
    report["author_updates"] = authorCount;
    report["publisher_updates"] = publisherCount;
    report["year_updates"] = yearCount;
    report["isbn_cleared"] = isbnClearedCount;
    report["isbn_replaced"] = isbnReplacedCount;
    report["items"] = items;
    const QString reportPath = saveJsonArtifact("database-text-repair", report);

    hideTaskProgress(QString("Database text repair complete. %1 records updated.").arg(repairedCount));

    QString message = QString("Checked %1 records in \"%2\".\n\n"
                              "Suspicious records targeted: %3\n"
                              "Records repaired: %4\n"
                              "Titles updated: %5\n"
                              "Authors updated: %6\n"
                              "Publishers updated: %7\n"
                              "Years updated: %8\n"
                              "ISBNs cleared: %9\n"
                              "ISBNs replaced: %10")
        .arg(books.size())
        .arg(m_currentLibraryName)
        .arg(targets.size())
        .arg(repairedCount)
        .arg(titleCount)
        .arg(authorCount)
        .arg(publisherCount)
        .arg(yearCount)
        .arg(isbnClearedCount)
        .arg(isbnReplacedCount);

    if (!previewLines.isEmpty())
        message += QString("\n\nExamples:\n%1").arg(previewLines.join("\n\n"));
    if (!reportPath.isEmpty())
        message += QString("\n\nJSON report: %1").arg(QDir::toNativeSeparators(reportPath));

    QMessageBox box(this);
    box.setWindowTitle("Repair Text Problems");
    box.setText(message);
    box.setStyleSheet(styleSheet());
    box.exec();
}

void MainWindow::openExternalToolsDialog()
{
    ExternalToolsDialog dlg(this);
    dlg.setStyleSheet(styleSheet());
    dlg.exec();
}

void MainWindow::openDatabaseFolder()
{
    const QString dir = QFileInfo(AppConfig::dbPath()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void MainWindow::openDatabaseEditor()
{
    DatabaseEditorDialog dlg(this);
    dlg.setStyleSheet(styleSheet());
    if (dlg.exec() == QDialog::Accepted) {
        reloadCurrentLibrary();
        rebuildSmartCategorySidebar();
        refreshCategoryOptions();
        updateStatusCount();
        m_statusLabel->setText("Database changes saved.");
    }
}

void MainWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(trl("action.about", "About Eagle Library"));
    box.setText(
        "<h2 style='color:#c8aa50;'>Eagle Library</h2>"
        "<p style='color:#aaa;'>Version " + AppConfig::version() + "</p>"
        "<p>Copyright &copy; 2026 Eagle Software. All rights reserved.</p>"
        "<p>Professional eBook library manager. Manage thousands of books "
        "across any folder structure, fetch metadata from the internet, "
        "auto-download cover art, and build your personal reading collection.</p>"
        "<p><a href='https://eaglesoftware.biz/' style='color:#4a90d0;'>"
        "https://eaglesoftware.biz/</a></p>");
    box.setStyleSheet(styleSheet());
    box.exec();
}

void MainWindow::updateStatusCount()
{
    int total = m_model->rowCount();
    int shown = m_filterModel->rowCount();
    if (total == shown)
        m_libraryStatsLabel->setText(trl("status.itemsInLibrary", "%1 items in library").arg(total));
    else
        m_libraryStatsLabel->setText(trl("status.showingItems", "Showing %1 of %2 items").arg(shown).arg(total));
    if (m_sidebarStatsLabel)
        m_sidebarStatsLabel->setText(trl("status.itemsInLibrary", "%1 items in library").arg(total));
    updateWorkspaceHeader();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    m_isClosing = true;
    if (m_libraryChangeTimer)
        m_libraryChangeTimer->stop();
    if (m_libraryWatcher)
        m_libraryWatcher->blockSignals(true);
    if (m_scanner)
        disconnect(m_scanner, nullptr, this, nullptr);
    if (m_metaFetcher)
        disconnect(m_metaFetcher, nullptr, this, nullptr);
    if (m_coverDownloader)
        disconnect(m_coverDownloader, nullptr, this, nullptr);
    disconnect(&PluginManager::instance(), nullptr, this, nullptr);

    stopAllTasks();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    PluginManager::instance().unloadAll();
    saveSettings();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    applyResponsiveLayout();
}

void MainWindow::loadSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    m_libraryProfiles = loadLibraryProfiles(s);
    m_currentLibraryName = s.value("library/currentName").toString().trimmed();
    if (m_currentLibraryName.isEmpty() || !m_libraryProfiles.contains(m_currentLibraryName)) {
        const QStringList keys = m_libraryProfiles.keys();
        m_currentLibraryName = keys.isEmpty() ? QStringLiteral("Main Library") : keys.first();
    }
    m_watchFolders = foldersForLibrary(m_libraryProfiles, m_currentLibraryName);
    m_showSidebarPreference = s.value("view/showSidebar", true).toBool();
    m_showSmartCategories = s.value("view/showSmartCategories", false).toBool();
    m_autoEnrichAfterScan = s.value("options/autoEnrichAfterScan", true).toBool();
    m_fastScanMode = s.value("options/fastScanMode", true).toBool();
    m_compactMode = s.value("view/compactMode", false).toBool();
    m_rememberWindowState = s.value("view/rememberWindowState", true).toBool();
    m_scanThreads = s.value("options/scanThreads", 0).toInt();
    m_startViewMode = s.value("view/startMode", "remember").toString();
    m_isGridView = (m_startViewMode == "grid")
        ? true
        : (m_startViewMode == "list" ? false : s.value("view/isGrid", true).toBool());
    m_activeShelfName = normalizeShelfId(s.value("library/activeShelf", "all").toString());
    if (m_isGridView) setGridView(); else setListView();
    if (m_rememberWindowState && s.contains("window/geometry")) restoreGeometry(s.value("window/geometry").toByteArray());
    if (m_rememberWindowState && s.contains("window/state"))    restoreState(s.value("window/state").toByteArray());
    refreshLibrarySelector();
    refreshShelfOptions();
    applyResponsiveLayout();
    updateWorkspaceHeader();
}

void MainWindow::saveSettings()
{
    QSettings s("Eagle Software", "Eagle Library");
    s.setValue("view/isGrid", m_isGridView);
    s.setValue("library/currentName", m_currentLibraryName);
    s.setValue("library/activeShelf", m_activeShelfName);
    s.setValue("library/profiles", QJsonDocument(m_libraryProfiles).toJson(QJsonDocument::Compact));
    s.setValue("library/folders", m_watchFolders);
    s.setValue("view/showSidebar", m_showSidebarPreference);
    s.setValue("view/showSmartCategories", m_showSmartCategories);
    s.setValue("options/autoEnrichAfterScan", m_autoEnrichAfterScan);
    s.setValue("options/fastScanMode", m_fastScanMode);
    s.setValue("view/compactMode", m_compactMode);
    s.setValue("view/rememberWindowState", m_rememberWindowState);
    s.setValue("view/startMode", m_startViewMode);
    s.setValue("options/scanThreads", m_scanThreads);
    if (m_rememberWindowState) {
        s.setValue("window/geometry", saveGeometry());
        s.setValue("window/state", saveState());
    } else {
        s.remove("window/geometry");
        s.remove("window/state");
    }
}

QListView* MainWindow::currentView() const { return m_isGridView ? m_gridView : m_listView; }

void MainWindow::updateWorkspaceHeader()
{
    if (!m_workspaceTitleLabel || !m_workspaceMetaLabel)
        return;

    const int total = m_model ? m_model->rowCount() : 0;
    const int shown = m_filterModel ? m_filterModel->rowCount() : total;
    const int selected = currentView() && currentView()->selectionModel()
        ? currentView()->selectionModel()->selectedIndexes().size()
        : 0;

    m_workspaceTitleLabel->setText(selected > 0
        ? trl("workspace.itemsSelected", "%1 items selected").arg(selected)
        : trl("workspace.libraryTitle", "%1 Library").arg(m_currentLibraryName.isEmpty() ? QStringLiteral("Main") : m_currentLibraryName));
    m_workspaceMetaLabel->setText(
        trl("workspace.meta", "Showing %1 of %2 items across %3 watched folders in the %4 library.")
            .arg(shown)
            .arg(total)
            .arg(m_watchFolders.size())
            .arg(m_currentLibraryName));

    const QString viewName = m_isGridView ? trl("view.grid", "Grid") : trl("view.list", "List");
    const QString density = m_compactMode ? trl("view.compact", "Compact") : trl("view.comfort", "Comfort");
    m_workspaceLibraryChip->setText(trl("workspace.libraryChip", "Library  %1 | %2 items").arg(m_currentLibraryName, QString::number(total)));
    m_workspaceViewChip->setText(trl("workspace.viewChip", "View  %1 | %2 | %3").arg(viewName, density, shelfLabel(m_activeShelfName.isEmpty() ? QStringLiteral("all") : m_activeShelfName)));
    m_workspaceActionChip->setText(selected > 0
        ? trl("workspace.selectionChip", "Selection  %1 items").arg(selected)
        : trl("workspace.tipChip", "Tip  Double-click opens the file"));

    const QString contextualHint = selected > 0
        ? trl("workspace.tipSelection", "Run tools on the current selection, or choose All Books at the next prompt to process the whole library.")
        : (!m_watchFolders.isEmpty()
            ? trl("workspace.tipCatalog", "Use search, filters, sorting, and the right-click menu to focus the catalog quickly.")
            : trl("workspace.tipSettings", "Open Settings and add one or more watched folders before scanning the library."));

    m_workspaceHeader->setToolTip(contextualHint);
    m_workspaceTitleLabel->setToolTip(contextualHint);
    m_workspaceMetaLabel->setToolTip(contextualHint);
    m_workspaceLibraryChip->setToolTip(trl("workspace.tooltip.library", "Current library size and filter coverage"));
    m_workspaceViewChip->setToolTip(trl("workspace.tooltip.view", "Current presentation mode and density"));
    m_workspaceActionChip->setToolTip(contextualHint);

    if (selected > 0) {
        m_workspaceHintLabel->setToolTip(contextualHint);
    } else if (!m_watchFolders.isEmpty()) {
        m_workspaceHintLabel->setToolTip(contextualHint);
    } else {
        m_workspaceHintLabel->setToolTip(contextualHint);
    }
}

void MainWindow::refreshCategoryOptions()
{
    if (!m_categoryCombo)
        return;

    const QString current = m_categoryCombo->currentData().toString().isEmpty()
        ? m_categoryCombo->currentText()
        : m_categoryCombo->currentData().toString();
    QSignalBlocker blocker(m_categoryCombo);
    m_categoryCombo->clear();
    m_categoryCombo->addItem(trl("category.all", "All Categories"), QString());
    m_categoryCombo->addItem(trl("category.book", "Book"), QStringLiteral("Book"));
    m_categoryCombo->addItem(trl("category.document", "Document"), QStringLiteral("Document"));

    QSet<QString> categorySet;
    for (const Book& book : currentLibraryBooks()) {
        for (const QString& tag : book.tags)
            if (!tag.trimmed().isEmpty())
                categorySet.insert(tag.trimmed());
    }
    QStringList categories(categorySet.begin(), categorySet.end());
    categories.removeDuplicates();
    categories.sort(Qt::CaseInsensitive);
    for (const QString& category : categories) {
        if (category.compare("Book", Qt::CaseInsensitive) != 0
            && category.compare("Document", Qt::CaseInsensitive) != 0) {
            m_categoryCombo->addItem(category, category);
        }
    }

    const int idx = m_categoryCombo->findData(current);
    m_categoryCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::refreshLibrarySelector()
{
    if (!m_libraryCombo)
        return;
    QSignalBlocker blocker(m_libraryCombo);
    m_libraryCombo->clear();
    QStringList names = m_libraryProfiles.keys();
    names.sort(Qt::CaseInsensitive);
    for (const QString& name : names)
        m_libraryCombo->addItem(name);
    const int idx = m_libraryCombo->findText(m_currentLibraryName);
    m_libraryCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::refreshShelfOptions()
{
    if (!m_shelfCombo)
        return;
    const QString current = normalizeShelfId(m_activeShelfName.isEmpty() ? QStringLiteral("all") : m_activeShelfName);
    QSignalBlocker blocker(m_shelfCombo);
    m_shelfCombo->clear();
    const QList<QPair<QString, QString>> shelves = {
        {trl("shelf.allItems", "All Items"), QStringLiteral("all")},
        {trl("shelf.booksOnly", "Books Only"), QStringLiteral("books")},
        {trl("shelf.documentsOnly", "Documents Only"), QStringLiteral("documents")},
        {trl("shelf.favourites", "Favourites"), QStringLiteral("favourites")},
        {trl("shelf.recentlyAdded", "Recently Added"), QStringLiteral("recent")},
        {trl("shelf.mostOpened", "Most Opened"), QStringLiteral("opened")},
        {trl("shelf.missingMetadata", "Missing Metadata"), QStringLiteral("missing_metadata")},
        {trl("shelf.noCover", "No Cover"), QStringLiteral("no_cover")}
    };
    for (const auto& shelf : shelves)
        m_shelfCombo->addItem(shelf.first, shelf.second);
    const int idx = m_shelfCombo->findData(current);
    m_shelfCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::reloadCurrentLibrary()
{
    BookFilter filter;
    if (!m_watchFolders.isEmpty()) {
        filter.restrictToPathPrefixes = true;
        filter.pathPrefixes = m_watchFolders;
    }
    const SortField sortField = m_sortCombo ? static_cast<SortField>(m_sortCombo->currentData().toInt()) : SortField::Title;
    m_model->reload(filter, sortField, m_sortOrder);
    refreshCategoryOptions();
    rebuildSmartCategorySidebar();
    rebuildSavedSearchSidebar();
    applyShelf(normalizeShelfId(m_activeShelfName.isEmpty() ? QStringLiteral("all") : m_activeShelfName));
    refreshLibraryWatcher();
}

void MainWindow::refreshLibraryWatcher()
{
    if (!m_libraryWatcher)
        return;

    const QStringList current = m_libraryWatcher->directories();
    if (!current.isEmpty())
        m_libraryWatcher->removePaths(current);

    QStringList watchable;
    for (const QString& folder : m_watchFolders) {
        const QString normalized = QDir::fromNativeSeparators(folder).trimmed();
        if (!normalized.isEmpty() && QFileInfo::exists(normalized))
            watchable << normalized;
    }
    if (!watchable.isEmpty())
        m_libraryWatcher->addPaths(watchable);
}

void MainWindow::scheduleIncrementalScan(const QString& changedPath)
{
    if (!changedPath.trimmed().isEmpty())
        m_statusLabel->setText(QString("Detected library change: %1").arg(QDir::toNativeSeparators(changedPath)));
    m_incrementalScanPending = true;
    if (m_libraryChangeTimer)
        m_libraryChangeTimer->start();
}

QVector<Book> MainWindow::currentLibraryBooks(SortField sort, SortOrder order) const
{
    BookFilter filter;
    filter.restrictToPathPrefixes = true;
    filter.pathPrefixes = m_watchFolders;
    return Database::instance().query(filter, sort, order);
}

void MainWindow::switchLibrary(const QString& libraryName)
{
    const QString trimmed = libraryName.trimmed();
    if (trimmed.isEmpty() || trimmed == m_currentLibraryName)
        return;

    m_currentLibraryName = trimmed;
    m_watchFolders = foldersForLibrary(m_libraryProfiles, m_currentLibraryName);
    reloadCurrentLibrary();
    updateStatusCount();
    m_statusLabel->setText(QString("Active library: %1").arg(m_currentLibraryName));
}

void MainWindow::applyShelf(const QString& shelfName)
{
    m_activeShelfName = normalizeShelfId(shelfName);

    m_filterModel->clearFilters();
    if (m_searchBox)
        m_searchBox->clear();
    if (m_favAction)
        m_favAction->setChecked(false);
    if (m_formatCombo)
        m_formatCombo->setCurrentText("All");
    if (m_categoryCombo)
        m_categoryCombo->setCurrentIndex(m_categoryCombo->findData(QString()));

    if (m_activeShelfName == QStringLiteral("books")) {
        m_filterModel->setFilterCategory("Book");
        if (m_categoryCombo)
            m_categoryCombo->setCurrentIndex(m_categoryCombo->findData(QStringLiteral("Book")));
    } else if (m_activeShelfName == QStringLiteral("documents")) {
        m_filterModel->setFilterCategory("Document");
        if (m_categoryCombo)
            m_categoryCombo->setCurrentIndex(m_categoryCombo->findData(QStringLiteral("Document")));
    } else if (m_activeShelfName == QStringLiteral("favourites")) {
        m_filterModel->setFilterFavourites(true);
        if (m_favAction)
            m_favAction->setChecked(true);
    } else if (m_activeShelfName == QStringLiteral("recent")) {
        m_filterModel->sortBy(SortField::DateAdded, SortOrder::Desc);
    } else if (m_activeShelfName == QStringLiteral("opened")) {
        m_filterModel->sortBy(SortField::OpenCount, SortOrder::Desc);
    } else if (m_activeShelfName == QStringLiteral("missing_metadata")) {
        m_filterModel->setFilterNoMeta(true);
    } else if (m_activeShelfName == QStringLiteral("no_cover")) {
        m_filterModel->setFilterNoCover(true);
    }

    if (m_sidebarContent) {
        const auto buttons = m_sidebarContent->findChildren<QPushButton*>("sidebarButton");
        for (QPushButton* button : buttons)
            button->setChecked(button->property("shelfId").toString() == m_activeShelfName);
    }

    updateStatusCount();
}

void MainWindow::showReferenceLookup(const Book& book)
{
    ReferenceLookupDialog dlg(book, this);
    dlg.setStyleSheet(styleSheet());
    dlg.exec();
}

void MainWindow::applyResponsiveLayout()
{
    const int widthPx = width();
    const bool narrow = widthPx < 1180;
    const bool compactWindow = widthPx < 920;

    if (m_sidebarDock) {
        const bool showSidebarNow = m_showSidebarPreference && !compactWindow;
        m_sidebarDock->setVisible(showSidebarNow);
        if (showSidebarNow) {
            const int dockWidth = narrow ? 184 : (widthPx > 1600 ? 236 : 208);
            m_sidebarDock->setMinimumWidth(dockWidth);
            m_sidebarDock->setMaximumWidth(dockWidth + 10);
        }
    }

    if (m_searchBox) {
        m_searchBox->setMinimumWidth(compactWindow ? 150 : (narrow ? 190 : 240));
        m_searchBox->setMaximumWidth(compactWindow ? 210 : (narrow ? 260 : 360));
    }
    if (m_libraryCombo)
        m_libraryCombo->setMinimumWidth(compactWindow ? 120 : 170);
    if (m_shelfCombo)
        m_shelfCombo->setMinimumWidth(compactWindow ? 120 : 160);
    if (m_formatCombo)
        m_formatCombo->setMinimumWidth(compactWindow ? 78 : 96);
    if (m_categoryCombo)
        m_categoryCombo->setMinimumWidth(compactWindow ? 110 : 140);
    if (m_sortCombo)
        m_sortCombo->setMinimumWidth(compactWindow ? 96 : 118);
    if (m_scanLabel)
        m_scanLabel->setVisible(!compactWindow);
    if (m_workspaceHintLabel)
        m_workspaceHintLabel->setVisible(false);

    const QSettings s("Eagle Software", "Eagle Library");
    int cardWidth = qBound(120, s.value("options/iconSize", 160).toInt(), 280);
    if (m_compactMode)
        cardWidth -= 14;
    if (compactWindow)
        cardWidth = qMin(cardWidth, 132);
    else if (narrow)
        cardWidth = qMin(cardWidth, 150);
    else if (widthPx > 1700)
        cardWidth = qMin(220, cardWidth + 16);

    const int cardHeight = cardWidth + (m_compactMode ? 78 : 94);
    const int coverHeight = qRound(cardHeight * 0.72);
    const int listRowHeight = m_compactMode || compactWindow ? 56 : 66;
    const int padding = m_compactMode ? 7 : 9;
    const int spacing = m_compactMode ? 8 : 12;

    if (m_gridDelegate)
        m_gridDelegate->setMetrics(cardWidth, cardHeight, coverHeight, listRowHeight, padding);
    if (m_listDelegate)
        m_listDelegate->setMetrics(cardWidth, cardHeight, coverHeight, listRowHeight, padding);
    if (m_gridView) {
        m_gridView->setGridSize(QSize(cardWidth + 20, cardHeight + 18));
        m_gridView->setSpacing(spacing);
    }
    if (m_listView)
        m_listView->setSpacing(m_compactMode ? 4 : 6);
}

void MainWindow::rebuildSmartCategorySidebar()
{
    if (!m_smartCategorySection || !m_smartCategoryButtonsLayout)
        return;

    refreshCategoryOptions();

    while (QLayoutItem* item = m_smartCategoryButtonsLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            delete widget;
        delete item;
    }

    m_smartCategorySection->setVisible(m_showSmartCategories);
    if (!m_showSmartCategories)
        return;

    QMap<QString, int> tagCounts;
    const QVector<Book> books = currentLibraryBooks();
    for (const Book& book : books) {
        for (const QString& tag : book.tags)
            tagCounts[tag] += 1;
    }

    QList<QPair<QString, int>> ranked;
    for (auto it = tagCounts.cbegin(); it != tagCounts.cend(); ++it) {
        if (it.value() >= 2)
            ranked.append({it.key(), it.value()});
    }
    std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second)
            return a.second > b.second;
        return a.first < b.first;
    });

    const int maxButtons = qMin(8, ranked.size());
    for (int i = 0; i < maxButtons; ++i) {
        const QString tag = ranked.at(i).first;
        auto* btn = new QPushButton(QString("%1 (%2)").arg(tag).arg(ranked.at(i).second), m_smartCategorySection);
        btn->setObjectName("sidebarButton");
        btn->setFlat(true);
        connect(btn, &QPushButton::clicked, this, [this, tag]() {
            m_filterModel->clearFilters();
            m_filterModel->setFilterTag(tag);
            updateStatusCount();
        });
        m_smartCategoryButtonsLayout->addWidget(btn);
    }

    if (maxButtons == 0) {
        auto* label = new QLabel("Run Smart Categorize to build category shortcuts.", m_smartCategorySection);
        label->setObjectName("statsLabel");
        label->setWordWrap(true);
        m_smartCategoryButtonsLayout->addWidget(label);
    }
}

void MainWindow::rebuildSavedSearchSidebar()
{
    if (!m_savedSearchSection || !m_savedSearchButtonsLayout)
        return;

    while (QLayoutItem* item = m_savedSearchButtonsLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            delete widget;
        delete item;
    }

    const QVector<SavedSearch> searches = Database::instance().savedSearches();
    m_savedSearchSection->setVisible(!searches.isEmpty());
    if (searches.isEmpty())
        return;

    for (const SavedSearch& search : searches) {
        auto* btn = new QPushButton(search.name, m_savedSearchSection);
        btn->setObjectName("sidebarButton");
        btn->setFlat(true);
        btn->setToolTip(search.query);
        connect(btn, &QPushButton::clicked, this, [this, id = search.id]() {
            loadSavedSearch(id);
        });
        m_savedSearchButtonsLayout->addWidget(btn);
    }
}

bool MainWindow::bookNeedsEnrichment(const Book& book) const
{
    const bool missingCoreMeta = book.title.trimmed().isEmpty()
        || book.author.trimmed().isEmpty()
        || book.publisher.trimmed().isEmpty()
        || book.year <= 0
        || book.description.trimmed().isEmpty()
        || book.language.trimmed().isEmpty();
    const bool missingDiscovery = book.isbn.trimmed().isEmpty()
        || (!book.hasCover || book.coverPath.trimmed().isEmpty())
        || book.tags.isEmpty();
    return missingCoreMeta || missingDiscovery;
}

void MainWindow::enrichIncompleteBooks(const QVector<Book>& books, const QString& title)
{
    QVector<Book> targets;
    for (const Book& book : books) {
        if (bookNeedsEnrichment(book))
            targets << book;
    }

    if (targets.isEmpty()) {
        m_statusLabel->setText("No incomplete books need enrichment.");
        return;
    }

    int isbnUpdated = 0;
    int categorized = 0;
    int metadataQueued = 0;

    int index = 0;
    for (Book book : targets) {
        ++index;
        showTaskProgress(title,
                         QString("Checking %1/%2 books").arg(index).arg(targets.size()),
                         index,
                         targets.size(),
                         book.displayTitle());

        bool changed = false;
        if (book.isbn.trimmed().isEmpty()) {
            const QString extracted = sanitizeIsbn(extractIsbnFromBookContent(book));
            if (!extracted.isEmpty() && (isValidIsbn10(extracted) || isValidIsbn13(extracted))) {
                book.isbn = extracted;
                ++isbnUpdated;
                changed = true;
            }
        }

        if (book.tags.isEmpty()) {
            const QStringList inferredTags = inferSmartTags(book);
            if (!inferredTags.isEmpty()) {
                book.tags = uniqueCaseInsensitive(book.tags + inferredTags);
                if (book.subjects.isEmpty())
                    book.subjects = book.tags.mid(0, qMin(3, book.tags.size()));
                ++categorized;
                changed = true;
            }
        }

        if (changed) {
            Database::instance().updateBook(book);
            m_model->updateBook(book);
        }

        if (book.title.trimmed().isEmpty() || book.author.trimmed().isEmpty() || book.publisher.trimmed().isEmpty()
            || book.year <= 0 || book.description.trimmed().isEmpty() || book.language.trimmed().isEmpty()
            || !book.hasCover || book.coverPath.trimmed().isEmpty()) {
            scheduleMetadataFetch(book, true);
            ++metadataQueued;
        }
    }

    rebuildSmartCategorySidebar();
    hideTaskProgress(QString("%1 complete. ISBN updated: %2, categorized: %3, metadata queued: %4.")
                         .arg(title)
                         .arg(isbnUpdated)
                         .arg(categorized)
                         .arg(metadataQueued));
}

void MainWindow::applyStyles()
{
    qApp->setStyle("Fusion");
    // Theme is applied by ThemeManager::applyTheme(). Keep a global fallback only
    // for first-launch/bootstrap before the real theme file is loaded.
    if (!qApp->styleSheet().isEmpty())
        return;
    qApp->setStyleSheet(R"(
QMainWindow, QWidget {
    background-color: #09111f;
    color: #e7dcc0;
    font-size: 12px;
}
QMenuBar {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #050c17, stop:0.35 #0b1524, stop:1 #09111f);
    color: #d7c48d;
    border-bottom: 1px solid #264e74;
    padding: 5px 8px;
}
QMenuBar::item {
    padding: 7px 12px;
    border-radius: 7px;
    margin: 1px 2px;
    background: transparent;
}
QMenuBar::item:selected { background: rgba(37, 85, 127, 0.78); color: #fff0ba; }
QMenu { background: #0d1b2d; color: #e7dcc0; border: 1px solid #29527b; padding: 6px; }
QMenu::item { padding: 8px 16px; border-radius: 6px; }
QMenu::item:selected { background: #20486f; color: #fff7db; }
QMenu::separator { background: #244565; height: 1px; margin: 6px 0; }
QToolBar#mainToolBar {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #14304c, stop:0.38 #10233b, stop:0.72 #0f1d31, stop:1 #1b3d5d);
    border-top: 1px solid rgba(255, 239, 183, 0.08);
    border-bottom: 1px solid #2c5e87;
    padding: 10px 12px;
    spacing: 9px;
}
QWidget#toolbarBrand {
    background: rgba(5, 12, 23, 0.34);
    border: 1px solid rgba(120, 178, 225, 0.22);
    border-radius: 12px;
}
QLabel#toolbarBrandText {
    color: #fff1bf;
    font-weight: 800;
    letter-spacing: 0.6px;
}
QWidget#workspaceHeader {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #183854, stop:0.28 #11263e, stop:0.72 #102138, stop:1 #1a4567);
    border-bottom: 1px solid #2b5d85;
}
QLabel#workspaceTitle {
    color: #fff2c7;
    font-size: 23px;
    font-weight: 800;
    letter-spacing: 0.5px;
}
QLabel#workspaceMeta {
    color: #b6cbe0;
    font-size: 12px;
}
QLabel#workspaceHint {
    color: #d7c690;
    background: rgba(7, 16, 29, 0.26);
    border: 1px solid rgba(240, 216, 142, 0.16);
    border-radius: 12px;
    padding: 10px 12px;
}
QLabel#workspaceChip {
    color: #f7e7b0;
    background: rgba(8, 18, 31, 0.34);
    border: 1px solid rgba(90, 136, 182, 0.35);
    border-radius: 10px;
    padding: 6px 10px;
    font-size: 11px;
    font-weight: 600;
}
QToolBar QToolButton {
    color: #ecd69b;
    background: rgba(255,255,255,0.03);
    border: 1px solid rgba(255,255,255,0.05);
    border-radius: 10px;
    padding: 8px 13px;
    font-size: 12px;
    font-weight: 700;
}
QToolBar QToolButton:hover { background: rgba(32, 76, 115, 0.95); border-color: #5e97c7; color: #fff2be; }
QToolBar QToolButton:pressed { background: #13304a; }
QToolBar QToolButton:checked { background: #25557f; border-color: #78b2e1; color: #fff8e3; }
QToolBar::separator { background: #22415f; width: 1px; margin: 6px 6px; }
QToolBar QLabel { color: #7f9cbc; padding: 0 4px; font-weight: 600; }
QLineEdit {
    background: #122134;
    border: 1px solid #2b5379;
    border-radius: 8px;
    color: #f1e5ca;
    padding: 7px 12px;
    selection-background-color: #315d88;
}
QLineEdit:focus { border-color: #d1a84f; background: #15263d; }
QComboBox {
    background: #122134;
    border: 1px solid #2b5379;
    border-radius: 8px;
    color: #f1e5ca;
    padding: 6px 10px;
}
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView {
    background: #0d1b2d;
    border: 1px solid #234468;
    color: #f1e5ca;
    selection-background-color: #244d75;
}
QDockWidget { color: #8fa7c2; font-weight: bold; font-size: 11px; }
QDockWidget::title { background: #0a1524; border-bottom: 1px solid #1f3c58; padding: 8px 10px; }
QDockWidget > QWidget { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #0b1625, stop:1 #08111d); }
QPushButton#navBtn {
    background: transparent;
    color: #94a9c2;
    border: 1px solid transparent;
    border-radius: 9px;
    text-align: left;
    padding: 9px 12px;
    font-size: 12px;
    font-weight: 600;
}
QPushButton#navBtn:hover { background: #14304b; border-color: #234c70; color: #f0d88e; }
QLabel#sideHeader { color: #59779a; font-size: 10px; font-weight: bold; padding: 12px 12px 4px 12px; letter-spacing: 1px; }
QLabel#statsLabel { color: #6683a2; font-size: 10px; padding: 12px 10px; border-top: 1px solid #1b3650; }
QListView {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0b1627, stop:1 #0f1d31);
    border: none;
    outline: none;
    padding: 10px;
}
QListView::item:selected { background: transparent; outline: none; }
QScrollBar:vertical { background: #08111d; width: 10px; border-radius: 5px; margin: 4px; }
QScrollBar::handle:vertical { background: #285378; border-radius: 5px; min-height: 36px; }
QScrollBar::handle:vertical:hover { background: #356d9b; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: #08111d; height: 10px; border-radius: 5px; margin: 4px; }
QScrollBar::handle:horizontal { background: #285378; border-radius: 5px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QStatusBar {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #07101d, stop:1 #0c1828);
    color: #8ea7c1;
    border-top: 1px solid #244866;
    font-size: 11px;
}
QStatusBar::item { border: none; }
QLabel#statusPrimary {
    color: #f4e7bf;
    font-weight: 700;
    padding: 0 8px;
}
QLabel#statusSecondary {
    color: #7fa0c0;
    padding: 0 8px;
}
QLabel#statusPill {
    color: #e9d69b;
    background: rgba(29, 57, 85, 0.66);
    border: 1px solid rgba(120, 178, 225, 0.24);
    border-radius: 9px;
    padding: 4px 10px;
    font-weight: 700;
}
QProgressBar#statusProgress {
    background: #122134;
    border: 1px solid #2b5379;
    border-radius: 6px;
    color: #f7ecce;
    height: 16px;
    padding: 0 2px;
}
QProgressBar#statusProgress::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #2c5f92, stop:0.55 #3e83b8, stop:1 #d2a449);
    border-radius: 5px;
}
QToolTip {
    background: #10233b;
    color: #f5e8c3;
    border: 1px solid #4b83b3;
    padding: 6px 8px;
}
QMessageBox { background: #0e1b2c; color: #e7dcc0; }
QMessageBox QLabel { color: #e7dcc0; }
QPushButton {
    background-color: #1b4062;
    color: #ecd48d;
    border: 1px solid #2f628d;
    border-radius: 8px;
    padding: 8px 16px;
    font-weight: 600;
}
QPushButton:hover { background-color: #25557f; }
QPushButton:pressed { background-color: #16364f; }
)");
}

// ── Command Palette setup ─────────────────────────────────────
void MainWindow::setupCommandPalette()
{
    m_commandPalette = new CommandPalette(this);

    // Register all application commands
    const QList<Command> cmds = {
        { "scan",         "Scan Folders",              "Scan watched folders for new files", "F5",       "Library",  [this]{ onScanStart(); } },
        { "refresh",      "Refresh View",              "Reload the current view",            "",         "Library",  [this]{ refreshLibrary(); } },
        { "fetch_meta",   "Fetch All Metadata",        "Download metadata for all books",    "",         "Library",  [this]{ fetchAllMetadata(); } },
        { "enrich",       "Enrich Incomplete Books",   "Fill in missing metadata",           "",         "Library",  [this]{ enrichIncompleteBooksAction(); } },
        { "find_dupes",   "Find Duplicates",           "Detect duplicate files by hash",     "",         "Library",  [this]{ findDuplicates(); } },
        { "quality",      "Quality Check",             "Scan for encoding and meta issues",  "",         "Library",  [this]{ runQualityCheck(); } },
        { "smart_cat",    "Smart Categorize",          "Auto-classify books vs documents",   "",         "Library",  [this]{ smartCategorizeLibrary(); } },
        { "smart_rename", "Smart Rename All",          "Auto-rename from filename patterns", "Ctrl+R",   "Library",  [this]{ onSmartRenameAll(); } },
        { "collections",  "Manage Collections",        "Create virtual book collections",    "",         "Library",  [this]{ manageCollections(); } },
        { "google",       "Search on Google",          "Search selected book on Google",     "Ctrl+Shift+G", "Research", [this]{ searchSelectedOnGoogle(); } },
        { "adv_search",   "Advanced Search",           "Multi-field search dialog",          "Ctrl+Shift+F", "Search", [this]{ openAdvancedSearch(); } },
        { "save_search",  "Save Current Search",       "Save search query for quick access", "",         "Search",   [this]{ saveCurrentSearch(); } },
        { "grid_view",    "Grid View",                 "Show books as cover cards",          "Ctrl+G",   "View",     [this]{ setGridView(); } },
        { "list_view",    "List View",                 "Show books as compact list",         "Ctrl+L",   "View",     [this]{ setListView(); } },
        { "theme_blue",   "Theme: Blue Pro",           "Apply dark navy theme",              "",         "Appearance", [this]{ switchTheme("Blue Pro"); } },
        { "theme_white",  "Theme: Pure White",         "Apply light theme",                  "",         "Appearance", [this]{ switchTheme("Pure White"); } },
        { "theme_mac",    "Theme: macOS Style",        "Apply macOS-inspired theme",         "",         "Appearance", [this]{ switchTheme("macOS"); } },
        { "settings",     "Preferences",               "Open application preferences",       "Ctrl+,",   "App",      [this]{ openSettings(); } },
        { "plugins",      "Manage Plugins",            "View and manage installed plugins",  "",         "App",      [this]{ openPluginManager(); } },
        { "export",       "Export Library",            "Export library metadata snapshot",   "",         "App",      [this]{ exportLibrarySnapshot(); } },
        { "import",       "Import Library",            "Import library metadata snapshot",   "",         "App",      [this]{ importLibrarySnapshot(); } },
        { "db_summary",   "Database Summary",          "View database statistics",           "",         "Database", [this]{ consultDatabaseSummary(); } },
        { "db_remap",     "Remap Drive / Folder Paths","Update stored file paths after a drive-letter or root-folder change", "", "Database", [this]{ remapLibraryPaths(); } },
        { "db_optimize",  "Optimize Database",         "Run SQLite ANALYZE and optimize",    "",         "Database", [this]{ Database::instance().optimize(); m_statusLabel->setText("Optimization complete."); } },
        { "stop_tasks",   "Stop All Tasks",            "Cancel all running tasks",           "Ctrl+.",   "App",      [this]{ stopAllTasks(); } },
        { "about",        "About Eagle Library",       "Show version and credits",           "",         "Help",     [this]{ showAbout(); } },
        { "open_book",    "Open Selected Book",        "Open the selected book file",        "Return",   "Library",  [this]{ openBookFile(); } },
        { "book_detail",  "Edit Book Details",         "Open metadata editor for selection", "E",        "Library",  [this]{ openBookDetail(); } },
        { "toggle_fav",   "Toggle Favourite",          "Mark/unmark selected as favourite",  "F",        "Library",  [this]{
            QListView* view = currentView();
            if (!view || !view->selectionModel()) return;
            const auto sel = view->selectionModel()->selectedIndexes();
            if (sel.isEmpty()) return;
            const QModelIndex src = m_filterModel->mapToSource(sel.first());
            if (!src.isValid()) return;
            Book b = m_model->bookAt(src.row());
            b.isFavourite = !b.isFavourite;
            Database::instance().setFavourite(b.id, b.isFavourite);
            m_model->updateBook(b);
        }},
    };

    m_commandPalette->registerCommands(cmds);
}

// ── Theme ─────────────────────────────────────────────────────
void MainWindow::switchTheme(const QString& name)
{
    ThemeManager::instance().applyTheme(name);

    if (m_themeBlueAct)  m_themeBlueAct->setChecked(name == "Blue Pro");
    if (m_themeWhiteAct) m_themeWhiteAct->setChecked(name == "Pure White");
    if (m_themeMacAct)   m_themeMacAct->setChecked(name == "macOS");

    // Let Qt process the queued stylesheet-change events first so all widgets
    // pick up the new app-level QSS before we force a repolish.
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // Repolish every descendant so custom properties and palette-derived colours
    // are re-evaluated against the new stylesheet.
    const QList<QWidget*> widgets = findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        if (!widget)
            continue;
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
    style()->unpolish(this);
    style()->polish(this);
    update();
    repaint();
    m_statusLabel->setText("Theme: " + name);
}

// ── Command Palette ───────────────────────────────────────────
void MainWindow::openCommandPalette()
{
    if (m_commandPalette)
        m_commandPalette->popup();
}

// ── Web Research ──────────────────────────────────────────────
static Book selectedBookFromView(QListView* view, BookFilterModel* filterModel, BookModel* model)
{
    if (!view || !view->selectionModel()) return {};
    const auto sel = view->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return {};
    const QModelIndex src = filterModel->mapToSource(sel.first());
    if (!src.isValid()) return {};
    return model->bookAt(src.row());
}

void MainWindow::searchSelectedOnGoogle()
{
    searchSelectedOnWeb("google");
}

void MainWindow::searchSelectedOnWeb(const QString& engine)
{
    const Book b = selectedBookFromView(currentView(), m_filterModel, m_model);
    if (b.id == 0) {
        m_statusLabel->setText("Select a book first.");
        return;
    }

    QString query;
    if (!b.isbn.isEmpty())
        query = b.isbn;
    else if (!b.title.isEmpty() && !b.author.isEmpty())
        query = b.displayTitle() + " " + b.author;
    else
        query = b.displayTitle();

    query = QUrl::toPercentEncoding(query);

    QUrl url;
    if (engine == "goodreads")
        url = QUrl("https://www.goodreads.com/search?q=" + query);
    else
        url = QUrl("https://www.google.com/search?q=" + query + "+book");

    QDesktopServices::openUrl(url);
    m_statusLabel->setText("Opening browser for: " + b.displayTitle());
}

void MainWindow::lookupSelectedOnGoodreads()
{
    searchSelectedOnWeb("goodreads");
}

// ── Collections ───────────────────────────────────────────────
void MainWindow::manageCollections()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Manage Collections");
    dlg.setMinimumWidth(480);
    dlg.setMinimumHeight(360);

    auto* layout = new QVBoxLayout(&dlg);

    auto* header = new QLabel("Virtual Collections let you group books across any folders or formats.");
    header->setWordWrap(true);
    layout->addWidget(header);

    auto* table = new QTableWidget(&dlg);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Name", "Color", "Books"});
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    layout->addWidget(table);

    const auto loadCollections = [&]() {
        table->setRowCount(0);
        const auto collections = Database::instance().allCollections();
        table->setRowCount(collections.size());
        for (int i = 0; i < collections.size(); ++i) {
            table->setItem(i, 0, new QTableWidgetItem(collections[i].name));
            auto* colorItem = new QTableWidgetItem(collections[i].color);
            colorItem->setBackground(QColor(collections[i].color));
            table->setItem(i, 1, colorItem);
            table->setItem(i, 2, new QTableWidgetItem(QString::number(collections[i].bookCount)));
        }
    };
    loadCollections();

    auto* btnBar = new QHBoxLayout;
    auto* addBtn = new QPushButton("+ New Collection");
    auto* deleteBtn = new QPushButton("Delete Selected");
    auto* closeBtn = new QPushButton("Close");
    closeBtn->setProperty("primary", true);
    btnBar->addWidget(addBtn);
    btnBar->addWidget(deleteBtn);
    btnBar->addStretch();
    btnBar->addWidget(closeBtn);
    layout->addLayout(btnBar);

    connect(addBtn, &QPushButton::clicked, &dlg, [&loadCollections, &dlg, this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(&dlg, "New Collection", "Collection name:", QLineEdit::Normal, {}, &ok);
        if (ok && !name.trimmed().isEmpty()) {
            Database::instance().createCollection(name.trimmed());
            loadCollections();
            refreshCategoryOptions();
        }
    });

    connect(deleteBtn, &QPushButton::clicked, &dlg, [&, this]() {
        const int row = table->currentRow();
        if (row < 0) return;
        const auto collections = Database::instance().allCollections();
        if (row >= collections.size()) return;
        if (QMessageBox::question(&dlg, "Delete Collection",
                                  "Delete collection \"" + collections[row].name + "\"?\n"
                                  "Books will not be deleted.",
                                  QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
            return;
        Database::instance().deleteCollection(collections[row].id);
        loadCollections();
        refreshCategoryOptions();
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

void MainWindow::createCollection()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New Collection", "Collection name:", QLineEdit::Normal, {}, &ok);
    if (ok && !name.trimmed().isEmpty()) {
        Database::instance().createCollection(name.trimmed());
        refreshCategoryOptions();
        m_statusLabel->setText("Collection created: " + name);
    }
}

// ── Plugin Manager ────────────────────────────────────────────
void MainWindow::openPluginManager()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Plugin Manager — Eagle Library");
    dlg.setMinimumWidth(800);
    dlg.setMinimumHeight(580);
    dlg.setStyleSheet(styleSheet());

    auto* root = new QVBoxLayout(&dlg);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    // ── Header ────────────────────────────────────────────────
    auto* header = new QFrame;
    header->setStyleSheet(
        "QFrame{"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #0d1f32, stop:1 #162c42);"
        "  border-bottom: 1px solid #254060;"
        "}");
    auto* hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(26, 20, 26, 20);
    hbox->setSpacing(18);

    auto* hIcon = new QLabel("\xF0\x9F\x94\x8C");
    hIcon->setStyleSheet("font-size:38px;background:transparent;");

    auto* hCol = new QVBoxLayout;
    hCol->setSpacing(4);
    auto* hTitle = new QLabel("Plugin Manager");
    hTitle->setStyleSheet(
        "font-size:18px;font-weight:800;color:#f2dc7a;background:transparent;");
    auto* hSub = new QLabel(
        "Extend Eagle Library with plugins. Each plugin folder must contain a "
        "<span style='color:#7abcdc;font-family:monospace;'>plugin.json</span> manifest. "
        "Plugin actions appear in book and document detail panels.");
    hSub->setWordWrap(true);
    hSub->setTextFormat(Qt::RichText);
    hSub->setStyleSheet("font-size:12px;color:#7ea8c4;background:transparent;");
    hCol->addWidget(hTitle);
    hCol->addWidget(hSub);

    hbox->addWidget(hIcon);
    hbox->addLayout(hCol, 1);
    root->addWidget(header);

    // ── Path bar ──────────────────────────────────────────────
    auto* pathBar = new QFrame;
    pathBar->setStyleSheet(
        "QFrame{background:#0a1825;border-bottom:1px solid #1c3448;padding:0;}");
    auto* pbox = new QHBoxLayout(pathBar);
    pbox->setContentsMargins(26, 8, 18, 8);
    auto* pathLbl = new QLabel(
        QString("<span style='color:#3e6080;'>Plugins folder:</span>"
                "&nbsp;&nbsp;<code style='color:#5aa4cc;'>%1</code>")
            .arg(QDir::toNativeSeparators(AppConfig::pluginsDir())));
    pathLbl->setTextFormat(Qt::RichText);
    pathLbl->setStyleSheet("font-size:11px;background:transparent;");
    pbox->addWidget(pathLbl, 1);
    root->addWidget(pathBar);

    // ── Scrollable plugin list ─────────────────────────────────
    auto* scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea,QWidget{background:#09111f;}");

    auto* listWidget = new QWidget;
    listWidget->setStyleSheet("background:#09111f;");
    auto* listVBox = new QVBoxLayout(listWidget);
    listVBox->setContentsMargins(20, 18, 20, 18);
    listVBox->setSpacing(10);
    scroll->setWidget(listWidget);
    root->addWidget(scroll, 1);

    // ── Bottom action bar ──────────────────────────────────────
    auto* bar = new QFrame;
    bar->setStyleSheet(
        "QFrame{"
        "  background:#0c1b2c;"
        "  border-top:1px solid #1e3a52;"
        "}"
        "QPushButton{"
        "  border-radius:7px;font-weight:700;font-size:12px;"
        "  padding:9px 18px;min-width:80px;"
        "}"
        "QPushButton#pmPrimary{"
        "  background:#1b4e7a;color:#fff6dc;border:1px solid #2e72a8;"
        "}"
        "QPushButton#pmPrimary:hover{background:#236090;}"
        "QPushButton#pmPrimary:pressed{background:#143860;}"
        "QPushButton#pmSecondary{"
        "  background:transparent;color:#7ab2d4;border:1px solid #244060;"
        "}"
        "QPushButton#pmSecondary:hover{background:#0f1e30;color:#c0dff4;border-color:#3a6888;}"
        "QPushButton#pmSecondary:pressed{background:#091420;}");
    auto* bbox = new QHBoxLayout(bar);
    bbox->setContentsMargins(20, 14, 20, 14);
    bbox->setSpacing(8);

    auto* installBtn    = new QPushButton("Install Starter Plugins");
    auto* openFolderBtn = new QPushButton("Open Plugins Folder");
    auto* reloadBtn     = new QPushButton("Reload");
    auto* closeBtn      = new QPushButton("Close");
    installBtn->setObjectName("pmSecondary");
    openFolderBtn->setObjectName("pmSecondary");
    reloadBtn->setObjectName("pmSecondary");
    closeBtn->setObjectName("pmPrimary");

    bbox->addWidget(installBtn);
    bbox->addWidget(openFolderBtn);
    bbox->addWidget(reloadBtn);
    bbox->addStretch();
    bbox->addWidget(closeBtn);
    root->addWidget(bar);

    // ── Card builder (called per plugin) ──────────────────────
    const auto buildCard = [](QWidget* parent, const LoadedPlugin& lp) -> QFrame* {
        auto* card = new QFrame(parent);
        card->setStyleSheet(
            "QFrame{"
            "  background:#0f1e30;border:1px solid #1e3a54;"
            "  border-radius:10px;"
            "}"
            "QFrame:hover{background:#122034;border-color:#3a6a90;}");

        auto* cl = new QHBoxLayout(card);
        cl->setContentsMargins(18, 14, 18, 14);
        cl->setSpacing(14);

        // Active/inactive indicator dot
        auto* dot = new QLabel(lp.active ? "●" : "○");
        dot->setFixedWidth(16);
        dot->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        dot->setStyleSheet(
            lp.active
                ? "color:#4caf50;font-size:15px;background:transparent;margin-top:2px;"
                : "color:#3a5060;font-size:15px;background:transparent;margin-top:2px;");

        // Text column
        auto* col = new QVBoxLayout;
        col->setSpacing(4);

        // Name + version + status badge
        auto* row1 = new QHBoxLayout;
        row1->setSpacing(8);

        auto* nameLbl = new QLabel(lp.info.name.isEmpty() ? "(unnamed)" : lp.info.name);
        nameLbl->setStyleSheet(
            "font-weight:800;font-size:13px;color:#e8d898;background:transparent;");
        row1->addWidget(nameLbl);

        if (!lp.info.version.isEmpty()) {
            auto* verLbl = new QLabel("v" + lp.info.version);
            verLbl->setStyleSheet("font-size:10px;color:#486880;background:transparent;");
            row1->addWidget(verLbl);
        }

        auto* badge = new QLabel(lp.active ? "Active" : "Registered");
        badge->setStyleSheet(
            lp.active
                ? "font-size:10px;font-weight:700;color:#66bb6a;"
                  "background:#0e2816;border:1px solid #2a5a2e;"
                  "border-radius:4px;padding:1px 8px;"
                : "font-size:10px;font-weight:700;color:#486070;"
                  "background:#0c1c28;border:1px solid #1c3848;"
                  "border-radius:4px;padding:1px 8px;");
        row1->addWidget(badge);
        row1->addStretch();
        col->addLayout(row1);

        // Description
        if (!lp.info.description.isEmpty()) {
            auto* descLbl = new QLabel(lp.info.description);
            descLbl->setWordWrap(true);
            descLbl->setStyleSheet("font-size:11px;color:#547a94;background:transparent;");
            col->addWidget(descLbl);
        }

        // Author + action count
        auto* row2 = new QHBoxLayout;
        row2->setSpacing(16);
        if (!lp.info.author.isEmpty()) {
            auto* authLbl = new QLabel("by " + lp.info.author);
            authLbl->setStyleSheet("font-size:10px;color:#344e60;background:transparent;");
            row2->addWidget(authLbl);
        }
        const int nAct = lp.bookActions.size();
        if (nAct > 0) {
            auto* actLbl = new QLabel(
                QString("%1 book action%2").arg(nAct).arg(nAct == 1 ? "" : "s"));
            actLbl->setStyleSheet("font-size:10px;color:#2c5878;background:transparent;");
            row2->addWidget(actLbl);
        }
        if (!lp.sourcePath.isEmpty()) {
            auto* pathLbl = new QLabel(QDir::toNativeSeparators(lp.sourcePath));
            pathLbl->setStyleSheet(
                "font-size:10px;color:#284050;background:transparent;"
                "font-family:monospace;");
            pathLbl->setWordWrap(false);
            row2->addWidget(pathLbl, 1);
        }
        row2->addStretch();
        col->addLayout(row2);

        cl->addWidget(dot);
        cl->addLayout(col, 1);
        return card;
    };

    // ── Populate (called on load and after reload) ─────────────
    const auto populate = [&]() {
        QLayoutItem* item;
        while ((item = listVBox->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        const QList<LoadedPlugin> plugins = PluginManager::instance().loadedPlugins();

        if (plugins.isEmpty()) {
            auto* empty = new QWidget;
            auto* el = new QVBoxLayout(empty);
            el->setAlignment(Qt::AlignCenter);
            el->setSpacing(10);
            el->setContentsMargins(40, 60, 40, 60);

            auto* ei = new QLabel("\xF0\x9F\x94\x8C");
            ei->setAlignment(Qt::AlignCenter);
            ei->setStyleSheet("font-size:56px;background:transparent;");
            auto* et = new QLabel("No plugins installed");
            et->setAlignment(Qt::AlignCenter);
            et->setStyleSheet(
                "font-size:16px;font-weight:700;color:#304e64;background:transparent;");
            auto* eh = new QLabel(
                "Click <b style='color:#5090b4;'>Install Starter Plugins</b> "
                "to install the built-in manifests,<br>or drop a plugin folder "
                "containing <code style='color:#3a7090;'>plugin.json</code> "
                "into the plugins directory.");
            eh->setAlignment(Qt::AlignCenter);
            eh->setTextFormat(Qt::RichText);
            eh->setStyleSheet("font-size:12px;color:#284050;background:transparent;");

            el->addWidget(ei);
            el->addWidget(et);
            el->addWidget(eh);
            listVBox->addWidget(empty);
        } else {
            for (const LoadedPlugin& lp : plugins)
                listVBox->addWidget(buildCard(listWidget, lp));
        }
        listVBox->addStretch();
    };

    populate();

    // ── Wire up buttons ────────────────────────────────────────
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    connect(openFolderBtn, &QPushButton::clicked, &dlg, []() {
        const QString dir = AppConfig::pluginsDir();
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });

    connect(installBtn, &QPushButton::clicked, &dlg, [this, &populate]() {
        PluginManager::instance().unloadAll();
        PluginManager::instance().loadAll(AppConfig::pluginsDir());
        if (m_statusLabel) m_statusLabel->setText("Starter plugins installed.");
        populate();
    });

    connect(reloadBtn, &QPushButton::clicked, &dlg, [this, &populate]() {
        PluginManager::instance().unloadAll();
        PluginManager::instance().loadAll(AppConfig::pluginsDir());
        if (m_statusLabel) m_statusLabel->setText("Plugins reloaded.");
        populate();
    });

    dlg.exec();
}

void MainWindow::remapLibraryPaths()
{
    QSet<QString> prefixes;
    for (const QString& folder : m_watchFolders) {
        const QString normalized = QDir::fromNativeSeparators(folder).trimmed();
        if (!normalized.isEmpty())
            prefixes.insert(normalized);
    }
    const QVector<Book> books = currentLibraryBooks();
    for (const Book& book : books) {
        const QString normalized = QDir::fromNativeSeparators(book.filePath).trimmed();
        if (normalized.isEmpty())
            continue;
        const int slash = normalized.indexOf('/', 3);
        prefixes.insert(slash > 0 ? normalized.left(slash) : normalized);
    }

    QStringList options = QStringList(prefixes.begin(), prefixes.end());
    options.sort(Qt::CaseInsensitive);
    if (options.isEmpty()) {
        QMessageBox::information(this, "Remap Paths", "No library paths are available to remap.");
        return;
    }

    bool ok = false;
    const QString fromPrefix = QInputDialog::getItem(this,
                                                     "Remap Paths",
                                                     "Old drive or folder root:",
                                                     options,
                                                     0,
                                                     true,
                                                     &ok).trimmed();
    if (!ok || fromPrefix.isEmpty())
        return;

    const QString toPrefix = QFileDialog::getExistingDirectory(this,
                                                               "Choose the new drive or replacement folder",
                                                               QDir::homePath(),
                                                               QFileDialog::ShowDirsOnly);
    if (toPrefix.trimmed().isEmpty())
        return;

    showTaskProgress("Remapping Paths", "Updating library file paths...", 0, 0, fromPrefix + " -> " + toPrefix);
    const int changed = Database::instance().remapPathPrefix(fromPrefix, toPrefix);

    for (QString& folder : m_watchFolders) {
        const QString normalized = QDir::fromNativeSeparators(folder).trimmed();
        if (normalized == fromPrefix)
            folder = toPrefix;
        else if (normalized.startsWith(fromPrefix + "/"))
            folder = toPrefix + normalized.mid(fromPrefix.size());
    }
    m_watchFolders.removeDuplicates();
    m_watchFolders.sort(Qt::CaseInsensitive);

    if (m_libraryProfiles.contains(m_currentLibraryName)) {
        QJsonArray folders;
        for (const QString& folder : m_watchFolders)
            folders.append(folder);
        m_libraryProfiles[m_currentLibraryName] = folders;
    }

    saveSettings();
    reloadCurrentLibrary();
    hideTaskProgress(QString("Remapped %1 library paths.").arg(changed));

    QMessageBox::information(this,
                             "Remap Paths",
                             QString("Updated %1 stored file paths.\n\nWatched folders for \"%2\" were refreshed as well.")
                                 .arg(changed)
                                 .arg(m_currentLibraryName));
}

// ── Saved searches ────────────────────────────────────────────
void MainWindow::saveCurrentSearch()
{
    const QString query = m_searchBox ? m_searchBox->text().trimmed() : QString();
    if (query.isEmpty()) {
        m_statusLabel->setText("Enter a search query first.");
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Save Search", "Name for this search:", QLineEdit::Normal, query, &ok);
    if (ok && !name.trimmed().isEmpty()) {
        BookFilter filter;
        filter.text = query;
        filter.format = m_formatCombo ? m_formatCombo->currentText() : QString();
        if (filter.format.compare("All", Qt::CaseInsensitive) == 0)
            filter.format.clear();
        filter.author = QString();
        filter.tag = QString();
        filter.language = QString();
        filter.series = QString();
        filter.favOnly = m_favAction && m_favAction->isChecked();
        filter.noCover = false;
        filter.noMeta = false;
        filter.restrictToPathPrefixes = true;
        filter.pathPrefixes = m_watchFolders;
        if (m_categoryCombo)
            filter.tag = m_categoryCombo->currentData().toString();

        Database::instance().saveSearch(name.trimmed(), query, QString::fromUtf8(QJsonDocument(filter.toJson()).toJson(QJsonDocument::Compact)));
        rebuildSavedSearchSidebar();
        m_statusLabel->setText("Search saved: " + name);
    }
}

void MainWindow::loadSavedSearch(int id)
{
    const QVector<SavedSearch> searches = Database::instance().savedSearches();
    const auto it = std::find_if(searches.begin(), searches.end(), [id](const SavedSearch& saved) {
        return saved.id == id;
    });
    if (it == searches.end()) {
        m_statusLabel->setText("Saved search not found.");
        return;
    }

    const BookFilter filter = BookFilter::fromJson(it->filterJson);
    if (m_searchBox)
        m_searchBox->setText(it->query);
    m_filterModel->setFilterText(it->query);
    m_filterModel->setFilterAuthor(filter.author);
    m_filterModel->setFilterYear(filter.yearFrom, filter.yearTo);
    m_filterModel->setFilterFavourites(filter.favOnly);
    m_filterModel->setFilterNoCover(filter.noCover);
    m_filterModel->setFilterNoMeta(filter.noMeta);
    m_filterModel->setFilterTag(filter.tag);
    if (m_formatCombo)
        m_formatCombo->setCurrentText(filter.format.isEmpty() ? "All" : filter.format);
    m_filterModel->setFilterFormat(filter.format);
    if (m_categoryCombo) {
        const int index = m_categoryCombo->findData(filter.tag);
        m_categoryCombo->setCurrentIndex(index >= 0 ? index : 0);
    }
    m_filterModel->setFilterCategory(filter.tag);
    updateStatusCount();
    m_statusLabel->setText("Saved search loaded: " + it->name);
}

void MainWindow::openAdvancedSearchDialog()
{
    openAdvancedSearch(); // forward to existing implementation
}
