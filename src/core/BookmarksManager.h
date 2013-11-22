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
	QList<Bookmark> children;
	BookmarkType type;
	int identifier;

	Bookmark() : type(FolderBookmark), identifier(-1) {}
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

	void writeBookmark(QXmlStreamWriter *writer, const Bookmark &bookmark);
	Bookmark readBookmark(QXmlStreamReader *reader) const;

	static BookmarksManager *m_instance;
	static QList<Bookmark> m_bookmarks;
	static QSet<QString> m_urls;

private slots:
	void load();
};

}

#endif
