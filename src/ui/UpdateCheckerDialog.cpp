/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2022 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#include "UpdateCheckerDialog.h"
#include "../core/Application.h"
#include "../core/Updater.h"

#include "ui_UpdateCheckerDialog.h"

#include <QtWidgets/QPushButton>

namespace Otter
{

UpdateCheckerDialog::UpdateCheckerDialog(QWidget *parent, const QVector<UpdateChecker::UpdateInformation> &availableUpdates) : Dialog(parent),
	m_ui(new Ui::UpdateCheckerDialog)
{
	m_ui->setupUi(this);
	m_ui->progressBar->setAlignment(Qt::AlignCenter);

	connect(this, &UpdateCheckerDialog::finished, this, &UpdateCheckerDialog::deleteLater);

	if (Updater::isReadyToInstall())
	{
		m_ui->progressBar->setRange(0, 100);
		m_ui->progressBar->setValue(100);
		m_ui->progressBar->setFormat(QString::number(100) + QLatin1Char('%'));

		handleReadyToInstall();
	}
	else if (availableUpdates.isEmpty())
	{
		m_ui->label->setText(tr("Checking for updates…"));

		connect(new UpdateChecker(this, false), &UpdateChecker::finished, this, &UpdateCheckerDialog::handleUpdateCheckFinished);
	}
	else
	{
		handleUpdateCheckFinished(availableUpdates);
	}
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
	}
}

void UpdateCheckerDialog::handleUpdateCheckFinished(const QVector<UpdateChecker::UpdateInformation> &availableUpdates)
{
	m_ui->progressBar->hide();

	if (availableUpdates.isEmpty())
	{
		m_ui->label->setText(tr("There are no new updates."));

		return;
	}

	bool hasUnavailableUpdates(false);

	m_ui->label->setText(tr("Available updates:"));

	for (int i = 0; i < availableUpdates.count(); ++i)
	{
		const UpdateChecker::UpdateInformation updateInformation(availableUpdates.at(i));
		QPushButton *detailsButton(new QPushButton(tr("Details…"), this));
		QPushButton *updateButton(new QPushButton(tr("Download"), this));

		if (!updateInformation.isAvailable)
		{
			hasUnavailableUpdates = true;

			updateButton->setDisabled(true);
		}

		m_ui->gridLayout->addWidget(new QLabel(tr("Version %1 from %2 channel").arg(availableUpdates.at(i).version, updateInformation.channel), this), i, 0);
		m_ui->gridLayout->addWidget(detailsButton, i, 1);
		m_ui->gridLayout->addWidget(updateButton, i, 2);

		connect(detailsButton, &QPushButton::clicked, this, [=]()
		{
			if (!SessionsManager::hasUrl(updateInformation.detailsUrl, true))
			{
				Application::triggerAction(ActionsManager::OpenUrlAction, {{QLatin1String("url"), updateInformation.detailsUrl}}, this);

				close();
			}
		});
		connect(updateButton, &QPushButton::clicked, this, [=]()
		{
			for (int j = 0; j < m_ui->gridLayout->count(); ++j)
			{
				m_ui->gridLayout->itemAt(j)->widget()->setVisible(false);
				m_ui->gridLayout->itemAt(j)->widget()->deleteLater();
			}

			m_ui->label->setText(tr("Downloading:"));
			m_ui->progressBar->setRange(0, 100);
			m_ui->progressBar->show();
			m_ui->buttonBox->setDisabled(true);

			Updater *updater(new Updater(updateInformation, this));

			connect(updater, &Updater::progress, this, [&](int progress)
			{
				m_ui->progressBar->setValue(progress);
				m_ui->progressBar->setFormat(QString::number(progress) + QLatin1Char('%'));
			});
			connect(updater, &Updater::finished, this, [&](bool isSuccess)
			{
				if (isSuccess)
				{
					handleReadyToInstall();
				}
				else
				{
					m_ui->label->setText(tr("Download failed!"));

					QLabel *informationLabel(new QLabel(tr("Check Error Console for more information."), this));
					informationLabel->setWordWrap(true);

					m_ui->gridLayout->addWidget(informationLabel);
				}

				m_ui->buttonBox->setEnabled(true);
			}
);
		});
	}

	if (hasUnavailableUpdates)
	{
		QLabel *packageWarning(new QLabel(tr("Some of the updates do not contain packages for your platform. Try to check for updates later or visit details page for more info."), this));
		packageWarning->setWordWrap(true);

		m_ui->gridLayout->addWidget(packageWarning, m_ui->gridLayout->rowCount(), 0, m_ui->gridLayout->columnCount(), 0);
	}
}

void UpdateCheckerDialog::handleReadyToInstall()
{
	m_ui->label->setText(tr("Download finished!"));
	m_ui->buttonBox->addButton(tr("Install"), QDialogButtonBox::AcceptRole);

	QLabel *informationLabel(new QLabel(tr("New version of Otter Browser is ready to install.\nClick Install button to restart browser and install the update or close this dialog to install the update during next browser restart."), this));
	informationLabel->setWordWrap(true);

	m_ui->gridLayout->addWidget(informationLabel);

	connect(m_ui->buttonBox, &QDialogButtonBox::clicked, this, [&](QAbstractButton *button)
	{
		if (m_ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
		{
			Updater::installUpdate();
		}

		close();
	});
}

}
