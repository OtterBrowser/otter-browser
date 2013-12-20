#ifndef OTTER_BOOKMARKSMANAGER_H
#define OTTER_BOOKMARKSMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

namespace Otter
{

enum BookmarkType
{
	FolderBookmark = 0,
	UrlBookmark = 1,
	SeparatorBookmark = 2
};

struct BookmarkInformation
{
	QString url;
	QString title;
	QString description;
	QList<BookmarkInformation*> children;
	BookmarkType type;
	int identifier;
	int parent;

	BookmarkInformation() : type(FolderBookmark), identifier(-1), parent(-1) {}
};

class BookmarksManager : public QObject
{
	Q_OBJECT

public:
	~BookmarksManager();

	static void createInstance(QObject *parent = NULL);
	static BookmarksManager* getInstance();
	static QStringList getUrls();
	static QList<BookmarkInformation*> getBookmarks();
	static QList<BookmarkInformation*> getFolder(int folder = 0);
	static bool addBookmark(BookmarkInformation *bookmark, int folder = 0, int index = -1);
	static bool updateBookmark(BookmarkInformation *bookmark);
	static bool deleteBookmark(BookmarkInformation *bookmark, bool notify = true);
	static bool deleteBookmark(const QUrl &url);
	static bool hasBookmark(const QString &url);
	static bool hasBookmark(const QUrl &url);
	static bool save(const QString &path = QString());

private:
	explicit BookmarksManager(QObject *parent = NULL);

	static void writeBookmark(QXmlStreamWriter *writer, BookmarkInformation *bookmark);
	static void updateUrls();
	BookmarkInformation* readBookmark(QXmlStreamReader *reader, int parent = -1);

	static BookmarksManager *m_instance;
	static QHash<int, BookmarkInformation*> m_pointers;
	static QList<BookmarkInformation*> m_bookmarks;
	static QList<BookmarkInformation*> m_allBookmarks;
	static QSet<QString> m_urls;
	static int m_identifier;

private slots:
	void load();

signals:
	void folderModified(int folder);
};

}

#endif
