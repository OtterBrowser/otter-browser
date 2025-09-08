/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentFiltersViewWidget.h"
#include "Animation.h"
#include "ContentBlockingProfileDialog.h"
#include "../core/SettingsManager.h"
#include "../core/ThemesManager.h"
#include "../core/Utils.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolTip>

namespace Otter
{

ContentFiltersTitleDelegate::ContentFiltersTitleDelegate(QObject *parent) : ItemDelegate({{ItemDelegate::ProgressHasErrorRole, ContentFiltersViewWidget::HasErrorRole}, {ItemDelegate::ProgressHasIndicatorRole, ContentFiltersViewWidget::IsShowingProgressIndicatorRole}, {ItemDelegate::ProgressValueRole, ContentFiltersViewWidget::UpdateProgressValueRole}}, parent)
{
}

void ContentFiltersTitleDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	if (!index.parent().isValid())
	{
		return;
	}

	option->features |= QStyleOptionViewItem::HasDecoration;

	if (index.data(ContentFiltersViewWidget::IsUpdatingRole).toBool())
	{
		const Animation *animation(ContentFiltersViewWidget::getUpdateAnimation());

		if (animation)
		{
			option->icon = QIcon(animation->getCurrentPixmap());
		}
	}
	else if (index.data(ContentFiltersViewWidget::HasErrorRole).toBool())
	{
		option->icon = ThemesManager::createIcon(QLatin1String("dialog-error"));
	}
	else if (!index.data(ContentFiltersViewWidget::UpdateUrlRole).toUrl().isValid() || index.data(ContentFiltersViewWidget::UpdateTimeRole).isNull() || index.data(ContentFiltersViewWidget::UpdateTimeRole).toDateTime().daysTo(QDateTime::currentDateTimeUtc()) > 7)
	{
		option->icon = ThemesManager::createIcon(QLatin1String("dialog-warning"));
	}

	if (index.parent().data(ContentFiltersViewWidget::CategoryRole).toInt() == ContentFiltersProfile::RegionalCategory)
	{
		const QStringList languages(index.data(ContentFiltersViewWidget::LanguagesRole).toStringList());

		if (!languages.isEmpty())
		{
			option->text = QStringLiteral("%1 [%2]").arg(index.data(Qt::DisplayRole).toString(), languages.join(QLatin1String(", ")));
		}
	}
}

bool ContentFiltersTitleDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	if (event->type() == QEvent::ToolTip)
	{
		const QModelIndex entryIndex(index.sibling(index.row(), 0));
		const ContentFiltersProfile *profile(ContentFiltersViewWidget::getProfile(entryIndex));
		QStringList toolTip;

		if (profile && profile->getError() != ContentFiltersProfile::NoError)
		{
			switch (profile->getError())
			{
				case ContentFiltersProfile::DownloadError:
					toolTip.append(tr("Failed to download profile rules"));

					break;
				case ContentFiltersProfile::ReadError:
					toolTip.append(tr("Failed to read profile file"));

					break;
				case ContentFiltersProfile::ParseError:
					toolTip.append(tr("Failed to parse profile file"));

					break;
				default:
					break;
			}
		}

		const QUrl updateUrl(entryIndex.data(ContentFiltersViewWidget::UpdateUrlRole).toUrl());
		const QDateTime lastUpdate(entryIndex.data(ContentFiltersViewWidget::UpdateTimeRole).toDateTime());

		if (updateUrl.isValid())
		{
			if (lastUpdate.isNull())
			{
				toolTip.append(tr("Profile was never updated"));
			}
			else if (lastUpdate.daysTo(QDateTime::currentDateTimeUtc()) > 7)
			{
				toolTip.append(tr("Profile was last updated more than one week ago"));
			}
		}
		else
		{
			toolTip.append(tr("No update URL"));
		}

		if (!toolTip.isEmpty())
		{
			toolTip.prepend(displayText(index.data(Qt::DisplayRole), view->locale()));

			QToolTip::showText(event->globalPos(), toolTip.join(QLatin1Char('\n')), view);

			return true;
		}
	}

	return ItemDelegate::helpEvent(event, view, option, index);
}

ContentFiltersIntervalDelegate::ContentFiltersIntervalDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ContentFiltersIntervalDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	const QSpinBox *widget(qobject_cast<QSpinBox*>(editor));

	if (widget && widget->value() != index.data(Qt::DisplayRole).toInt())
	{
		const QModelIndex entryIndex(index.sibling(index.row(), 0));

		model->setData(index, widget->value(), Qt::DisplayRole);
		model->setData(entryIndex, true, ContentFiltersViewWidget::IsModifiedRole);

		emit model->dataChanged(entryIndex, entryIndex, {ContentFiltersViewWidget::IsModifiedRole});
	}
}

QWidget* ContentFiltersIntervalDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QSpinBox *widget(new QSpinBox(parent));
	widget->setSuffix(tr(" day(s)"));
	widget->setSpecialValueText(tr("Never"));
	widget->setMinimum(0);
	widget->setMaximum(365);
	widget->setValue(index.data(Qt::DisplayRole).toInt());
	widget->setFocus();

	return widget;
}

QString ContentFiltersIntervalDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
	Q_UNUSED(locale)

	if (value.isNull())
	{
		return {};
	}

	const int updateInterval(value.toInt());

	return ((updateInterval > 0) ? tr("%n day(s)", "", updateInterval) : tr("Never"));
}

Animation* ContentFiltersViewWidget::m_updateAnimation = nullptr;

ContentFiltersViewWidget::ContentFiltersViewWidget(QWidget *parent) : ItemViewWidget(parent),
	m_model(new ItemModel(this)),
	m_areProfilesModified(false)
{
	setModel(m_model);
	setViewMode(ItemViewWidget::TreeView);
	setItemDelegateForColumn(0, new ContentFiltersTitleDelegate(this));
	setItemDelegateForColumn(1, new ContentFiltersIntervalDelegate(this));
	getViewportWidget()->setMouseTracking(true);
	getViewportWidget()->setUpdateDataRole(IsShowingProgressIndicatorRole);

	m_model->setHorizontalHeaderLabels({tr("Title"), tr("Update Interval"), tr("Last Update"), tr("Active Rules"), tr("All Rules")});
	m_model->setHeaderData(0, Qt::Horizontal, true, HeaderViewWidget::IsShowingCheckBoxIndicatorRole);
	m_model->setHeaderData(0, Qt::Horizontal, 250, HeaderViewWidget::WidthRole);

	QHash<ContentFiltersProfile::ProfileCategory, QList<QList<QStandardItem*> > > categoryEntries;
	const QVector<ContentFiltersProfile*> contentBlockingProfiles(ContentFiltersManager::getContentBlockingProfiles());
	const QStringList profiles(SettingsManager::getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList());

	for (int i = 0; i < contentBlockingProfiles.count(); ++i)
	{
		const ContentFiltersProfile *profile(contentBlockingProfiles.at(i));
		QList<QStandardItem*> profileItems(createEntry(profile->getProfileSummary(), profiles, false));
		const ContentFiltersProfile::ProfileCategory category(profile->getCategory());

		if (!profileItems.isEmpty())
		{
			const QHash<AdblockContentFiltersProfile::RuleType, quint32> information(getRulesInformation(profile->getProfileSummary(), profile->getPath()));

			profileItems[0]->setData(createLanguagesList(profile), LanguagesRole);
			profileItems[3]->setText(QString::number(information.value(AdblockContentFiltersProfile::ActiveRule)));
			profileItems[4]->setText(QString::number(information.value(AdblockContentFiltersProfile::AnyRule)));

			if (!categoryEntries.contains(category))
			{
				categoryEntries[category] = {};
			}

			categoryEntries[category].append(profileItems);
		}
	}

	const QVector<QPair<ContentFiltersProfile::ProfileCategory, QString> > categories({{ContentFiltersProfile::AdvertisementsCategory, tr("Advertisements")}, {ContentFiltersProfile::AnnoyanceCategory, tr("Annoyance")}, {ContentFiltersProfile::PrivacyCategory, tr("Privacy")}, {ContentFiltersProfile::SocialCategory, tr("Social")}, {ContentFiltersProfile::RegionalCategory, tr("Regional")}, {ContentFiltersProfile::OtherCategory, tr("Other")}});

	for (int i = 0; i < categories.count(); ++i)
	{
		const ContentFiltersProfile::ProfileCategory category(categories.at(i).first);
		QList<QStandardItem*> categoryItems({new QStandardItem(categories.at(i).second), new QStandardItem(), new QStandardItem()});
		categoryItems[0]->setData(category, CategoryRole);
		categoryItems[0]->setData(false, IsShowingProgressIndicatorRole);
		categoryItems[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		categoryItems[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		categoryItems[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		m_model->appendRow(categoryItems);

		if (categoryEntries.contains(category))
		{
			const QList<QList<QStandardItem*> > profileItems(categoryEntries[category]);

			for (int j = 0; j < profileItems.count(); ++j)
			{
				categoryItems[0]->appendRow(profileItems.at(j));
			}
		}
		else
		{
			setRowHidden(i, m_model->invisibleRootItem()->index(), true);
		}
	}

	expandAll();

	connect(m_model, &QStandardItemModel::dataChanged, this, &ContentFiltersViewWidget::handleDataChanged);
	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &ContentFiltersViewWidget::updateSelection);
	connect(ContentFiltersManager::getInstance(), &ContentFiltersManager::profileAdded, this, &ContentFiltersViewWidget::handleProfileAdded);
	connect(ContentFiltersManager::getInstance(), &ContentFiltersManager::profileModified, this, &ContentFiltersViewWidget::handleProfileModified);
	connect(ContentFiltersManager::getInstance(), &ContentFiltersManager::profileRemoved, this, &ContentFiltersViewWidget::handleProfileRemoved);
}

void ContentFiltersViewWidget::changeEvent(QEvent *event)
{
	ItemViewWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_model->setHorizontalHeaderLabels({tr("Title"), tr("Update Interval"), tr("Last Update"), tr("Active Rules"), tr("All Rules")});

		for (int i = 0; i < getRowCount(); ++i)
		{
			const QModelIndex categoryIndex(getIndex(i));

			if (categoryIndex.data(CategoryRole).toInt() == ContentFiltersProfile::RegionalCategory)
			{
				for (int j = 0; j < getRowCount(categoryIndex); ++j)
				{
					const QModelIndex entryIndex(getIndex(j, 0, categoryIndex));
					ContentFiltersProfile *profile(getProfile(entryIndex));

					if (profile)
					{
						m_model->setData(entryIndex, createLanguagesList(profile), LanguagesRole);
					}
				}
			}
		}
	}
}

void ContentFiltersViewWidget::contextMenuEvent(QContextMenuEvent *event)
{
	const QModelIndex index(currentIndex().sibling(currentIndex().row(), 0));
	QMenu menu(this);
	QMenu *addMenu(menu.addMenu(tr("Add")));
	addMenu->addAction(tr("New…"), this, &ContentFiltersViewWidget::addProfile);
	addMenu->addAction(tr("File…"), this, &ContentFiltersViewWidget::importProfileFromFile);
	addMenu->addAction(tr("URL…"), this, &ContentFiltersViewWidget::importProfileFromUrl);

	if (index.isValid() && index.flags().testFlag(Qt::ItemNeverHasChildren))
	{
		menu.addSeparator();
		menu.addAction(tr("Edit…"), this, &ContentFiltersViewWidget::editProfile);
		menu.addAction(tr("Update"), this, &ContentFiltersViewWidget::updateProfile)->setEnabled(index.isValid() && index.data(UpdateUrlRole).toUrl().isValid());
		menu.addSeparator();
		menu.addAction(ThemesManager::createIcon(QLatin1String("edit-delete")), tr("Remove"), this, &ContentFiltersViewWidget::removeProfile);
	}

	menu.exec(event->globalPos());
}

void ContentFiltersViewWidget::markProfilesAsModified()
{
	if (!m_areProfilesModified)
	{
		m_areProfilesModified = true;

		emit areProfilesModifiedChanged(true);
	}
}

void ContentFiltersViewWidget::appendProfile(const QList<QStandardItem*> &items, ContentFiltersProfile::ProfileCategory category)
{
	for (int i = 0; i < getRowCount(); ++i)
	{
		QStandardItem *categoryItem(m_model->item(i));

		if (categoryItem && categoryItem->data(CategoryRole).toInt() == category)
		{
			categoryItem->appendRow(items);

			if (getRowCount(categoryItem->index()) == 1)
			{
				setRowHidden(i, m_model->invisibleRootItem()->index(), false);
				expand(categoryItem->index());
			}

			break;
		}
	}
}

void ContentFiltersViewWidget::moveProfile(QStandardItem *entryItem, ContentFiltersProfile::ProfileCategory newCategory)
{
	QStandardItem *currentCategoryItem(entryItem->parent());

	if (currentCategoryItem)
	{
		appendProfile(currentCategoryItem->takeRow(entryItem->row()), newCategory);

		if (getRowCount(currentCategoryItem->index()) == 0)
		{
			setRowHidden(currentCategoryItem->row(), m_model->invisibleRootItem()->index(), true);
		}
	}
}

void ContentFiltersViewWidget::addProfile()
{
	const QModelIndex index(currentIndex().sibling(currentIndex().row(), 0));
	ContentFiltersProfile::ProfileSummary profileSummary;
	profileSummary.category = getCategory(index);

	ContentBlockingProfileDialog dialog(profileSummary, {}, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		profileSummary = dialog.getProfile();
		profileSummary.name = Utils::createIdentifier(QFileInfo(profileSummary.updateUrl.fileName()).completeBaseName(), getProfileNames());

		QList<QStandardItem*> items(createEntry(profileSummary));

		if (!dialog.getRulesPath().isEmpty())
		{
			m_filesToRemove.append(dialog.getRulesPath());

			items[0]->setData(dialog.getRulesPath(), ImportPathRole);
		}

		appendProfile(items, profileSummary.category);
		markProfilesAsModified();
	}
}

void ContentFiltersViewWidget::importProfileFromFile()
{
	const QString path(QFileDialog::getOpenFileName(this, tr("Select File"), Utils::getStandardLocation(QStandardPaths::HomeLocation), tr("AdBlock files (*.txt)")));

	if (path.isEmpty())
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to open content blocking profile file:\n%1").arg(file.errorString()));

		return;
	}

	const AdblockContentFiltersProfile::HeaderInformation information(AdblockContentFiltersProfile::loadHeader(&file));

	file.close();

	if (information.hasError())
	{
		QMessageBox::critical(this, tr("Error"), information.errorString);

		return;
	}

	const QModelIndex index(currentIndex().sibling(currentIndex().row(), 0));
	ContentFiltersProfile::ProfileSummary profileSummary;
	profileSummary.title = information.title;
	profileSummary.category = getCategory(index);

	ContentBlockingProfileDialog dialog(profileSummary, path, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		profileSummary = dialog.getProfile();
		profileSummary.name = Utils::createIdentifier(QFileInfo(path).completeBaseName(), getProfileNames());

		QList<QStandardItem*> items(createEntry(profileSummary));
		items[0]->setData(path, ImportPathRole);

		appendProfile(items, profileSummary.category);
		markProfilesAsModified();
	}
}

void ContentFiltersViewWidget::importProfileFromUrl()
{
	const QUrl url(QInputDialog::getText(this, tr("Select URL"), tr("Enter URL:")));

	if (url.isEmpty())
	{
		return;
	}

	if (!url.isValid())
	{
		QMessageBox::critical(this, tr("Error"), tr("Invaid URL"), QMessageBox::Close);

		return;
	}

	const QModelIndex index(currentIndex().sibling(currentIndex().row(), 0));
	ContentFiltersProfile::ProfileSummary profileSummary;
	profileSummary.updateUrl = url;
	profileSummary.category = getCategory(index);

	ContentBlockingProfileDialog dialog(profileSummary, {}, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		profileSummary = dialog.getProfile();
		profileSummary.name = Utils::createIdentifier(QFileInfo(profileSummary.updateUrl.fileName()).completeBaseName(), getProfileNames());

		QList<QStandardItem*> items(createEntry(profileSummary));
		items[0]->setData(dialog.getRulesPath(), ImportPathRole);

		m_filesToRemove.append(dialog.getRulesPath());

		appendProfile(items, profileSummary.category);
		markProfilesAsModified();
	}
}

void ContentFiltersViewWidget::editProfile()
{
	const QModelIndex index(currentIndex().sibling(currentIndex().row(), 0));
	QStandardItem *item(m_model->itemFromIndex(index));

	if (!item)
	{
		return;
	}

	const QString path(getProfilePath(index));
	ContentFiltersProfile::ProfileSummary profileSummary(getProfileSummary(index));
	ContentBlockingProfileDialog dialog(profileSummary, path, this);

	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}

	profileSummary = dialog.getProfile();

	const QHash<AdblockContentFiltersProfile::RuleType, quint32> information(getRulesInformation(profileSummary, path));
	const int row(index.row());

	m_model->setData(index, true, IsModifiedRole);
	m_model->setData(index, profileSummary.title, TitleRole);
	m_model->setData(index, profileSummary.updateUrl, UpdateUrlRole);
	m_model->setData(index, profileSummary.cosmeticFiltersMode, CosmeticFiltersModeRole);
	m_model->setData(index, profileSummary.areWildcardsEnabled, AreWildcardsEnabledRole);
	m_model->setData(index.sibling(row, 1), profileSummary.updateInterval, Qt::DisplayRole);
	m_model->setData(index.sibling(row, 1), profileSummary.updateUrl, UpdateUrlRole);
	m_model->setData(index.sibling(row, 2), profileSummary.updateUrl, UpdateUrlRole);
	m_model->setData(index.sibling(row, 3), QString::number(information.value(AdblockContentFiltersProfile::ActiveRule)), Qt::DisplayRole);
	m_model->setData(index.sibling(row, 3), profileSummary.updateUrl, UpdateUrlRole);
	m_model->setData(index.sibling(row, 4), QString::number(information.value(AdblockContentFiltersProfile::AnyRule)), Qt::DisplayRole);
	m_model->setData(index.sibling(row, 4), profileSummary.updateUrl, UpdateUrlRole);

	if (index.parent().data(CategoryRole).toInt() != profileSummary.category)
	{
		moveProfile(item, profileSummary.category);
	}

	markProfilesAsModified();
}

void ContentFiltersViewWidget::removeProfile()
{
	const QModelIndex index(currentIndex().sibling(currentIndex().row(), 0));
	ContentFiltersProfile *profile(getProfile(index));
	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("Do you really want to remove this profile?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Cancel);

	if (profile && QFile::exists(profile->getPath()))
	{
		messageBox.setCheckBox(new QCheckBox(tr("Delete profile permanently")));
	}

	if (messageBox.exec() != QMessageBox::Yes)
	{
		return;
	}

	m_profilesToRemove[profile->getName()] = (messageBox.checkBox() && messageBox.checkBox()->isChecked());

	QStandardItem *categoryItem(m_model->itemFromIndex(index.parent()));

	if (categoryItem)
	{
		categoryItem->removeRow(index.row());

		if (getRowCount(categoryItem->index()) == 0)
		{
			setRowHidden(categoryItem->row(), m_model->invisibleRootItem()->index(), true);
		}
	}

	markProfilesAsModified();
}

void ContentFiltersViewWidget::updateProfile()
{
	ContentFiltersProfile *profile(getProfile(currentIndex()));

	if (!profile)
	{
		return;
	}

	if (!m_updateAnimation)
	{
		m_updateAnimation = ThemesManager::createAnimation();
		m_updateAnimation->start();

		getViewportWidget()->updateDirtyIndexesList();
	}

	profile->update();
}

void ContentFiltersViewWidget::updateSelection()
{
	const QModelIndex index(currentIndex().sibling(currentIndex().row(), 0));

	if (index.column() != 1)
	{
		setCurrentIndex(index.sibling(index.row(), 1));
	}
}

void ContentFiltersViewWidget::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
	Q_UNUSED(topLeft)
	Q_UNUSED(bottomRight)

	if (roles.contains(Qt::CheckStateRole) || roles.contains(IsModifiedRole))
	{
		markProfilesAsModified();
	}
}

void ContentFiltersViewWidget::handleProfileAdded(const QString &name)
{
	const ContentFiltersProfile *profile(ContentFiltersManager::getProfile(name));

	if (!profile)
	{
		return;
	}

	for (int i = 0; i < getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(getIndex(i));

		if (categoryIndex.data(CategoryRole).toInt() == profile->getCategory())
		{
			QList<QStandardItem*> profileItems(createEntry(profile->getProfileSummary(), {}, false));

			if (!profileItems.isEmpty())
			{
				QStandardItem *categoryItem(m_model->itemFromIndex(categoryIndex));

				if (categoryItem)
				{
					const QHash<AdblockContentFiltersProfile::RuleType, quint32> information(getRulesInformation(profile->getProfileSummary(), profile->getPath()));

					profileItems[0]->setData(createLanguagesList(profile), LanguagesRole);
					profileItems[3]->setText(QString::number(information.value(AdblockContentFiltersProfile::ActiveRule)));
					profileItems[4]->setText(QString::number(information.value(AdblockContentFiltersProfile::AnyRule)));

					categoryItem->appendRow(profileItems);

					if (getRowCount(categoryItem->index()) == 1)
					{
						setRowHidden(i, m_model->invisibleRootItem()->index(), false);
						expand(categoryItem->index());
					}
				}
			}

			break;
		}
	}
}

void ContentFiltersViewWidget::handleProfileModified(const QString &name)
{
	const ContentFiltersProfile *profile(ContentFiltersManager::getProfile(name));

	if (!profile)
	{
		return;
	}

	QStandardItem *entryItem(nullptr);

	for (int i = 0; i < getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(getIndex(i));
		bool hasFound(false);

		for (int j = 0; j < getRowCount(categoryIndex); ++j)
		{
			const QModelIndex entryIndex(getIndex(j, 0, categoryIndex));

			if (entryIndex.data(NameRole).toString() == name)
			{
				entryItem = m_model->itemFromIndex(entryIndex);
				hasFound = true;

				setData(entryIndex.sibling(j, 2), Utils::formatDateTime(profile->getLastUpdate()), Qt::DisplayRole);

				if (!entryIndex.data(IsModifiedRole).toBool())
				{
					setData(entryIndex, profile->getTitle(), TitleRole);
					setData(entryIndex, createLanguagesList(profile), LanguagesRole);
					setData(entryIndex.sibling(j, 1), profile->getUpdateInterval(), Qt::DisplayRole);
				}

				break;
			}
		}

		if (hasFound)
		{
			break;
		}
	}

	if (entryItem)
	{
		const bool hasError(profile->getError() != ContentFiltersProfile::NoError);

		entryItem->setData(hasError, HasErrorRole);
		entryItem->setData(profile->getLastUpdate(), UpdateTimeRole);

		if (profile->isUpdating() != entryItem->data(IsUpdatingRole).toBool())
		{
			entryItem->setData(profile->isUpdating(), IsUpdatingRole);

			if (profile->isUpdating())
			{
				entryItem->setData(true, IsShowingProgressIndicatorRole);

				connect(profile, &ContentFiltersProfile::updateProgressChanged, profile, [=](int progress)
				{
					entryItem->setData(((progress < 0 && entryItem->data(HasErrorRole).toBool()) ? 0 : progress), UpdateProgressValueRole);
				});
			}
			else
			{
				if (entryItem->data(UpdateProgressValueRole).toInt() < 0)
				{
					entryItem->setData((hasError ? 0 : 100), UpdateProgressValueRole);
				}

				const QModelIndex entryIndex(entryItem->index());
				const QHash<AdblockContentFiltersProfile::RuleType, quint32> information(getRulesInformation(getProfileSummary(entryIndex), getProfilePath(entryIndex)));

				m_model->setData(entryIndex.sibling(entryIndex.row(), 3), QString::number(information.value(AdblockContentFiltersProfile::ActiveRule)), Qt::DisplayRole);
				m_model->setData(entryIndex.sibling(entryIndex.row(), 4), QString::number(information.value(AdblockContentFiltersProfile::AnyRule)), Qt::DisplayRole);

				QTimer::singleShot(2500, this, [=]()
				{
					if (!profile->isUpdating())
					{
						entryItem->setData(false, IsShowingProgressIndicatorRole);
					}
				});
			}
		}

		if (!entryItem->data(IsModifiedRole).toBool())
		{
			QStandardItem *currentCategoryItem(entryItem->parent());

			if (currentCategoryItem && profile->getCategory() != currentCategoryItem->data(CategoryRole).toInt())
			{
				moveProfile(entryItem, profile->getCategory());
			}
		}
	}

	viewport()->update();
}

void ContentFiltersViewWidget::handleProfileRemoved(const QString &name)
{
	for (int i = 0; i < getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(getIndex(i));
		bool hasFound(false);

		for (int j = 0; j < getRowCount(categoryIndex); ++j)
		{
			const QModelIndex entryIndex(getIndex(j, 0, categoryIndex));

			if (entryIndex.data(NameRole).toString() == name)
			{
				hasFound = true;

				m_model->removeRow(entryIndex.row(), entryIndex.parent());

				if (getRowCount(entryIndex.parent()) == 0)
				{
					setRowHidden(i, m_model->invisibleRootItem()->index(), true);
				}

				break;
			}
		}

		if (hasFound)
		{
			break;
		}
	}
}

void ContentFiltersViewWidget::setHost(const QString &host)
{
	if (host == m_host)
	{
		return;
	}

	disconnect(m_model, &QStandardItemModel::dataChanged, this, &ContentFiltersViewWidget::handleDataChanged);

	const QStringList profiles(SettingsManager::getOption(SettingsManager::ContentBlocking_ProfilesOption, host).toStringList());

	m_host = host;

	for (int i = 0; i < getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(getIndex(i));

		for (int j = 0; j < getRowCount(categoryIndex); ++j)
		{
			const QModelIndex entryIndex(getIndex(j, 0, categoryIndex));

			m_model->setData(entryIndex, (profiles.contains(entryIndex.data(NameRole).toString()) ? Qt::Checked : Qt::Unchecked), Qt::CheckStateRole);
		}
	}

	connect(m_model, &QStandardItemModel::dataChanged, this, &ContentFiltersViewWidget::handleDataChanged);

	if (m_areProfilesModified && m_profilesToRemove.isEmpty() && m_model->match(m_model->index(0, 0), IsModifiedRole, true, 1, (Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap)).isEmpty())
	{
		m_areProfilesModified = false;

		emit areProfilesModifiedChanged(false);
	}
}

void ContentFiltersViewWidget::save()
{
	QHash<QString, bool>::iterator iterator;

	for (iterator = m_profilesToRemove.begin(); iterator != m_profilesToRemove.end(); ++iterator)
	{
		ContentFiltersProfile *profile(ContentFiltersManager::getProfile(iterator.key()));

		if (profile)
		{
			ContentFiltersManager::removeProfile(profile, iterator.value());
		}
	}

	m_profilesToRemove.clear();

	QStringList profiles;

	for (int i = 0; i < getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(getIndex(i));

		for (int j = 0; j < getRowCount(categoryIndex); ++j)
		{
			const QModelIndex entryIndex(getIndex(j, 0, categoryIndex));

			if (static_cast<Qt::CheckState>(entryIndex.data(Qt::CheckStateRole).toInt()) == Qt::Checked)
			{
				profiles.append(entryIndex.data(NameRole).toString());
			}

			if (!entryIndex.data(IsModifiedRole).toBool())
			{
				continue;
			}

			const ContentFiltersProfile::ProfileSummary profileSummary(getProfileSummary(entryIndex));
			ContentFiltersProfile *profile(ContentFiltersManager::getProfile(profileSummary.name));

			if (profile)
			{
				profile->setProfileSummary(profileSummary);

				continue;
			}

			const QString importPath(entryIndex.data(ImportPathRole).toString());

			if (importPath.isEmpty() && !AdblockContentFiltersProfile::create(profileSummary))
			{
				QMessageBox::critical(this, tr("Error"), tr("Failed to create profile file."), QMessageBox::Close);

				continue;
			}

			if (!importPath.isEmpty())
			{
				if (!QFile::exists(importPath))
				{
					QMessageBox::critical(this, tr("Error"), tr("Rules file does not exist."), QMessageBox::Close);

					continue;
				}

				QFile file(importPath);

				if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					QMessageBox::critical(this, tr("Error"), tr("Failed to create profile file."), QMessageBox::Close);

					continue;
				}

				const bool isSuccess(AdblockContentFiltersProfile::create(profileSummary, &file));

				file.close();

				if (!isSuccess)
				{
					QMessageBox::critical(this, tr("Error"), tr("Failed to create profile file."), QMessageBox::Close);

					continue;
				}
			}

			m_model->setData(entryIndex, false, IsModifiedRole);
		}
	}

	Utils::removeFiles(m_filesToRemove);

	m_filesToRemove.clear();

	SettingsManager::setOption(SettingsManager::ContentBlocking_ProfilesOption, profiles, m_host);

	m_areProfilesModified = false;

	emit areProfilesModifiedChanged(false);
}

ContentFiltersProfile* ContentFiltersViewWidget::getProfile(const QModelIndex &index)
{
	return ContentFiltersManager::getProfile(index.sibling(index.row(), 0).data(NameRole).toString());
}

Animation* ContentFiltersViewWidget::getUpdateAnimation()
{
	return m_updateAnimation;
}

QString ContentFiltersViewWidget::getProfilePath(const QModelIndex &index) const
{
	const ContentFiltersProfile *profile(getProfile(index));

	return (profile ? profile->getPath() : index.data(ImportPathRole).toString());
}

QString ContentFiltersViewWidget::getHost() const
{
	return m_host;
}

ContentFiltersProfile::ProfileSummary ContentFiltersViewWidget::getProfileSummary(const QModelIndex &index) const
{
	ContentFiltersProfile::ProfileSummary profileSummary;
	profileSummary.name = index.data(NameRole).toString();
	profileSummary.title = index.data(TitleRole).toString();
	profileSummary.lastUpdate = index.data(UpdateTimeRole).toDateTime();
	profileSummary.updateUrl = index.data(UpdateUrlRole).toUrl();
	profileSummary.category = static_cast<ContentFiltersProfile::ProfileCategory>(index.parent().data(CategoryRole).toInt());
	profileSummary.cosmeticFiltersMode = static_cast<ContentFiltersManager::CosmeticFiltersMode>(index.data(CosmeticFiltersModeRole).toInt());
	profileSummary.updateInterval = index.sibling(index.row(), 1).data(Qt::DisplayRole).toInt();
	profileSummary.areWildcardsEnabled = index.data(AreWildcardsEnabledRole).toBool();

	return profileSummary;
}

QHash<AdblockContentFiltersProfile::RuleType, quint32> ContentFiltersViewWidget::getRulesInformation(const ContentFiltersProfile::ProfileSummary &profileSummary, const QString &path)
{
	QHash<AdblockContentFiltersProfile::RuleType, quint32> information;
	QFile file(path);

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		information = AdblockContentFiltersProfile::loadRulesInformation(profileSummary, &file);

		file.close();
	}

	return information;
}

QStringList ContentFiltersViewWidget::createLanguagesList(const ContentFiltersProfile *profile) const
{
	const QVector<QLocale::Language> languages(profile->getLanguages());
	QStringList languageNames;
	languageNames.reserve(languages.count());

	for (int i = 0; i < languages.count(); ++i)
	{
		languageNames.append(QLocale::languageToString(languages.at(i)));
	}

	return languageNames;
}

QStringList ContentFiltersViewWidget::getProfileNames() const
{
	QStringList profiles(ContentFiltersManager::getProfileNames());

	for (int i = 0; i < getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(getIndex(i));

		profiles.reserve(profiles.count() + getRowCount(categoryIndex));

		for (int j = 0; j < getRowCount(categoryIndex); ++j)
		{
			const QString name(getIndex(j, 0, categoryIndex).data(NameRole).toString());

			if (!profiles.contains(name))
			{
				profiles.append(name);
			}
		}
	}

	return profiles;
}

QList<QStandardItem*> ContentFiltersViewWidget::createEntry(const ContentFiltersProfile::ProfileSummary &profileSummary, const QStringList &profiles, bool isModified) const
{
	QList<QStandardItem*> items({new QStandardItem(profileSummary.title), new QStandardItem(QString::number(profileSummary.updateInterval)), new QStandardItem(Utils::formatDateTime(profileSummary.lastUpdate)), new QStandardItem(), new QStandardItem()});
	items[0]->setData(profileSummary.name, NameRole);
	items[0]->setData(profileSummary.areWildcardsEnabled, AreWildcardsEnabledRole);
	items[0]->setData(profileSummary.cosmeticFiltersMode, CosmeticFiltersModeRole);
	items[0]->setData(false, HasErrorRole);
	items[0]->setData(isModified, IsModifiedRole);
	items[0]->setData(false, IsShowingProgressIndicatorRole);
	items[0]->setData(false, IsUpdatingRole);
	items[0]->setData(-1, UpdateProgressValueRole);
	items[0]->setData(profileSummary.lastUpdate, UpdateTimeRole);
	items[0]->setData(profileSummary.updateUrl, UpdateUrlRole);
	items[0]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	items[0]->setCheckState(profiles.contains(profileSummary.name) ? Qt::Checked : Qt::Unchecked);
	items[0]->setCheckable(true);
	items[1]->setData(profileSummary.updateUrl, UpdateUrlRole);
	items[1]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	items[2]->setData(profileSummary.updateUrl, UpdateUrlRole);
	items[2]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	items[3]->setData(profileSummary.updateUrl, UpdateUrlRole);
	items[3]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	items[4]->setData(profileSummary.updateUrl, UpdateUrlRole);
	items[4]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	return items;
}

ContentFiltersProfile::ProfileCategory ContentFiltersViewWidget::getCategory(const QModelIndex &index) const
{
	if (!index.data(CategoryRole).isNull())
	{
		return static_cast<ContentFiltersProfile::ProfileCategory>(index.data(CategoryRole).toInt());
	}

	if (!index.parent().data(CategoryRole).isNull())
	{
		return static_cast<ContentFiltersProfile::ProfileCategory>(index.parent().data(CategoryRole).toInt());
	}

	return ContentFiltersProfile::OtherCategory;
}

bool ContentFiltersViewWidget::areProfilesModified() const
{
	return m_areProfilesModified;
}

}
