/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 - 2017 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_SEARCHWIDGET_H
#define OTTER_SEARCHWIDGET_H

#include "../../../core/WindowsManager.h"
#include "../../../ui/LineEditWidget.h"

#include <QtCore/QPointer>
#include <QtWidgets/QItemDelegate>

namespace Otter
{

class SearchSuggester;
class Window;

class SearchDelegate : public QItemDelegate
{
public:
	explicit SearchDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class SearchWidget : public LineEditWidget
{
	Q_OBJECT

public:
	explicit SearchWidget(Window *window, QWidget *parent = nullptr);

	QVariantMap getOptions() const;
	bool event(QEvent *event) override;

public slots:
	void setWindow(Window *window = nullptr);
	void setSearchEngine(const QString &searchEngine = QString());
	void setOptions(const QVariantMap &options);
	void setQuery(const QString &query);

protected:
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	QModelIndex getCurrentIndex() const;

protected slots:
	void sendRequest(const QString &query = QString());
	void showCompletion(bool showSearchModel = false);
	void pasteAndGo();
	void addSearchEngine(QAction *action);
	void storeCurrentSearchEngine();
	void restoreCurrentSearchEngine();
	void handleOptionChanged(int identifier, const QVariant &value);
	void updateGeometries();
	void setSearchEngine(QModelIndex index);

private:
	QPointer<Window> m_window;
	SearchSuggester *m_suggester;
	QString m_query;
	QString m_storedSearchEngine;
	QRect m_iconRectangle;
	QRect m_dropdownArrowRectangle;
	QRect m_addButtonRectangle;
	QRect m_searchButtonRectangle;
	QVariantMap m_options;
	bool m_isIgnoringActivation;
	bool m_isSearchEngineLocked;

signals:
	void searchEngineChanged(const QString &searchEngine);
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints);
};

}

#endif
