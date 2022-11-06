/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CATEGORIESTABWIDGET_H
#define OTTER_CATEGORIESTABWIDGET_H

#include <QtWidgets/QTabWidget>

namespace Otter
{

class CategoryPage;

class CategoriesTabWidget final : public QTabWidget
{
public:
	explicit CategoriesTabWidget(QWidget *parent = nullptr);

	void addPage(CategoryPage *page);
	void addPage(const QString &title);
	CategoryPage* getPage(int index);

protected:
	void changeEvent(QEvent *event);
	void updateStyle();

private:
	QVector<CategoryPage*> m_pages;
};

class CategoryPage : public QWidget
{
	Q_OBJECT

public:
	explicit CategoryPage(QWidget *parent = nullptr);

	virtual void load() = 0;
	virtual QString getTitle() const = 0;
	bool wasLoaded() const;

public slots:
	virtual void save();

protected:
	void showEvent(QShowEvent *event) override;
	void markAsLoaded();

private:
	bool m_wasLoaded;

signals:
	void loaded();
	void settingsModified();
};

}

#endif
