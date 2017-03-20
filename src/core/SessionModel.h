/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_SESSIONMODEL_H
#define OTTER_SESSIONMODEL_H

#include <QtGui/QStandardItemModel>

namespace Otter
{

class MainWindow;
class Window;

class SessionItem : public QStandardItem
{
public:
	virtual Window* getActiveWindow() const;

protected:
	explicit SessionItem();

friend class SessionModel;
};

class MainWindowSessionItem : public SessionItem
{
public:
	Window* getActiveWindow() const override;

protected:
	explicit MainWindowSessionItem(MainWindow *mainWindow);

private:
	MainWindow *m_mainWindow;
};

class SessionModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum EntityType
	{
		UnknownEntity = 0,
		SessionEntity,
		TrashEntity,
		MainWindowEntity,
		WindowEntity,
		HistoryEntity
	};

	enum EntityRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		IdentifierRole = Qt::UserRole,
		TypeRole,
		IndexRole,
		LastActivityRole,
		ZoomRole,
		OptionsRole,
		IsPinnedRole,
		IsPrivateRole,
		IsSuspendedRole
	};

	explicit SessionModel(const QString &path, QObject *parent);

	SessionItem* getRootItem() const;
	SessionItem* getTrashItem() const;

private:
	SessionItem *m_rootItem;
	SessionItem *m_trashItem;
	QString m_path;
};

}

#endif
