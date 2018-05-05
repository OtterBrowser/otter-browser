/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include <QtCore/QPointer>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class MainWindow;
class Window;

class SessionItem : public QStandardItem
{
public:
	virtual Window* getActiveWindow() const;
	QVariant data(int role) const override;

protected:
	explicit SessionItem();

friend class SessionModel;
};

class MainWindowSessionItem final : public QObject, public SessionItem
{
	Q_OBJECT

public:
	Window* getActiveWindow() const override;
	MainWindow* getMainWindow() const;
	QVariant data(int role) const override;

protected:
	explicit MainWindowSessionItem(MainWindow *mainWindow);

protected slots:
	void handleWindowAdded(quint64 identifier);
	void handleWindowRemoved(quint64 identifier);
	void notifyMainWindowModified();

private:
	QPointer<MainWindow> m_mainWindow;

friend class SessionModel;
};

class WindowSessionItem final : public SessionItem
{
public:
	Window* getActiveWindow() const override;
	QVariant data(int role) const override;

protected:
	explicit WindowSessionItem(Window *window);

private:
	QPointer<Window> m_window;

friend class MainWindowSessionItem;
};

class SessionModel final : public QStandardItemModel
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
		IconRole = Qt::DecorationRole,
		IdentifierRole = Qt::UserRole,
		TypeRole,
		IndexRole,
		LastActivityRole,
		ZoomRole,
		IsActiveRole,
		IsAudibleRole,
		IsAudioMutedRole,
		IsDeferredRole,
		IsPinnedRole,
		IsPrivateRole,
		IsTrashedRole
	};

	explicit SessionModel(QObject *parent);

	SessionItem* getRootItem() const;
	SessionItem* getTrashItem() const;
	MainWindowSessionItem* getMainWindowItem(MainWindow *mainWindow) const;

protected slots:
	void handleMainWindowAdded(MainWindow *mainWindow);
	void handleMainWindowRemoved(MainWindow *mainWindow);

private:
	SessionItem *m_rootItem;
	SessionItem *m_trashItem;
	QMap<MainWindow*, MainWindowSessionItem*> m_mainWindowItems;

signals:
	void modelModified();
};

}

#endif
