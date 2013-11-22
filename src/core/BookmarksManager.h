#ifndef OTTER_BOOKMARKSMANAGER_H
#define OTTER_BOOKMARKSMANAGER_H

#include <QtCore/QObject>

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
	QList<Bookmark> children;
	BookmarkType type;

	Bookmark() : type(FolderBookmark) {}
};

class BookmarksManager : public QObject
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = NULL);
	static QList<Bookmark> getBookmarks();
	static bool hasBookmark(const QString &url);
	static bool setBookmarks(const QList<Bookmark> &bookmarks);

private:
	explicit BookmarksManager(QObject *parent = NULL);

	void load();

	static BookmarksManager *m_instance;
	static QList<Bookmark> m_bookmarks;
	static QSet<QString> m_urls;
};

}

#endif
