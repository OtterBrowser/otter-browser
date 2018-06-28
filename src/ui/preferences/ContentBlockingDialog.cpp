/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ContentBlockingDialog.h"
#include "ContentBlockingProfileDialog.h"
#include "../Animation.h"
#include "../../core/AdblockContentFiltersProfile.h"
#include "../../core/Console.h"
#include "../../core/ContentFiltersManager.h"
#include "../../core/SessionsManager.h"
#include "../../core/SettingsManager.h"
#include "../../core/ThemesManager.h"
#include "../../core/Utils.h"

#include "ui_ContentBlockingDialog.h"

#include <QtCore/QDir>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolTip>

namespace Otter
{

ContentBlockingTitleDelegate::ContentBlockingTitleDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ContentBlockingTitleDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	const ContentFiltersProfile *profile(ContentFiltersManager::getProfile(index.data(ContentFiltersManager::NameRole).toString()));

	if (profile)
	{
		option->features |= QStyleOptionViewItem::HasDecoration;

		if (profile->isUpdating())
		{
			const Animation *animation(ContentBlockingDialog::getUpdateAnimation());

			if (animation)
			{
				option->icon = QIcon(animation->getCurrentPixmap());
			}
		}
		else if (profile->getError() != ContentFiltersProfile::NoError)
		{
			option->icon = ThemesManager::createIcon(QLatin1String("dialog-error"));
		}
		else if (profile->getLastUpdate().isNull() || profile->getLastUpdate().daysTo(QDateTime::currentDateTimeUtc()) > 7)
		{
			option->icon = ThemesManager::createIcon(QLatin1String("dialog-warning"));
		}
	}
}

bool ContentBlockingTitleDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	if (event->type() == QEvent::ToolTip)
	{
		const ContentFiltersProfile *profile(ContentFiltersManager::getProfile(index.data(ContentFiltersManager::NameRole).toString()));

		if (profile)
		{
			QString toolTip;

			if (profile->getError() != ContentFiltersProfile::NoError)
			{
				switch (profile->getError())
				{
					case ContentFiltersProfile::ReadError:
						toolTip = tr("Failed to read profile file");

						break;
					case ContentFiltersProfile::DownloadError:
						toolTip = tr("Failed to download profile rules");

						break;
					case ContentFiltersProfile::ChecksumError:
						toolTip = tr("Failed to verify profile rules using checksum");

						break;
					default:
						break;
				}
			}
			else if (profile->getLastUpdate().isNull())
			{
				toolTip = tr("Profile was never updated");
			}
			else if (profile->getLastUpdate().daysTo(QDateTime::currentDateTimeUtc()) > 7)
			{
				toolTip = tr("Profile was last updated more than one week ago");
			}

			if (!toolTip.isEmpty())
			{
				QToolTip::showText(event->globalPos(), displayText(index.data(Qt::DisplayRole), view->locale()) + QLatin1Char('\n') + toolTip, view);

				return true;
			}
		}
	}

	return ItemDelegate::helpEvent(event, view, option, index);
}

ContentBlockingIntervalDelegate::ContentBlockingIntervalDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void ContentBlockingIntervalDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	const QSpinBox *widget(qobject_cast<QSpinBox*>(editor));

	if (widget)
	{
		model->setData(index, widget->value(), Qt::DisplayRole);
	}
}

QWidget* ContentBlockingIntervalDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	QSpinBox *widget(new QSpinBox(parent));
	widget->setSuffix(QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", " day(s)"));
	widget->setSpecialValueText(QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", "Never"));
	widget->setMinimum(0);
	widget->setMaximum(365);
	widget->setValue(index.data(Qt::DisplayRole).toInt());
	widget->setFocus();

	return widget;
}

QString ContentBlockingIntervalDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
	Q_UNUSED(locale)

	if (value.isNull())
	{
		return {};
	}

	const int updateInterval(value.toInt());

	return ((updateInterval > 0) ? QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", "%n day(s)", "", updateInterval) : QCoreApplication::translate("Otter::ContentBlockingIntervalDelegate", "Never"));
}

Animation* ContentBlockingDialog::m_updateAnimation = nullptr;

ContentBlockingDialog::ContentBlockingDialog(QWidget *parent) : Dialog(parent),
	m_ui(new Ui::ContentBlockingDialog)
{
	m_ui->setupUi(this);

	const QStringList globalProfiles(SettingsManager::getOption(SettingsManager::ContentBlocking_ProfilesOption).toStringList());

	m_ui->profilesViewWidget->setModel(ContentFiltersManager::createModel(this, globalProfiles));
	m_ui->profilesViewWidget->setItemDelegateForColumn(0, new ContentBlockingTitleDelegate(this));
	m_ui->profilesViewWidget->setItemDelegateForColumn(1, new ContentBlockingIntervalDelegate(this));
	m_ui->profilesViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->profilesViewWidget->expandAll();
	m_ui->cosmeticFiltersComboBox->addItem(tr("All"), QLatin1String("all"));
	m_ui->cosmeticFiltersComboBox->addItem(tr("Domain specific only"), QLatin1String("domainOnly"));
	m_ui->cosmeticFiltersComboBox->addItem(tr("None"), QLatin1String("none"));

	const int cosmeticFiltersIndex(m_ui->cosmeticFiltersComboBox->findData(SettingsManager::getOption(SettingsManager::ContentBlocking_CosmeticFiltersModeOption).toString()));

	m_ui->cosmeticFiltersComboBox->setCurrentIndex((cosmeticFiltersIndex < 0) ? 0 : cosmeticFiltersIndex);
	m_ui->enableCustomRulesCheckBox->setChecked(globalProfiles.contains(QLatin1String("custom")));

	QStandardItemModel *customRulesModel(new QStandardItemModel(this));
	QFile file(SessionsManager::getWritableDataPath("contentBlocking/custom.txt"));

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream stream(&file);

		if (stream.readLine().trimmed().startsWith(QLatin1String("[Adblock Plus"), Qt::CaseInsensitive))
		{
			while (!stream.atEnd())
			{
				QStandardItem *item(new QStandardItem(stream.readLine().trimmed()));
				item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

				customRulesModel->appendRow(item);
			}
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to load custom rules: invalid adblock header"), Console::OtherCategory, Console::ErrorLevel, file.fileName());
		}

		file.close();
	}

	m_ui->customRulesViewWidget->setModel(customRulesModel);
	m_ui->enableWildcardsCheckBox->setChecked(SettingsManager::getOption(SettingsManager::ContentBlocking_EnableWildcardsOption).toBool());

	connect(ContentFiltersManager::getInstance(), &ContentFiltersManager::profileModified, this, &ContentBlockingDialog::handleProfileModified);
	connect(m_ui->profilesViewWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, &ContentBlockingDialog::updateProfilesActions);
	connect(m_ui->addProfileButton, &QPushButton::clicked, this, &ContentBlockingDialog::addProfile);
	connect(m_ui->editProfileButton, &QPushButton::clicked, this, &ContentBlockingDialog::editProfile);
	connect(m_ui->updateProfileButton, &QPushButton::clicked, this, &ContentBlockingDialog::updateProfile);
	connect(m_ui->removeProfileButton, &QPushButton::clicked, this, &ContentBlockingDialog::removeProfile);
	connect(m_ui->confirmButtonBox, &QDialogButtonBox::accepted, this, &ContentBlockingDialog::save);
	connect(m_ui->confirmButtonBox, &QDialogButtonBox::rejected, this, &ContentBlockingDialog::close);
	connect(m_ui->enableCustomRulesCheckBox, &QCheckBox::toggled, this, &ContentBlockingDialog::updateRulesActions);
	connect(m_ui->customRulesViewWidget, &ItemViewWidget::needsActionsUpdate, this, &ContentBlockingDialog::updateRulesActions);
	connect(m_ui->addRuleButton, &QPushButton::clicked, this, &ContentBlockingDialog::addRule);
	connect(m_ui->editRuleButton, &QPushButton::clicked, this, &ContentBlockingDialog::editRule);
	connect(m_ui->removeRuleButton, &QPushButton::clicked, this, &ContentBlockingDialog::removeRule);

	updateRulesActions();
}

ContentBlockingDialog::~ContentBlockingDialog()
{
	delete m_ui;
}

void ContentBlockingDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void ContentBlockingDialog::addProfile()
{
	ContentBlockingProfileDialog dialog(this);

	if (dialog.exec() == QDialog::Accepted)
	{
		updateModel(dialog.getProfile(), true);
	}
}

void ContentBlockingDialog::editProfile()
{
	const QModelIndex index(m_ui->profilesViewWidget->currentIndex().sibling(m_ui->profilesViewWidget->currentIndex().row(), 0));
	ContentFiltersProfile *profile(ContentFiltersManager::getProfile(index.data(ContentFiltersManager::NameRole).toString()));

	if (profile)
	{
		const ContentFiltersProfile::ProfileCategory category(profile->getCategory());
		ContentBlockingProfileDialog dialog(this, profile);

		if (dialog.exec() == QDialog::Accepted)
		{
			updateModel(profile, (category != profile->getCategory()));
		}
	}
}

void ContentBlockingDialog::removeProfile()
{
	const QModelIndex index(m_ui->profilesViewWidget->currentIndex().sibling(m_ui->profilesViewWidget->currentIndex().row(), 0));
	ContentFiltersProfile *profile(ContentFiltersManager::getProfile(index.data(ContentFiltersManager::NameRole).toString()));

	if (profile)
	{
		ContentFiltersManager::removeProfile(profile);

		m_ui->profilesViewWidget->model()->removeRow(index.row(), index.parent());

		if (m_ui->profilesViewWidget->getRowCount(index.parent()) == 0)
		{
			m_ui->profilesViewWidget->model()->removeRow(index.parent().row(), index.parent().parent());
		}

		m_ui->profilesViewWidget->clearSelection();
	}
}

void ContentBlockingDialog::updateProfile()
{
	const QModelIndex index(m_ui->profilesViewWidget->currentIndex().sibling(m_ui->profilesViewWidget->currentIndex().row(), 0));
	ContentFiltersProfile *profile(ContentFiltersManager::getProfile(index.data(ContentFiltersManager::NameRole).toString()));

	if (!m_updateAnimation)
	{
		const QString path(ThemesManager::getAnimationPath(QLatin1String("spinner")));

		if (path.isEmpty())
		{
			m_updateAnimation = new SpinnerAnimation(this);
		}
		else
		{
			m_updateAnimation = new GenericAnimation(path, this);
		}

		m_updateAnimation->start();

		connect(m_updateAnimation, &Animation::frameChanged, m_ui->profilesViewWidget->viewport(), static_cast<void(QWidget::*)()>(&QWidget::update));
	}

	if (profile)
	{
		profile->update();
	}
}

void ContentBlockingDialog::updateProfilesActions()
{
	const QModelIndex index(m_ui->profilesViewWidget->currentIndex().sibling(m_ui->profilesViewWidget->currentIndex().row(), 0));
	const bool isEditable(index.isValid() && index.flags().testFlag(Qt::ItemNeverHasChildren));

	if (index.column() != 1)
	{
		m_ui->profilesViewWidget->setCurrentIndex(index.sibling(index.row(), 1));
	}

	m_ui->editProfileButton->setEnabled(isEditable);
	m_ui->removeProfileButton->setEnabled(isEditable);
	m_ui->updateProfileButton->setEnabled(index.isValid() && index.data(ContentFiltersManager::UpdateUrlRole).toUrl().isValid());
}

void ContentBlockingDialog::addRule()
{
	m_ui->customRulesViewWidget->insertRow();

	editRule();
}

void ContentBlockingDialog::editRule()
{
	m_ui->customRulesViewWidget->edit(m_ui->customRulesViewWidget->getIndex(m_ui->customRulesViewWidget->getCurrentRow()));
}

void ContentBlockingDialog::removeRule()
{
	m_ui->customRulesViewWidget->removeRow();
	m_ui->customRulesViewWidget->setFocus();

	updateRulesActions();
}

void ContentBlockingDialog::updateRulesActions()
{
	m_ui->tabWidget->setTabEnabled(1, m_ui->enableCustomRulesCheckBox->isChecked());

	const bool isEditable(m_ui->customRulesViewWidget->getCurrentRow() >= 0);

	m_ui->editRuleButton->setEnabled(isEditable);
	m_ui->removeRuleButton->setEnabled(isEditable);
}

void ContentBlockingDialog::updateModel(ContentFiltersProfile *profile, bool isNewOrMoved)
{
	if (isNewOrMoved)
	{
		const QModelIndexList removeList(m_ui->profilesViewWidget->model()->match(m_ui->profilesViewWidget->model()->index(0, 0), ContentFiltersManager::NameRole, QVariant::fromValue(profile->getName()), 2, Qt::MatchRecursive));

		if (removeList.count() > 0)
		{
			m_ui->profilesViewWidget->model()->removeRow(removeList[0].row(), removeList[0].parent());
		}

		QList<QStandardItem*> profileItems({new QStandardItem(profile->getTitle()), new QStandardItem(QString::number(profile->getUpdateInterval())), new QStandardItem(Utils::formatDateTime(profile->getLastUpdate()))});
		profileItems[0]->setData(profile->getName(), ContentFiltersManager::NameRole);
		profileItems[0]->setData(profile->getUpdateUrl(), ContentFiltersManager::UpdateUrlRole);
		profileItems[0]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		profileItems[0]->setCheckable(true);
		profileItems[0]->setCheckState(Qt::Unchecked);
		profileItems[1]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		profileItems[2]->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		const QModelIndexList indexList(m_ui->profilesViewWidget->model()->match(m_ui->profilesViewWidget->model()->index(0, 0), ContentFiltersManager::NameRole, QVariant::fromValue(static_cast<int>(profile->getCategory()))));

		if (indexList.count() > 0)
		{
			QStandardItem *categoryItem(qobject_cast<QStandardItemModel* >(m_ui->profilesViewWidget->model())->itemFromIndex(indexList[0]));
			categoryItem->appendRow(profileItems);
			categoryItem->sortChildren(m_ui->profilesViewWidget->getSortColumn(), m_ui->profilesViewWidget->getSortOrder());
		}

		return;
	}

	const QModelIndex currentIndex(m_ui->profilesViewWidget->currentIndex());

	m_ui->profilesViewWidget->setData(currentIndex.sibling(currentIndex.row(), 0), profile->getTitle(), Qt::DisplayRole);
	m_ui->profilesViewWidget->setData(currentIndex.sibling(currentIndex.row(), 0), profile->getUpdateUrl(), ContentFiltersManager::UpdateUrlRole);
	m_ui->profilesViewWidget->setData(currentIndex.sibling(currentIndex.row(), 1), profile->getUpdateInterval(), Qt::DisplayRole);
	m_ui->profilesViewWidget->setData(currentIndex.sibling(currentIndex.row(), 2), Utils::formatDateTime(profile->getLastUpdate()), Qt::DisplayRole);
}

void ContentBlockingDialog::handleProfileModified(const QString &name)
{
	const ContentFiltersProfile *profile(ContentFiltersManager::getProfile(name));

	if (!profile)
	{
		return;
	}

	m_ui->profilesViewWidget->viewport()->update();

	for (int i = 0; i < m_ui->profilesViewWidget->getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(m_ui->profilesViewWidget->getIndex(i));

		for (int j = 0; j < m_ui->profilesViewWidget->getRowCount(categoryIndex); ++j)
		{
			const QModelIndex entryIndex(m_ui->profilesViewWidget->getIndex(j, 0, categoryIndex));

			if (entryIndex.data(ContentFiltersManager::NameRole).toString() == name)
			{
				QString title(profile->getTitle());

				if (profile->getCategory() == ContentFiltersProfile::RegionalCategory)
				{
					const QVector<QLocale::Language> languages(profile->getLanguages());
					QStringList languageNames;

					for (int k = 0; k < languages.count(); ++k)
					{
						languageNames.append(QLocale::languageToString(languages.at(k)));
					}

					title = QStringLiteral("%1 [%2]").arg(title).arg(languageNames.join(QLatin1String(", ")));
				}

				m_ui->profilesViewWidget->setData(entryIndex, title, Qt::DisplayRole);
				m_ui->profilesViewWidget->setData(entryIndex.sibling(j, 2), Utils::formatDateTime(profile->getLastUpdate()), Qt::DisplayRole);

				return;
			}
		}
	}
}

void ContentBlockingDialog::save()
{
	QStringList profiles;

	for (int i = 0; i < m_ui->profilesViewWidget->getRowCount(); ++i)
	{
		const QModelIndex categoryIndex(m_ui->profilesViewWidget->getIndex(i));

		for (int j = 0; j < m_ui->profilesViewWidget->getRowCount(categoryIndex); ++j)
		{
			const QModelIndex entryIndex(m_ui->profilesViewWidget->getIndex(j, 0, categoryIndex));
			const QModelIndex intervalIndex(m_ui->profilesViewWidget->getIndex(j, 1, categoryIndex));
			const QString name(entryIndex.data(ContentFiltersManager::NameRole).toString());
			ContentFiltersProfile *profile(ContentFiltersManager::getProfile(name));

			if (intervalIndex.data(Qt::EditRole).toInt() != profile->getUpdateInterval())
			{
				profile->setUpdateInterval(intervalIndex.data(Qt::EditRole).toInt());
			}

			if (entryIndex.data(Qt::CheckStateRole).toBool())
			{
				profiles.append(name);
			}
		}
	}

	if (m_ui->enableCustomRulesCheckBox->isChecked())
	{
		QDir().mkpath(SessionsManager::getWritableDataPath(QLatin1String("contentBlocking")));

		QFile file(SessionsManager::getWritableDataPath("contentBlocking/custom.txt"));

		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			file.write(QStringLiteral("[AdBlock Plus 2.0]\n").toUtf8());

			for (int i = 0; i < m_ui->customRulesViewWidget->getRowCount(); ++i)
			{
				file.write(m_ui->customRulesViewWidget->getIndex(i, 0).data().toString().toLatin1() + QStringLiteral("\n").toUtf8());
			}

			file.close();

			ContentFiltersProfile *profile(ContentFiltersManager::getProfile(QLatin1String("custom")));

			if (profile)
			{
				profile->clear();
			}
			else
			{
				profile = new AdblockContentFiltersProfile(QLatin1String("custom"), tr("Custom Rules"), {}, {}, {}, 0, ContentFiltersProfile::OtherCategory, ContentFiltersProfile::NoFlags);

				ContentFiltersManager::addProfile(profile);
			}

			profiles.append(QLatin1String("custom"));
		}
		else
		{
			Console::addMessage(QCoreApplication::translate("main", "Failed to create a file with custom rules: %1").arg(file.errorString()), Console::OtherCategory, Console::ErrorLevel, file.fileName());
		}
	}

	SettingsManager::setOption(SettingsManager::ContentBlocking_ProfilesOption, profiles);
	SettingsManager::setOption(SettingsManager::ContentBlocking_EnableWildcardsOption, m_ui->enableWildcardsCheckBox->isChecked());
	SettingsManager::setOption(SettingsManager::ContentBlocking_CosmeticFiltersModeOption, m_ui->cosmeticFiltersComboBox->currentData().toString());

	close();
}

Animation *ContentBlockingDialog::getUpdateAnimation()
{
	return m_updateAnimation;
}

}
