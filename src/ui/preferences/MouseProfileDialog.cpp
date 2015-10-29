/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "MouseProfileDialog.h"
#include "../../core/ActionsManager.h"

#include "ui_MouseProfileDialog.h"

namespace Otter
{

MouseProfileDialog::MouseProfileDialog(const QString &profile, const QHash<QString, MouseProfile> &profiles, QWidget *parent) : Dialog(parent),
	m_profile(profile),
	m_isModified(profiles[profile].isModified),
	m_ui(new Ui::MouseProfileDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model = new QStandardItemModel(this);
	QStringList labels;
	labels << tr("Context and Steps") << tr("Action");

	QList<QPair<QString, QString> > contexts;
	contexts << qMakePair(QLatin1String("Generic"), tr("Generic")) << qMakePair(QLatin1String("Link"), tr("Link")) << qMakePair(QLatin1String("Tab"), tr("Tab Handle"));

	for (int i = 0; i < contexts.count(); ++i)
	{
		QStandardItem *item = new QStandardItem(contexts.at(i).second);
		item->setData(contexts.at(i).first, Qt::UserRole);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		if (profiles[profile].gestures.contains(contexts.at(i).first))
		{
			QHash<QString, int>::const_iterator iterator;

			for (iterator = profiles[profile].gestures[contexts.at(i).first].constBegin(); iterator != profiles[profile].gestures[contexts.at(i).first].constEnd(); ++iterator)
			{
				const ActionDefinition action = ActionsManager::getActionDefinition(iterator.value());
				QList<QStandardItem*> items;
				items.append(new QStandardItem(QString(iterator.key()).replace(QLatin1Char(','), QLatin1String(", "))));
				items[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
				items.append(new QStandardItem(action.icon, QCoreApplication::translate("actions", (action.description.isEmpty() ? action.text : action.description).toUtf8().constData())));
				items[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

				item->appendRow(items);
			}
		}

		model->appendRow(item);
	}

	model->setHorizontalHeaderLabels(labels);
	model->sort(0);

	m_ui->gesturesViewWidget->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->gesturesViewWidget->setModel(model);
	m_ui->stepsViewWidget->setModel(new QStandardItemModel(this));
	m_ui->titleLineEdit->setText(profiles[profile].title);
	m_ui->descriptionLineEdit->setText(profiles[profile].description);
	m_ui->versionLineEdit->setText(profiles[profile].version);
	m_ui->authorLineEdit->setText(profiles[profile].author);

	connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)), m_ui->gesturesViewWidget, SLOT(setFilterString(QString)));
	connect(m_ui->gesturesViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateGesturesActions()));
	connect(m_ui->addGestureButton, SIGNAL(clicked()), this, SLOT(addGesture()));
	connect(m_ui->removeGestureButton, SIGNAL(clicked()), this, SLOT(removeGesture()));
	connect(m_ui->stepsViewWidget, SIGNAL(needsActionsUpdate()), this, SLOT(updateStepsActions()));
	connect(m_ui->addStepButton, SIGNAL(clicked()), this, SLOT(addStep()));
	connect(m_ui->removeStepButton, SIGNAL(clicked()), this, SLOT(removeStep()));
}

MouseProfileDialog::~MouseProfileDialog()
{
	delete m_ui;
}

void MouseProfileDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void MouseProfileDialog::addGesture()
{
///TODO
}

void MouseProfileDialog::removeGesture()
{
///TODO
}

void MouseProfileDialog::updateGesturesActions()
{
///TODO
}

void MouseProfileDialog::addStep()
{
///TODO
}

void MouseProfileDialog::removeStep()
{
///TODO
}

void MouseProfileDialog::updateStepsActions()
{
///TODO
}

MouseProfile MouseProfileDialog::getProfile() const
{
	MouseProfile profile;
	profile.title = m_ui->titleLineEdit->text();
	profile.description = m_ui->descriptionLineEdit->text();
	profile.version = m_ui->versionLineEdit->text();
	profile.author = m_ui->authorLineEdit->text();
	profile.isModified = m_isModified;

///TODO

	return profile;
}

}
