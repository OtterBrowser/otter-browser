/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_ADDRESSCOMPLETIONMODEL_H
#define OTTER_ADDRESSCOMPLETIONMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QUrl>

namespace Otter
{

class AddressCompletionModel : public QAbstractListModel
{
	Q_OBJECT

public:
	static AddressCompletionModel* getInstance();
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex &index = QModelIndex()) const;

protected:
	explicit AddressCompletionModel(QObject *parent = NULL);

	void timerEvent(QTimerEvent *event);

protected slots:
	void optionChanged(const QString &option);
	void updateCompletion();

private:
	QList<QUrl> m_urls;
	int m_updateTimer;

	static AddressCompletionModel *m_instance;
};

}

#endif
