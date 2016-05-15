/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_SEARCHWIDGET_H
#define OTTER_SEARCHWIDGET_H

#include "../../core/WindowsManager.h"

#include <QtCore/QTime>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCompleter>

namespace Otter
{

class LineEditWidget;
class SearchSuggester;
class Window;

class SearchWidget : public QComboBox
{
	Q_OBJECT

public:
	explicit SearchWidget(Window *window, QWidget *parent = NULL);

	void hidePopup();
	QString getCurrentSearchEngine() const;
	QVariantMap getOptions() const;

public slots:
	void activate(Qt::FocusReason reason);
	void setWindow(Window *window = NULL);
	void setSearchEngine(const QString &searchEngine = QString());
	void setOptions(const QVariantMap &options);

protected:
	void changeEvent(QEvent *event);
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void focusInEvent(QFocusEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void currentIndexChanged(int index);
	void queryChanged(const QString &query);
	void sendRequest(const QString &query = QString());
	void pasteAndGo();
	void storeCurrentSearchEngine();
	void restoreCurrentSearchEngine();

private:
	QPointer<Window> m_window;
	LineEditWidget *m_lineEdit;
	QCompleter *m_completer;
	SearchSuggester *m_suggester;
	QString m_query;
	QString m_storedSearchEngine;
	QTime m_popupHideTime;
	QRect m_iconRectangle;
	QRect m_dropdownArrowRectangle;
	QRect m_lineEditRectangle;
	QRect m_searchButtonRectangle;
	QVariantMap m_options;
	int m_lastValidIndex;
	bool m_isIgnoringActivation;
	bool m_isPopupUpdated;
	bool m_isSearchEngineLocked;
	bool m_wasPopupVisible;

signals:
	void searchEngineChanged(const QString &searchEngine);
	void requestedOpenUrl(const QUrl &url, WindowsManager::OpenHints hints);
	void requestedSearch(const QString &query, const QString &searchEngine, WindowsManager::OpenHints hints);
};

}

#endif
