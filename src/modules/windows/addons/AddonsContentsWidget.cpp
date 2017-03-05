/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#include "AddonsContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/SessionsManager.h"
#include "../../../core/JsonSettings.h"
#include "../../../core/ThemesManager.h"
#include "../../../core/UserScript.h"
#include "../../../core/Utils.h"

#include "ui_AddonsContentsWidget.h"

#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

AddonsContentsWidget::AddonsContentsWidget(const QVariantMap &parameters, Window *window) : ContentsWidget(parameters, window),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::AddonsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->addonsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->addonsViewWidget->installEventFilter(this);
	m_ui->filterLineEdit->installEventFilter(this);

	QTimer::singleShot(100, this, SLOT(populateAddons()));

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterAddons(QString)));
	connect(m_ui->addonsViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(m_ui->addonsViewWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(save()));
}

AddonsContentsWidget::~AddonsContentsWidget()
{
	delete m_ui;
}

void AddonsContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void AddonsContentsWidget::populateAddons()
{
	m_types.clear();
	m_types[Addon::UserScriptType] = 0;

	QStandardItem *userScriptsItem(new QStandardItem(ThemesManager::getIcon(QLatin1String("addon-user-script"), false), tr("User Scripts")));
	userScriptsItem->setData(Addon::UserScriptType, TypeRole);

	m_model->appendRow(userScriptsItem);

	const QStringList userScripts(AddonsManager::getUserScripts());

	for (int i = 0; i < userScripts.count(); ++i)
	{
		addAddon(AddonsManager::getUserScript(userScripts.at(i)));
	}

	m_ui->addonsViewWidget->setModel(m_model);
	m_ui->addonsViewWidget->expandAll();

	m_isLoading = false;

	emit loadingStateChanged(WindowsManager::FinishedLoadingState);
}

void AddonsContentsWidget::addAddon()
{
	const QStringList sourcePaths(QFileDialog::getOpenFileNames(this, tr("Select Files"), QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0), Utils::formatFileTypes(QStringList(tr("User Script files (*.js)")))));

	if (sourcePaths.isEmpty())
	{
		return;
	}

	QStringList failedPaths;
	ReplaceMode replaceMode(UnknownMode);

	for (int i = 0; i < sourcePaths.count(); ++i)
	{
		if (sourcePaths.at(i).isEmpty())
		{
			continue;
		}

		const QString scriptName(QFileInfo(sourcePaths.at(i)).completeBaseName());
		const QString targetDirectory(QDir(SessionsManager::getWritableDataPath(QLatin1String("scripts"))).filePath(scriptName));
		const QString targetPath(QDir(targetDirectory).filePath(QFileInfo(sourcePaths.at(i)).fileName()));
		bool replaceScript(false);

		if (QFile::exists(targetPath))
		{
			if (replaceMode == IgnoreAllMode)
			{
				continue;
			}

			if (replaceMode == ReplaceAllMode)
			{
				replaceScript = true;
			}
			else
			{
				QMessageBox messageBox;
				messageBox.setWindowTitle(tr("Question"));
				messageBox.setText(tr("User Script with this name already exists:\n%1").arg(targetPath));
				messageBox.setInformativeText(tr("Do you want to replace it?"));
				messageBox.setIcon(QMessageBox::Question);
				messageBox.addButton(QMessageBox::Yes);
				messageBox.addButton(QMessageBox::No);

				if (i < (sourcePaths.count() - 1))
				{
					messageBox.setCheckBox(new QCheckBox(tr("Apply to all")));
				}

				messageBox.exec();

				replaceScript = (messageBox.standardButton(messageBox.clickedButton()) == QMessageBox::Yes);

				if (messageBox.checkBox() && messageBox.checkBox()->isChecked())
				{
					replaceMode = (replaceScript ? ReplaceAllMode : IgnoreAllMode);
				}
			}

			if (!replaceScript)
			{
				continue;
			}
		}

		if ((replaceScript && !QDir().remove(targetPath)) || (!replaceScript && !QDir().mkpath(targetDirectory)) || !QFile::copy(sourcePaths.at(i), targetPath))
		{
			failedPaths.append(sourcePaths.at(i));

			continue;
		}

		if (replaceScript)
		{
			UserScript *script(AddonsManager::getUserScript(scriptName));

			if (script)
			{
				script->reload();
			}
		}
		else
		{
			UserScript script(targetPath);

			addAddon(&script);
		}
	}

	if (!failedPaths.isEmpty())
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to import following User Script file(s):\n%1", "", failedPaths.count()).arg(failedPaths.join(QLatin1Char('\n'))), QMessageBox::Close);
	}

	save();

	AddonsManager::loadUserScripts();
}

void AddonsContentsWidget::addAddon(Addon *addon)
{
	if (!addon)
	{
		return;
	}

	QStandardItem *typeItem(nullptr);

	if (m_types.contains(addon->getType()))
	{
		typeItem = m_model->item(m_types[addon->getType()]);
	}

	if (!typeItem)
	{
		return;
	}

	QStandardItem *item(new QStandardItem(addon->getIcon(), (addon->getVersion().isEmpty() ? addon->getTitle() : QStringLiteral("%1 %2").arg(addon->getTitle()).arg(addon->getVersion()))));
	item->setFlags(item->flags() | Qt::ItemNeverHasChildren);
	item->setCheckable(true);
	item->setCheckState(addon->isEnabled() ? Qt::Checked : Qt::Unchecked);
	item->setToolTip(addon->getDescription());

	if (addon->getType() == Addon::UserScriptType)
	{
		UserScript *script(qobject_cast<UserScript*>(addon));

		if (script)
		{
			item->setData(script->getName(), NameRole);
		}
	}

	typeItem->appendRow(item);
}

void AddonsContentsWidget::reloadAddon()
{
	const QModelIndexList indexes(m_ui->addonsViewWidget->selectionModel()->selectedIndexes());

	for (int i = 0; i < indexes.count(); ++i)
	{
		if (indexes.at(i).isValid() && indexes.at(i).parent() != m_model->invisibleRootItem()->index())
		{
			Addon::AddonType type(static_cast<Addon::AddonType>(indexes.at(i).parent().data(TypeRole).toInt()));

			if (type == Addon::UserScriptType)
			{
				UserScript *script(AddonsManager::getUserScript(indexes.at(i).data(NameRole).toString()));

				if (script)
				{
					script->reload();
				}
			}
		}
	}
}

void AddonsContentsWidget::removeAddons()
{
	const QModelIndexList indexes(m_ui->addonsViewWidget->selectionModel()->selectedIndexes());

	if (indexes.isEmpty())
	{
		return;
	}

//TODO
}

void AddonsContentsWidget::save()
{
	QStandardItem *userScriptsItem(nullptr);

	if (m_types.contains(Addon::UserScriptType))
	{
		userScriptsItem = m_model->item(m_types[Addon::UserScriptType]);
	}

	if (!userScriptsItem)
	{
		return;
	}

	QJsonObject settingsObject;

	for (int i = 0; i < userScriptsItem->rowCount(); ++i)
	{
		QStandardItem *item(userScriptsItem->child(i));

		if (item && !item->data(NameRole).toString().isEmpty())
		{
			QJsonObject scriptObject;
			scriptObject.insert(QLatin1String("isEnabled"), QJsonValue(item->checkState() == Qt::Checked));

			settingsObject.insert(item->data(NameRole).toString(), scriptObject);
		}
	}

	JsonSettings settings;
	settings.setObject(settingsObject);
	settings.save(SessionsManager::getWritableDataPath(QLatin1String("scripts/scripts.json")));
}

void AddonsContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index(m_ui->addonsViewWidget->indexAt(point));
	QMenu menu(this);

	if (index.isValid() && index.parent() != m_model->invisibleRootItem()->index())
	{
		menu.addAction(tr("Reload Addon"), this, SLOT(reloadAddon()));
		menu.addSeparator();
		menu.addAction(tr("Remove Addon…"), this, SLOT(removeAddons()))->setEnabled(false);
		menu.addSeparator();
	}

	menu.addAction(tr("Add Addon…"), this, SLOT(addAddon()));
	menu.exec(m_ui->addonsViewWidget->mapToGlobal(point));
}

void AddonsContentsWidget::print(QPrinter *printer)
{
	m_ui->addonsViewWidget->render(printer);
}

void AddonsContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->addonsViewWidget->selectAll();

			break;
		case ActionsManager::DeleteAction:
			removeAddons();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->addonsViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void AddonsContentsWidget::filterAddons(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->addonsViewWidget->setRowHidden(i, m_model->invisibleRootItem()->index(), (!filter.isEmpty() && !m_model->item(i, 0)->data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive)));
	}
}

Action* AddonsContentsWidget::getAction(int identifier)
{
	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	if (identifier != ActionsManager::DeleteAction && identifier != ActionsManager::SelectAllAction)
	{
		return nullptr;
	}

	Action *action(new Action(identifier, this));

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	return action;
}

QString AddonsContentsWidget::getTitle() const
{
	return tr("Addons");
}

QLatin1String AddonsContentsWidget::getType() const
{
	return QLatin1String("addons");
}

QUrl AddonsContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:addons"));
}

QIcon AddonsContentsWidget::getIcon() const
{
	return ThemesManager::getIcon(QLatin1String("preferences-plugin"), false);
}

WindowsManager::LoadingState AddonsContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WindowsManager::OngoingLoadingState : WindowsManager::FinishedLoadingState);
}

bool AddonsContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->addonsViewWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent && keyEvent->key() == Qt::Key_Delete)
		{
			removeAddons();

			return true;
		}
	}
	else if (object == m_ui->filterLineEdit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent->key() == Qt::Key_Escape)
		{
			m_ui->filterLineEdit->clear();
		}
	}

	return ContentsWidget::eventFilter(object, event);
}

}
