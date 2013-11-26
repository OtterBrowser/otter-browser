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

struct Bookmark
{
	QString url;
	QString title;
	QString description;
	QList<Bookmark*> children;
	BookmarkType type;
	int identifier;
	int parent;

	Bookmark() : type(FolderBookmark), identifier(-1), parent(-1) {}
};

class BookmarksManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static QList<Bookmark*> getBookmarks();
	static QList<Bookmark*> getFolder(int folder = 0);
	static bool hasBookmark(const QString &url);
	static bool setBookmarks(const QList<Bookmark*> &bookmarks);

private:
	explicit BookmarksManager(QObject *parent = NULL);

	void writeBookmark(QXmlStreamWriter *writer, Bookmark *bookmark);
	Bookmark* readBookmark(QXmlStreamReader *reader, int parent = -1);

	static BookmarksManager *m_instance;
	static QHash<int, Bookmark*> m_pointers;
	static QList<Bookmark*> m_bookmarks;
	static QSet<QString> m_urls;
	static int m_identifier;

private slots:
	void load();
};

}

#endif
