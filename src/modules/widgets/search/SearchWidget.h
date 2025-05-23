/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "../../../core/SessionsManager.h"
#include "../../../ui/LineEditWidget.h"
#include "../../../ui/WebWidget.h"

#include <QtCore/QPointer>
#include <QtWidgets/QItemDelegate>

namespace Otter
{

class SearchSuggester;
class Window;

class SearchDelegate final : public QItemDelegate
{
public:
	explicit SearchDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class SearchWidget final : public LineEditWidget
{
	Q_OBJECT

public:
	explicit SearchWidget(Window *window, QWidget *parent = nullptr);

	QVariantMap getOptions() const;
	bool event(QEvent *event) override;

public slots:
	void setWindow(Window *window);
	void setOptions(const QVariantMap &options);
	void setQuery(const QString &query);

protected:
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	QModelIndex getCurrentIndex() const;

protected slots:
	void sendRequest(const QString &query = {});
	void showSearchEngines();
	void showSearchSuggestions();
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleWindowOptionChanged(int identifier, const QVariant &value);
	void handleWatchedDataChanged(WebWidget::ChangeWatcher watcher);
	void handleLoadingStateChanged();
	void updateGeometries();
	void setSearchEngine(const QString &searchEngine);
	void setSearchEngine(const QModelIndex &index, bool canSendRequest = true);

private:
	QPointer<Window> m_window;
	SearchSuggester *m_suggester;
	QString m_query;
	QString m_searchEngine;
	QRect m_iconRectangle;
	QRect m_dropdownArrowRectangle;
	QRect m_addButtonRectangle;
	QRect m_searchButtonRectangle;
	QVariantMap m_options;
	bool m_hasAllWindowSearchEngines;
	bool m_isPrivate;
	bool m_isSearchEngineLocked;
	bool m_isSearchInPrivateTabsEnabled;

signals:
	void requestedSearch(const QString &query, const QString &searchEngine, SessionsManager::OpenHints hints);
};

}

#endif
