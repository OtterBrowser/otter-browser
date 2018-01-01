/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_PREFERENCESCONTENTPAGEWIDGET_H
#define OTTER_PREFERENCESCONTENTPAGEWIDGET_H

#include "../ItemDelegate.h"

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class PreferencesContentPageWidget;
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

class PreferencesContentPageWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit PreferencesContentPageWidget(QWidget *parent = nullptr);
	~PreferencesContentPageWidget();

public slots:
	void save();

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void handleCurrentFontChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
	void handleCurrentColorChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);

private:
	Ui::PreferencesContentPageWidget *m_ui;

signals:
	void settingsModified();
};

}

#endif
