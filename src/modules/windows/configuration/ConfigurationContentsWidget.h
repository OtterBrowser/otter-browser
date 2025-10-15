/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_CONFIGURATIONCONTENTSWIDGET_H
#define OTTER_CONFIGURATIONCONTENTSWIDGET_H

#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ItemDelegate.h"

#include <QtGui/QStandardItemModel>

namespace Otter
{

namespace Ui
{
	class ConfigurationContentsWidget;
}

class Window;

class ConfigurationOptionDelegate final : public ItemDelegate
{
public:
	explicit ConfigurationOptionDelegate(bool shouldMarkAsModified, QObject *parent);

	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
	bool m_shouldMarkAsModified;
};

class ConfigurationContentsWidget final : public ContentsWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		NameRole,
		IsModifiedRole,
		RequiresRestartRole
	};

	explicit ConfigurationContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent);
	~ConfigurationContentsWidget();

	void print(QPrinter *printer) override;
	QString getTitle() const override;
	QLatin1String getType() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;

protected:
	void changeEvent(QEvent *event) override;
	void saveAll(bool reset);
	QModelIndex findGroup(int identifier) const;
	bool canClose() override;

protected slots:
	void resetOption();
	void saveOption();
	void handleOptionChanged(int identifier, const QVariant &value);
	void handleHostOptionChanged(int identifier);
	void showContextMenu(const QPoint &position);
	void updateActions();

private:
	QStandardItemModel *m_model;
	Ui::ConfigurationContentsWidget *m_ui;
};

}

#endif
