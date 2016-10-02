/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PasswordsContentsWidget.h"
#include "../../../core/ActionsManager.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/PasswordsManager.h"
#include "../../../core/ThemesManager.h"

#include "ui_PasswordsContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

PasswordsContentsWidget::PasswordsContentsWidget(Window *window) : ContentsWidget(window),
	m_model(new QStandardItemModel(this)),
	m_isLoading(true),
	m_ui(new Ui::PasswordsContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->passwordsViewWidget->installEventFilter(this);
	m_ui->filterLineEdit->installEventFilter(this);

	m_ui->passwordsViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->passwordsViewWidget->setModel(m_model);

	QTimer::singleShot(100, this, SLOT(populatePasswords()));

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterPasswords(QString)));
	connect(m_ui->passwordsViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

PasswordsContentsWidget::~PasswordsContentsWidget()
{
	delete m_ui;
}

void PasswordsContentsWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PasswordsContentsWidget::populatePasswords()
{
	m_model->clear();
	m_model->setHorizontalHeaderLabels(QStringList({tr("Name"), tr("Value")}));

	const QStringList hosts(PasswordsManager::getHosts());

	for (int i = 0; i < hosts.count(); ++i)
	{
		const QUrl url(QStringLiteral("http://%1/").arg(hosts.at(i)));
		const QList<PasswordsManager::PasswordInformation> passwords(PasswordsManager::getPasswords(url));
		QStandardItem *hostItem(new QStandardItem(HistoryManager::getIcon(url), hosts.at(i)));
		hostItem->setData(hosts.at(i), Qt::UserRole);

		for (int j = 0; j < passwords.count(); ++j)
		{
			QStandardItem *setItem(new QStandardItem(tr("Set #%1").arg(j + 1)));
			setItem->setData(passwords.at(j).url, Qt::UserRole);
			setItem->setData(((passwords.at(j).type == PasswordsManager::AuthPassword) ? QLatin1String("auth") : QLatin1String("form")), (Qt::UserRole + 1));

			for (int k = 0; k < passwords.at(j).fields.count(); ++k)
			{
				const bool isPassword(passwords.at(j).fields.at(k).type == PasswordsManager::PasswordField);
				QList<QStandardItem*> fieldItems({new QStandardItem(passwords.at(j).fields.at(k).name), new QStandardItem(isPassword ? QLatin1String("*****") : passwords.at(j).fields.at(k).value)});
				fieldItems[0]->setData((isPassword ? QLatin1String("password") : QLatin1String("text")), Qt::UserRole);

				setItem->appendRow(fieldItems);
			}

			hostItem->appendRow(QList<QStandardItem*>({setItem, new QStandardItem()}));
		}

		hostItem->setText(QStringLiteral("%1 (%2)").arg(hosts.at(i)).arg(hostItem->rowCount()));

		m_model->appendRow(hostItem);
	}

	m_model->sort(0);

	if (m_isLoading)
	{
		m_isLoading = false;

		emit loadingStateChanged(WindowsManager::FinishedLoadingState);

		connect(PasswordsManager::getInstance(), SIGNAL(passwordsModified()), this, SLOT(populatePasswords()));
	}
}

void PasswordsContentsWidget::removePasswords()
{
	const QModelIndexList indexes(m_ui->passwordsViewWidget->selectionModel()->selectedIndexes());

	if (indexes.isEmpty())
	{
		return;
	}

	QList<PasswordsManager::PasswordInformation> passwords;

	for (int i = 0; i < indexes.count(); ++i)
	{
		if (!indexes.at(i).isValid() || indexes.at(i).column() > 0)
		{
			continue;
		}

		if (indexes.at(i).parent() == m_model->invisibleRootItem()->index())
		{
			const QModelIndex hostIndex(indexes.at(i));

			if (!hostIndex.isValid())
			{
				continue;
			}

			for (int j = 0; j < m_model->rowCount(hostIndex); ++j)
			{
				passwords.append(getPassword(hostIndex.child(j, 0)));
			}
		}
		else
		{
			const QModelIndex setIndex((indexes.at(i).parent().parent() == m_model->invisibleRootItem()->index()) ? indexes.at(i) : indexes.at(i).parent());

			if (setIndex.isValid())
			{
				passwords.append(getPassword(setIndex));
			}
		}
	}

	if (passwords.isEmpty())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete %n password(s).", "", passwords.count()));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		for (int i = 0; i < passwords.count(); ++i)
		{
			PasswordsManager::removePassword(passwords.at(i));
		}
	}
}

void PasswordsContentsWidget::removeHostPasswords()
{
	const QModelIndexList indexes(m_ui->passwordsViewWidget->selectionModel()->selectedIndexes());

	if (indexes.isEmpty())
	{
		return;
	}

	QStringList hosts;
	int amount(0);

	for (int i = 0; i < indexes.count(); ++i)
	{
		QModelIndex hostIndex(indexes.at(i));

		while (hostIndex.parent().isValid() && hostIndex.parent() != m_model->invisibleRootItem()->index())
		{
			hostIndex = hostIndex.parent();
		}

		if (hostIndex.isValid())
		{
			const QString host(hostIndex.data(Qt::UserRole).toString());

			if (!host.isEmpty() && !hosts.contains(host))
			{
				hosts.append(host);

				amount += m_model->rowCount(hostIndex);
			}
		}
	}

	if (hosts.isEmpty())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete %n password(s).", "", amount));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		for (int i = 0; i < hosts.count(); ++i)
		{
			PasswordsManager::clearPasswords(hosts.at(i));
		}
	}
}

void PasswordsContentsWidget::removeAllPasswords()
{
	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete all passwords."));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		PasswordsManager::clearPasswords();
	}
}

void PasswordsContentsWidget::showContextMenu(const QPoint &point)
{
	const QModelIndex index(m_ui->passwordsViewWidget->indexAt(point));
	QMenu menu(this);

	if (index.isValid())
	{
		if (index.parent() != m_model->invisibleRootItem()->index())
		{
			menu.addAction(tr("Remove Password"), this, SLOT(removePasswords()));
		}

		menu.addAction(tr("Remove All Passwords from This Domain…"), this, SLOT(removeHostPasswords()));
	}

	menu.addAction(tr("Remove All Passwords…"), this, SLOT(removeAllPasswords()))->setEnabled(m_ui->passwordsViewWidget->model()->rowCount() > 0);
	menu.addSeparator();
	menu.addAction(ActionsManager::getAction(ActionsManager::ClearHistoryAction, this));
	menu.exec(m_ui->passwordsViewWidget->mapToGlobal(point));
}

void PasswordsContentsWidget::print(QPrinter *printer)
{
	m_ui->passwordsViewWidget->render(printer);
}

void PasswordsContentsWidget::triggerAction(int identifier, const QVariantMap &parameters)
{
	Q_UNUSED(parameters)

	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->passwordsViewWidget->selectAll();

			break;
		case ActionsManager::DeleteAction:
			removePasswords();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
		case ActionsManager::ActivateAddressFieldAction:
			m_ui->filterLineEdit->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->passwordsViewWidget->setFocus();

			break;
		default:
			break;
	}
}

void PasswordsContentsWidget::filterPasswords(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		m_ui->passwordsViewWidget->setRowHidden(i, m_model->invisibleRootItem()->index(), (!filter.isEmpty() && !m_model->item(i, 0)->data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive)));
	}
}

Action* PasswordsContentsWidget::getAction(int identifier)
{
	if (m_actions.contains(identifier))
	{
		return m_actions[identifier];
	}

	if (identifier != ActionsManager::DeleteAction && identifier != ActionsManager::SelectAllAction)
	{
		return NULL;
	}

	Action *action(new Action(identifier, this));

	m_actions[identifier] = action;

	connect(action, SIGNAL(triggered()), this, SLOT(triggerAction()));

	return action;
}

QString PasswordsContentsWidget::getTitle() const
{
	return tr("Passwords Manager");
}

QLatin1String PasswordsContentsWidget::getType() const
{
	return QLatin1String("passwords");
}

QUrl PasswordsContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:passwords"));
}

QIcon PasswordsContentsWidget::getIcon() const
{
	return ThemesManager::getIcon(QLatin1String("dialog-password"), false);
}

PasswordsManager::PasswordInformation PasswordsContentsWidget::getPassword(const QModelIndex &index) const
{
	PasswordsManager::PasswordInformation password;
	password.url = index.data(Qt::UserRole).toString();
	password.type = ((index.data(Qt::UserRole + 1).toString() == QLatin1String("auth")) ? PasswordsManager::AuthPassword : PasswordsManager::FormPassword);

	for (int i = 0; i < m_model->rowCount(index); ++i)
	{
		const QModelIndex nameIndex(index.child(i, 0));
		const bool isPassword(nameIndex.data(Qt::UserRole).toString() == QLatin1String("password"));
		PasswordsManager::FieldInformation field;
		field.name = nameIndex.data(Qt::DisplayRole).toString();
		field.value = (isPassword ? QString() : index.child(i, 1).data(Qt::DisplayRole).toString());
		field.type = (isPassword ? PasswordsManager::PasswordField : PasswordsManager::TextField);

		password.fields.append(field);
	}

	return password;
}

WindowsManager::LoadingState PasswordsContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WindowsManager::OngoingLoadingState : WindowsManager::FinishedLoadingState);
}

bool PasswordsContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->passwordsViewWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent(static_cast<QKeyEvent*>(event));

		if (keyEvent && keyEvent->key() == Qt::Key_Delete)
		{
			removePasswords();

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
