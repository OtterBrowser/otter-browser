/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_NOTESCONTENTSWIDGET_H
#define OTTER_NOTESCONTENTSWIDGET_H

#include "../../../core/BookmarksModel.h"
#include "../../../ui/ContentsWidget.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class NotesContentsWidget;
}

class Window;

class NotesContentsWidget final : public ContentsWidget
{
	Q_OBJECT

public:
	explicit NotesContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent);
	~NotesContentsWidget();

	void print(QPrinter *printer) override;
	QString getTitle() const override;
	QLatin1String getType() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void changeEvent(QEvent *event) override;
	BookmarksModel::Bookmark* findFolder(const QModelIndex &index);
	QVariant getCurrentIndexData(int role) const;

protected slots:
	void addNote();
	void addFolder();
	void addSeparator();
	void removeNote();
	void openUrl();
	void showContextMenu(const QPoint &position);
	void updateActions();
	void updateText();

private:
	Ui::NotesContentsWidget *m_ui;
};

}

#endif
