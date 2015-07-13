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

#include "UpdateCheckerDialog.h"
#include "../core/UpdateChecker.h"
#include "../core/WindowsManager.h"

#include "ui_UpdateCheckerDialog.h"

#include <QtWidgets/QPushButton>

namespace Otter
{

UpdateCheckerDialog::UpdateCheckerDialog(QWidget *parent) : QDialog(parent),
	m_updateChecker(new UpdateChecker(this, false)),
	m_ui(new Ui::UpdateCheckerDialog)
{
	m_ui->setupUi(this);

	connect(m_updateChecker, SIGNAL(finished(QList<UpdateInformation>)), this, SLOT(updateCheckFinished(QList<UpdateInformation>)));
	connect(this, SIGNAL(finished(int)), this, SLOT(deleteLater()));
}

UpdateCheckerDialog::~UpdateCheckerDialog()
{
	delete m_ui;
}

void UpdateCheckerDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		adjustSize();
	}
}

void UpdateCheckerDialog::updateCheckFinished(const QList<UpdateInformation> &availableUpdates)
{
	m_ui->progressBar->hide();

	if (availableUpdates.isEmpty())
	{
		m_ui->label->setText(tr("There are no new updates."));
	}
	else
	{
		m_ui->label->setText(tr("Available updates:"));

		for (int i = 0; i < availableUpdates.count(); ++i)
		{
			QPushButton *button = new QPushButton(tr("Update…"), this);
			button->setProperty("detailsUrl", availableUpdates.at(i).detailsUrl);

			m_ui->gridLayout->addWidget(new QLabel(tr("Version %1 from %2 channel").arg(availableUpdates.at(i).version).arg(availableUpdates.at(i).channel), this), i, 0);
			m_ui->gridLayout->addWidget(button, i, 1);

			connect(button, SIGNAL(clicked(bool)), this, SLOT(runUpdate()));
		}
	}
}

void UpdateCheckerDialog::runUpdate()
{
	QPushButton *button = qobject_cast<QPushButton*>(sender());

	if (button && !SessionsManager::hasUrl(button->property("detailsUrl").toUrl(), true))
	{
		WindowsManager *manager = SessionsManager::getWindowsManager();

		if (manager)
		{
			manager->open(button->property("detailsUrl").toUrl());
		}
	}

	close();
}

}
