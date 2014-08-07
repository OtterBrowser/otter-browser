/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtWidgets/QComboBox>
#include <QtWidgets/QCompleter>

namespace Otter
{

class SearchSuggester;

class SearchWidget : public QComboBox
{
	Q_OBJECT

public:
	explicit SearchWidget(QWidget *parent = NULL);

	void hidePopup();
	QString getCurrentSearchEngine() const;

public slots:
	void setCurrentSearchEngine(const QString &engine = QString());

protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	bool setPlaceholderText(int index);

protected slots:
	void optionChanged(const QString &option, const QVariant &value);
	void currentSearchEngineChanged(int index);
	void searchEngineSelected(int index);
	void queryChanged(const QString &query);
	void sendRequest(const QString &query = QString());
	void storeCurrentSearchEngine();
	void restoreCurrentSearchEngine();

private:
	QCompleter *m_completer;
	SearchSuggester *m_suggester;
	QString m_query;
	QString m_storedSearchEngine;
	QRect m_selectButtonIconRectangle;
	QRect m_selectButtonArrowRectangle;
	QRect m_lineEditRectangle;
	QRect m_searchButtonRectangle;
	int m_index;
	bool m_popupUpdated;

signals:
	void requestedSearch(QString query, QString engine);
};

}

#endif
