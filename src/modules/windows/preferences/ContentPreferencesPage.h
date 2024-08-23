/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2024 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_CONTENTPREFERENCESPAGE_H
#define OTTER_CONTENTPREFERENCESPAGE_H

#include "../../../ui/CategoriesTabWidget.h"
#include "../../../ui/ItemDelegate.h"

namespace Otter
{

namespace Ui
{
	class ContentPreferencesPage;
}

class ColorItemDelegate final : public ItemDelegate
{
public:
	explicit ColorItemDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class FontItemDelegate final : public ItemDelegate
{
public:
	explicit FontItemDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class ContentPreferencesPage final : public CategoryPage
{
	Q_OBJECT

public:
	explicit ContentPreferencesPage(QWidget *parent);
	~ContentPreferencesPage();

	void load() override;
	QString getTitle() const override;

public slots:
	void save() override;

protected:
	void changeEvent(QEvent *event) override;
	void updateStyle();

private:
	Ui::ContentPreferencesPage *m_ui;
};

}

#endif
