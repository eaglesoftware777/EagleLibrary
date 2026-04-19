// ============================================================
//  Eagle Library -- BookModel.cpp
//  Copyright (c) 2026 Eagle Software. All rights reserved.
// ============================================================

#include "BookModel.h"
#include "Database.h"
#include <QFileInfo>
#include <QRegularExpression>

namespace {

QStringList parseSearchTokens(const QString& input)
{
    QStringList tokens;
    QRegularExpression re(R"re((\w+):"([^"]+)"|"([^"]+)"|(\S+))re");
    QRegularExpressionMatchIterator it = re.globalMatch(input);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        QString token;
        if (!match.captured(1).isEmpty())
            token = match.captured(1) + ":" + match.captured(2);
        else if (!match.captured(3).isEmpty())
            token = match.captured(3);
        else
            token = match.captured(4);
        if (!token.trimmed().isEmpty())
            tokens << token.trimmed();
    }
    return tokens;
}

bool tokenMatchesField(const QString& field, const QString& value)
{
    return field.toLower().contains(value.toLower());
}

}

// ── BookModel ─────────────────────────────────────────────────
BookModel::BookModel(QObject* parent) : QAbstractListModel(parent) {}

int BookModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_books.size();
}

QVariant BookModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_books.size()) return {};
    const Book& b = m_books[index.row()];
    switch (role) {
        case Qt::DisplayRole:   return b.displayTitle();
        case IdRole:            return b.id;
        case TitleRole:         return b.displayTitle();
        case AuthorRole:        return b.displayAuthor();
        case FormatRole:        return b.format;
        case YearRole:          return b.year;
        case RatingRole:        return b.rating;
        case CoverPathRole:     return b.coverPath;
        case HasCoverRole:      return b.hasCover;
        case IsFavouriteRole:   return b.isFavourite;
        case FileSizeRole:      return b.fileSize;
        case FilePathRole:      return b.filePath;
        case DescriptionRole:   return b.description;
        case TagsRole: {
            QStringList tags = b.tags;
            const QString synthetic = b.isLikelyDocument() ? QStringLiteral("Document") : QStringLiteral("Book");
            if (!tags.contains(synthetic, Qt::CaseInsensitive))
                tags << synthetic;
            return tags;
        }
        case IsbnRole:          return b.isbn;
        case PagesRole:         return b.pages;
        case PublisherRole:     return b.publisher;
        case LanguageRole:      return b.language;
        case OpenCountRole:     return b.openCount;
        case DateAddedRole:     return b.dateAdded;
        case CategoryRole:      return b.classificationTag();
    }
    return {};
}

QHash<int, QByteArray> BookModel::roleNames() const
{
    return {
        {IdRole,          "bookId"},
        {TitleRole,       "title"},
        {AuthorRole,      "author"},
        {FormatRole,      "format"},
        {YearRole,        "year"},
        {RatingRole,      "rating"},
        {CoverPathRole,   "coverPath"},
        {HasCoverRole,    "hasCover"},
        {IsFavouriteRole, "isFavourite"},
        {FileSizeRole,    "fileSize"},
        {FilePathRole,    "filePath"},
        {TagsRole,        "tags"},
        {PublisherRole,   "publisher"},
        {LanguageRole,    "language"},
        {OpenCountRole,   "openCount"},
        {DateAddedRole,   "dateAdded"},
        {CategoryRole,    "category"},
    };
}

void BookModel::loadAll()
{
    reload({}, SortField::Title, SortOrder::Asc);
}

void BookModel::reload(const BookFilter& filter, SortField sort, SortOrder order)
{
    m_filter = filter;
    m_sort   = sort;
    m_order  = order;
    beginResetModel();
    m_books = Database::instance().query(filter, sort, order);
    endResetModel();
    emit countChanged(m_books.size());
}

void BookModel::addBook(const Book& book)
{
    beginInsertRows({}, m_books.size(), m_books.size());
    m_books.append(book);
    endInsertRows();
    emit countChanged(m_books.size());
}

void BookModel::updateBook(const Book& book)
{
    int row = rowForId(book.id);
    if (row < 0) return;
    m_books[row] = book;
    emit dataChanged(index(row), index(row));
}

void BookModel::removeBook(qint64 id)
{
    int row = rowForId(id);
    if (row < 0) return;
    beginRemoveRows({}, row, row);
    m_books.removeAt(row);
    endRemoveRows();
    emit countChanged(m_books.size());
}

void BookModel::updateCover(qint64 id, const QString& coverPath)
{
    int row = rowForId(id);
    if (row < 0) return;
    m_books[row].coverPath = coverPath;
    m_books[row].hasCover  = !coverPath.isEmpty();
    emit dataChanged(index(row), index(row), {CoverPathRole, HasCoverRole});
}

void BookModel::updateMetadata(qint64 id, const Book& updated)
{
    int row = rowForId(id);
    if (row < 0) return;
    Book& b = m_books[row];
    bool changed = false;
    auto setIfEmpty = [&](QString& dest, const QString& src) {
        if (dest.isEmpty() && !src.isEmpty()) { dest = src; changed = true; }
    };
    setIfEmpty(b.title,       updated.title);
    setIfEmpty(b.author,      updated.author);
    setIfEmpty(b.publisher,   updated.publisher);
    setIfEmpty(b.isbn,        updated.isbn);
    setIfEmpty(b.description, updated.description);
    setIfEmpty(b.language,    updated.language);
    if (b.year   == 0 && updated.year   > 0) { b.year   = updated.year;   changed = true; }
    if (b.pages  == 0 && updated.pages  > 0) { b.pages  = updated.pages;  changed = true; }
    if (b.rating == 0 && updated.rating > 0) { b.rating = updated.rating; changed = true; }
    if (b.subjects.isEmpty() && !updated.subjects.isEmpty()) {
        b.subjects = updated.subjects; changed = true;
    }
    if (changed) emit dataChanged(index(row), index(row));
}

const Book* BookModel::bookById(qint64 id) const
{
    for (const auto& b : m_books) if (b.id == id) return &b;
    return nullptr;
}

int BookModel::rowForId(qint64 id) const
{
    for (int i = 0; i < m_books.size(); ++i)
        if (m_books[i].id == id) return i;
    return -1;
}

void BookModel::setSortField(SortField f)
{
    m_sort = f;
    reload(m_filter, m_sort, m_order);
}

void BookModel::setSortOrder(SortOrder o)
{
    m_order = o;
    reload(m_filter, m_sort, m_order);
}

void BookModel::toggleSortOrder()
{
    m_order = (m_order == SortOrder::Asc) ? SortOrder::Desc : SortOrder::Asc;
    reload(m_filter, m_sort, m_order);
}

// ── BookFilterModel ───────────────────────────────────────────
BookFilterModel::BookFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

void BookFilterModel::setFilterText(const QString& text)   { m_filterText   = text;   invalidateFilter(); }
void BookFilterModel::setFilterFormat(const QString& fmt)  { m_filterFormat = fmt;    invalidateFilter(); }
void BookFilterModel::setFilterFavourites(bool fav)        { m_filterFav    = fav;    invalidateFilter(); }
void BookFilterModel::setFilterNoCover(bool v)             { m_filterNoCover= v;      invalidateFilter(); }
void BookFilterModel::setFilterNoMeta(bool v)              { m_filterNoMeta = v;      invalidateFilter(); }
void BookFilterModel::setFilterAuthor(const QString& auth) { m_filterAuthor = auth;   invalidateFilter(); }
void BookFilterModel::setFilterTag(const QString& tag)      { m_filterTag    = tag;    invalidateFilter(); }
void BookFilterModel::setFilterCategory(const QString& category) { m_filterCategory = category; invalidateFilter(); }

void BookFilterModel::setFilterYear(int from, int to)
{
    m_yearFrom = from;
    m_yearTo   = to;
    invalidateFilter();
}

void BookFilterModel::clearFilters()
{
    m_filterText.clear();
    m_filterFormat.clear();
    m_filterAuthor.clear();
    m_filterTag.clear();
    m_filterCategory.clear();
    m_filterFav    = false;
    m_filterNoCover= false;
    m_filterNoMeta = false;
    m_yearFrom     = 0;
    m_yearTo       = 0;
    invalidateFilter();
}

void BookFilterModel::sortBy(SortField f, SortOrder o)
{
    m_sortField = f;
    // Map SortField to role
    int role = TitleRole;
    switch (f) {
        case SortField::Author:    role = AuthorRole;    break;
        case SortField::Year:      role = YearRole;      break;
        case SortField::Rating:    role = RatingRole;    break;
        case SortField::FileSize:  role = FileSizeRole;  break;
        case SortField::Format:    role = FormatRole;    break;
        case SortField::OpenCount: role = OpenCountRole; break;
        case SortField::DateAdded: role = DateAddedRole; break;
        default:                   role = TitleRole;     break;
    }
    setSortRole(role);
    QSortFilterProxyModel::sort(0, o == SortOrder::Asc ? Qt::AscendingOrder : Qt::DescendingOrder);
}

bool BookFilterModel::filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const
{
    auto* src = sourceModel();
    QModelIndex idx = src->index(srcRow, 0, srcParent);

    if (m_filterFav && !src->data(idx, IsFavouriteRole).toBool()) return false;
    if (m_filterNoCover && src->data(idx, HasCoverRole).toBool()) return false;
    if (m_filterNoMeta && !src->data(idx, AuthorRole).toString().isEmpty()) return false;

    if (!m_filterFormat.isEmpty()) {
        if (src->data(idx, FormatRole).toString() != m_filterFormat) return false;
    }
    if (!m_filterAuthor.isEmpty()) {
        if (!src->data(idx, AuthorRole).toString()
                .contains(m_filterAuthor, Qt::CaseInsensitive)) return false;
    }
    if (!m_filterTag.isEmpty()) {
        const QStringList tags = src->data(idx, TagsRole).toStringList();
        bool matched = false;
        for (const QString& tag : tags) {
            if (tag.compare(m_filterTag, Qt::CaseInsensitive) == 0
                || tag.contains(m_filterTag, Qt::CaseInsensitive)) {
                matched = true;
                break;
            }
        }
        if (!matched) return false;
    }
    if (!m_filterCategory.isEmpty()) {
        const QString category = src->data(idx, CategoryRole).toString();
        const QStringList tags = src->data(idx, TagsRole).toStringList();
        const QString value = m_filterCategory.trimmed();
        bool matched = category.compare(value, Qt::CaseInsensitive) == 0;
        if (!matched) {
            for (const QString& tag : tags) {
                if (tag.compare(value, Qt::CaseInsensitive) == 0 || tag.contains(value, Qt::CaseInsensitive)) {
                    matched = true;
                    break;
                }
            }
        }
        if (!matched) return false;
    }
    if (m_yearFrom > 0) {
        int y = src->data(idx, YearRole).toInt();
        if (y > 0 && y < m_yearFrom) return false;
    }
    if (m_yearTo > 0) {
        int y = src->data(idx, YearRole).toInt();
        if (y > 0 && y > m_yearTo) return false;
    }
    if (!m_filterText.isEmpty()) {
        const QString title = src->data(idx, TitleRole).toString();
        const QString author = src->data(idx, AuthorRole).toString();
        const QString publisher = src->data(idx, PublisherRole).toString();
        const QString description = src->data(idx, DescriptionRole).toString();
        const QString tags = src->data(idx, TagsRole).toStringList().join(" ");
        const QString isbn = src->data(idx, IsbnRole).toString();
        const QString language = src->data(idx, LanguageRole).toString();
        const QString path = src->data(idx, FilePathRole).toString();
        const QString category = src->data(idx, CategoryRole).toString();
        const QString format = src->data(idx, FormatRole).toString();
        const QString year = src->data(idx, YearRole).toString();
        const QString haystack = (title + "\n" + author + "\n" + publisher + "\n" + description + "\n"
                                  + tags + "\n" + isbn + "\n" + language + "\n" + path + "\n"
                                  + category + "\n" + format + "\n" + year).toLower();
        const QStringList tokens = parseSearchTokens(m_filterText);
        for (QString token : tokens) {
            if (token.compare("and", Qt::CaseInsensitive) == 0)
                continue;

            bool negated = false;
            if (token.startsWith('-')) {
                negated = true;
                token.remove(0, 1);
            }

            bool matched = false;
            const int colonIndex = token.indexOf(':');
            if (colonIndex > 0) {
                const QString key = token.left(colonIndex).toLower();
                const QString value = token.mid(colonIndex + 1);
                if (key == "author") matched = tokenMatchesField(author, value);
                else if (key == "title") matched = tokenMatchesField(title, value);
                else if (key == "tag" || key == "tags") matched = tokenMatchesField(tags, value);
                else if (key == "isbn") matched = tokenMatchesField(isbn, value);
                else if (key == "path" || key == "file") matched = tokenMatchesField(path, value);
                else if (key == "category" || key == "type") matched = tokenMatchesField(category, value);
                else if (key == "publisher" || key == "pub") matched = tokenMatchesField(publisher, value);
                else if (key == "lang" || key == "language") matched = tokenMatchesField(language, value);
                else if (key == "format" || key == "ext") matched = tokenMatchesField(format, value);
                else if (key == "year") matched = tokenMatchesField(year, value);
                else if (key == "desc" || key == "description" || key == "content") matched = tokenMatchesField(description, value);
                else matched = haystack.contains(token.toLower());
            } else {
                matched = haystack.contains(token.toLower());
            }

            if ((!negated && !matched) || (negated && matched))
                return false;
        }
    }
    return true;
}

bool BookFilterModel::lessThan(const QModelIndex& l, const QModelIndex& r) const
{
    // Use the sort role set by sortBy()
    return QSortFilterProxyModel::lessThan(l, r);
}
