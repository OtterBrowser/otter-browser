/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "KeyboardProfileDialog.h"
#include "../ActionComboBoxWidget.h"

#include "ui_KeyboardProfileDialog.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtWidgets/QKeySequenceEdit>
#include <QtWidgets/QToolButton>

namespace Otter
{

KeyboardActionDelegate::KeyboardActionDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void KeyboardActionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	ActionComboBoxWidget *widget(qobject_cast<ActionComboBoxWidget*>(editor));

	if (widget && widget->getActionIdentifier() >= 0)
	{
		const ActionsManager::ActionDefinition definition(ActionsManager::getActionDefinition(widget->getActionIdentifier()));

		model->setData(index, definition.getText(true), Qt::DisplayRole);
		model->setData(index, QStringLiteral("%1 (%2)").arg(definition.getText(true)).arg(ActionsManager::getActionName(widget->getActionIdentifier())), Qt::ToolTipRole);
		model->setData(index, widget->getActionIdentifier(), KeyboardProfileDialog::IdentifierRole);

		if (definition.defaultState.icon.isNull())
		{
			model->setData(index, QColor(Qt::transparent), Qt::DecorationRole);
		}
		else
		{
			model->setData(index, definition.defaultState.icon, Qt::DecorationRole);
		}
	}
}

QWidget* KeyboardActionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	ActionComboBoxWidget *widget(new ActionComboBoxWidget(parent));
	widget->setActionIdentifier(index.data(KeyboardProfileDialog::IdentifierRole).toInt());
	widget->setFocus();

	return widget;
}

KeyboardShortcutDelegate::KeyboardShortcutDelegate(QObject *parent) : ItemDelegate(parent)
{
}

void KeyboardShortcutDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	option->text = QKeySequence(index.data(Qt::DisplayRole).toString()).toString(QKeySequence::NativeText);
}

void KeyboardShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QKeySequenceEdit *widget(qobject_cast<QKeySequenceEdit*>(editor));

	if (widget)
	{
		const QKeySequence shortcut(widget->keySequence());

		if (!shortcut.isEmpty())
		{
			const QModelIndexList indexes(index.model()->match(index.model()->index(0, 2), Qt::DisplayRole, shortcut.toString(), 1, Qt::MatchExactly));

			if (!indexes.isEmpty() && indexes.first() != index)
			{
				model->setData(index, QString());

				return;
			}
		}

		model->setData(index, shortcut.toString());
	}
}

QWidget* KeyboardShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	const QKeySequence shortcut(index.data(Qt::DisplayRole).toString());
	QPointer<QKeySequenceEdit> widget(new QKeySequenceEdit(shortcut, parent));
	widget->setFocus();

	QVBoxLayout *layout(widget->findChild<QVBoxLayout*>());

	if (layout)
	{
		QToolButton *button(new QToolButton(widget));
		button->setText(tr("Clear"));
		button->setEnabled(!shortcut.isEmpty());

		layout->setDirection(QBoxLayout::LeftToRight);
		layout->addWidget(button);

		connect(button, &QToolButton::clicked, widget, &QKeySequenceEdit::clear);
		connect(widget, &QKeySequenceEdit::keySequenceChanged, [=](const QKeySequence &shortcut)
		{
			button->setEnabled(!shortcut.isEmpty());

			if (!shortcut.isEmpty())
			{
				const QModelIndexList indexes(index.model()->match(index.model()->index(0, 2), Qt::DisplayRole, shortcut.toString(), 1, Qt::MatchExactly));

				if (!indexes.isEmpty() && indexes.first() != index)
				{
					widget->clear();
					widget->setStyleSheet(QLatin1String("QLineEdit {background:#F1E7E4;}"));

					QTimer::singleShot(1000, [=]()
					{
						if (widget)
						{
							widget->setStyleSheet(QString());
						}
					});
				}
			}
		});
	}

	return widget;
}

KeyboardProfileDialog::KeyboardProfileDialog(const QString &profile, const QHash<QString, KeyboardProfile> &profiles, QWidget *parent) : Dialog(parent),
	m_profile(profiles[profile]),
	m_ui(new Ui::KeyboardProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model(new QStandardItemModel(this));
	const QVector<KeyboardProfile::Action> definitions(m_profile.getDefinitions().value(ActionsManager::GenericContext));

	for (int i = 0; i < definitions.count(); ++i)
	{
		const ActionsManager::ActionDefinition action(ActionsManager::getActionDefinition(definitions.at(i).action));
		const QString parameters(definitions.at(i).parameters.isEmpty() ? QString() : QJsonDocument(QJsonObject::fromVariantMap(definitions.at(i).parameters)).toJson(QJsonDocument::Compact));

		for (int j = 0; j < definitions.at(i).shortcuts.count(); ++j)
		{
			QList<QStandardItem*> items({new QStandardItem(action.getText(true)), new QStandardItem(parameters), new QStandardItem(definitions.at(i).shortcuts.at(j).toString())});
			items[0]->setData(QColor(Qt::transparent), Qt::DecorationRole);
			items[0]->setData(definitions.at(i).action, IdentifierRole);
			items[0]->setData(definitions.at(i).parameters, ParametersRole);
			items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
			items[0]->setToolTip(QStringLiteral("%1 (%2)").arg(action.getText(true)).arg(ActionsManager::getActionName(definitions.at(i).action)));
			items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
			items[1]->setToolTip(parameters);
			items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

			if (!action.defaultState.icon.isNull())
			{
				items[0]->setIcon(action.defaultState.icon);
			}

			model->appendRow(items);
		}
	}

	model->setHorizontalHeaderLabels({tr("Action"), tr("Parameters"), tr("Shortcut")});
	model->sort(0);

	m_ui->actionsViewWidget->setModel(model);
	m_ui->actionsViewWidget->setItemDelegateForColumn(0, new KeyboardActionDelegate(this));
	m_ui->actionsViewWidget->setItemDelegateForColumn(2, new KeyboardShortcutDelegate(this));
	m_ui->actionsViewWidget->setModified(m_profile.isModified());
	m_ui->titleLineEdit->setText(m_profile.getTitle());
	m_ui->descriptionLineEdit->setText(m_profile.getDescription());
	m_ui->versionLineEdit->setText(m_profile.getVersion());
	m_ui->authorLineEdit->setText(m_profile.getAuthor());

	connect(m_ui->filterLineEdit, &QLineEdit::textChanged, m_ui->actionsViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->actionsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &KeyboardProfileDialog::updateActions);
	connect(m_ui->addActionButton, &QPushButton::clicked, this, &KeyboardProfileDialog::addAction);
	connect(m_ui->removeActionButton, &QPushButton::clicked, this, &KeyboardProfileDialog::removeAction);
}

KeyboardProfileDialog::~KeyboardProfileDialog()
{
	delete m_ui;
}

void KeyboardProfileDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void KeyboardProfileDialog::addAction()
{
	QList<QStandardItem*> items({new QStandardItem(), new QStandardItem(), new QStandardItem()});
	items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
	items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
	items[2]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

	m_ui->actionsViewWidget->insertRow(items);
}

void KeyboardProfileDialog::removeAction()
{
	m_ui->actionsViewWidget->removeRow();
}

void KeyboardProfileDialog::updateActions()
{
	m_ui->removeActionButton->setEnabled(m_ui->actionsViewWidget->getCurrentIndex().isValid());
}

KeyboardProfile KeyboardProfileDialog::getProfile() const
{
	KeyboardProfile profile;
	profile.setTitle(m_ui->titleLineEdit->text());
	profile.setDescription(m_ui->descriptionLineEdit->text());
	profile.setVersion(m_ui->versionLineEdit->text());
	profile.setAuthor(m_ui->authorLineEdit->text());

	QMap<int, QVector<QPair<QVariantMap, QVector<QKeySequence> > > > actions;

	for (int i = 0; i < m_ui->actionsViewWidget->getRowCount(); ++i)
	{
		const QKeySequence shortcut(m_ui->actionsViewWidget->getIndex(i, 2).data(Qt::DisplayRole).toString());
		const int action(m_ui->actionsViewWidget->getIndex(i, 0).data(IdentifierRole).toInt());

		if (action >= 0 && !shortcut.isEmpty())
		{
			const QVariantMap parameters(m_ui->actionsViewWidget->getIndex(i, 0).data(ParametersRole).toMap());
			bool hasFound(false);

			if (actions.contains(action))
			{
				QVector<QPair<QVariantMap, QVector<QKeySequence> > > actionVariants(actions[action]);

				for (int j = 0; j < actionVariants.count(); ++j)
				{
					if (actionVariants.at(j).first == parameters)
					{
						actionVariants[j].second.append(shortcut);

						actions[action] = actionVariants;

						hasFound = true;

						break;
					}
				}
			}

			if (!hasFound)
			{
				actions[action] = {{parameters, {shortcut}}};
			}
		}
	}

	QMap<int, QVector<QPair<QVariantMap, QVector<QKeySequence> > > >::iterator iterator;
	QVector<KeyboardProfile::Action> definitions;
	definitions.reserve(actions.count());

	for (iterator = actions.begin(); iterator != actions.end(); ++iterator)
	{
		const QVector<QPair<QVariantMap, QVector<QKeySequence> > > actionVariants(iterator.value());

		for (int j = 0; j < actionVariants.count(); ++j)
		{
			KeyboardProfile::Action definition;
			definition.parameters = actionVariants[j].first;
			definition.shortcuts = actionVariants[j].second;
			definition.action = iterator.key();

			definitions.append(definition);
		}
	}

	profile.setDefinitions({{ActionsManager::GenericContext, definitions}});
	profile.setModified(m_ui->actionsViewWidget->isModified());

	return profile;
}

bool KeyboardProfileDialog::isModified() const
{
	return m_ui->actionsViewWidget->isModified();
}

}
