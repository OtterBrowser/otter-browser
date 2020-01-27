/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CONTENTFILTERSVIEWWIDGET_H
#define OTTER_CONTENTFILTERSVIEWWIDGET_H

#include "ItemDelegate.h"
#include "ItemViewWidget.h"
#include "../core/ContentFiltersManager.h"

namespace Otter
{

class Animation;

class ContentFiltersTitleDelegate final : public ItemDelegate
{
public:
	explicit ContentFiltersTitleDelegate(QObject *parent = nullptr);

	bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;

protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class ContentFiltersIntervalDelegate final : public ItemDelegate
{
public:
	explicit ContentFiltersIntervalDelegate(QObject *parent = nullptr);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QString displayText(const QVariant &value, const QLocale &locale) const override;
};

class ContentFiltersViewWidget final : public ItemViewWidget
{
public:
	enum DataRole
	{
		CategoryRole = Qt::UserRole,
		HasErrorRole,
		IsModifiedRole,
		IsShowingProgressIndicatorRole,
		IsUpdatingRole,
		NameRole,
		UpdateProgressValueRole,
		UpdateTimeRole,
		UpdateUrlRole
	};

	explicit ContentFiltersViewWidget(QWidget *parent);

	static Animation* getUpdateAnimation();
	QString getHost() const;

public slots:
	void addProfile();
	void editProfile();
	void removeProfile();
	void updateProfile();
	void setHost(const QString &host);
	void save(bool areCustomRulesEnabled);

protected:
	void changeEvent(QEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void moveProfile(QStandardItem *entryItem, ContentFiltersProfile::ProfileCategory newCategory);
	QString createProfileTitle(const ContentFiltersProfile *profile) const;
	QList<QStandardItem*> createEntry(const ContentFiltersProfile *profile, const QStringList &profiles) const;

protected slots:
	void handleProfileAdded(const QString &name);
	void handleProfileModified(const QString &name);
	void handleProfileRemoved(const QString &name);
	void updateSelection();

private:
	QStandardItemModel *m_model;
	QString m_host;
	QHash<QString, bool> m_profilesToRemove;

	static Animation* m_updateAnimation;
};

}

#endif
