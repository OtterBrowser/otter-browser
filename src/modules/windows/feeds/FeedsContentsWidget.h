/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_FEEDSCONTENTSWIDGET_H
#define OTTER_FEEDSCONTENTSWIDGET_H

#include "../../../core/FeedsManager.h"
#include "../../../ui/ContentsWidget.h"
#include "../../../ui/ItemDelegate.h"

namespace Otter
{

namespace Ui
{
	class FeedsContentsWidget;
}

class Animation;
class Window;

class FeedEntryDelegate final : public ItemDelegate
{
public:
	explicit FeedEntryDelegate(QObject *parent);

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class FeedDelegate final : public ItemDelegate
{
public:
	explicit FeedDelegate(QObject *parent);

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class FeedsContentsWidget final : public ContentsWidget
{
	Q_OBJECT

public:
	enum DataRole
	{
		TitleRole = Qt::DisplayRole,
		UrlRole = Qt::StatusTipRole,
		IdentifierRole = Qt::UserRole,
		SummaryRole,
		ContentRole,
		AuthorRole,
		EmailRole,
		LastReadTimeRole,
		PublicationTimeRole,
		UpdateTimeRole,
		CategoriesRole
	};

	explicit FeedsContentsWidget(const QVariantMap &parameters, QWidget *parent);
	~FeedsContentsWidget();

	void print(QPrinter *printer) override;
	static Animation* getUpdateAnimation();
	QString getTitle() const override;
	QLatin1String getType() const override;
	QUrl getUrl() const override;
	QIcon getIcon() const override;
	ActionsManager::ActionDefinition::State getActionState(int identifier, const QVariantMap &parameters = {}) const override;
	bool eventFilter(QObject *object, QEvent *event) override;

public slots:
	void triggerAction(int identifier, const QVariantMap &parameters = {}, ActionsManager::TriggerType trigger = ActionsManager::UnknownTrigger) override;
	void setUrl(const QUrl &url, bool isTypedIn = true) override;

protected:
	void changeEvent(QEvent *event) override;
	void setFeed(Feed *feed);
	FeedsModel::Entry* findFolder(const QModelIndex &index) const;

protected slots:
	void addFeed();
	void addFolder();
	void openFeed();
	void updateFeed();
	void removeFeed();
	void subscribeFeed();
	void feedProperties();
	void openEntry();
	void removeEntry();
	void handleFeedModified(const QUrl &url);
	void showEntriesContextMenu(const QPoint &position);
	void showFeedsContextMenu(const QPoint &position);
	void updateActions();
	void updateEntry();
	void updateFeedModel();

private:
	Feed *m_feed;
	QStandardItemModel *m_feedModel;
	QStringList m_categories;
	Ui::FeedsContentsWidget *m_ui;

	static Animation* m_updateAnimation;
};

}

#endif
