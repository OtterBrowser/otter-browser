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

#include "TransferDialog.h"
#include "../core/Transfer.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"

#include "ui_TransferDialog.h"

#include <QtCore/QtMath>

namespace Otter
{

TransferDialog::TransferDialog(Transfer *transfer, QWidget *parent) : QDialog(parent),
	m_transfer(transfer),
	m_ui(new Ui::TransferDialog)
{
	QString fileName = transfer->getSuggestedFileName();

	if (fileName.isEmpty())
	{
		fileName = tr("unknown file");
	}

	m_ui->setupUi(this);
	m_ui->iconLabel->setFixedSize(16, 16);
	m_ui->iconLabel->setPixmap(Utils::getIcon(transfer->getMimeType().iconName()).pixmap(16, 16));
	m_ui->nameTextLabelWidget->setText(fileName);
	m_ui->typeTextLabelWidget->setText(transfer->getMimeType().comment());
	m_ui->fromTextLabelWidget->setText(transfer->getSource().host().isEmpty() ? QLatin1String("localhost") : transfer->getSource().host());

	const QList<ApplicationInformation> applications = Utils::getApplicationsForMimeType(transfer->getMimeType());

	if (applications.isEmpty())
	{
		m_ui->openWithComboBox->addItem(tr("Default Application"));
	}
	else
	{
		for (int i = 0; i < applications.count(); ++i)
		{
			m_ui->openWithComboBox->addItem(applications.at(i).icon, ((applications.at(i).name.isEmpty()) ? tr("Unknown") : applications.at(i).name), applications.at(i).command);

			if (i == 0 && applications.count() > 1)
			{
				m_ui->openWithComboBox->insertSeparator(1);
			}
		}
	}

	setProgress(m_transfer->getBytesReceived(), m_transfer->getBytesTotal());
	setWindowTitle(tr("Opening %1").arg(fileName));

	adjustSize();

	connect(transfer, SIGNAL(progressChanged(qint64,qint64)), this, SLOT(setProgress(qint64,qint64)));
	connect(m_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

TransferDialog::~TransferDialog()
{
	delete m_ui;
}

void TransferDialog::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

void TransferDialog::buttonClicked(QAbstractButton *button)
{
	const QDialogButtonBox::StandardButton standardButton = m_ui->buttonBox->standardButton(button);

	if (!m_transfer || (standardButton != QDialogButtonBox::Open && standardButton != QDialogButtonBox::Save))
	{
		if (m_transfer)
		{
			m_transfer->cancel();
		}

		reject();

		return;
	}

	if (standardButton == QDialogButtonBox::Open)
	{
		m_transfer->setOpenCommand(m_ui->openWithComboBox->currentData().toString());
	}
	else if (standardButton == QDialogButtonBox::Save)
	{
		const QString path = TransfersManager::getSavePath(m_transfer->getSuggestedFileName());

		if (path.isEmpty())
		{
			m_transfer->cancel();

			reject();

			return;
		}

		m_transfer->setTarget(path);
	}

	TransfersManager::addTransfer(m_transfer);

	accept();
}

void TransferDialog::setProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (bytesTotal < 1)
	{
		m_ui->sizeTextLabelWidget->setText(tr("unknown"));
	}
	else if (bytesReceived > 0 && bytesReceived == bytesTotal)
	{
		m_ui->sizeTextLabelWidget->setText(tr("%1 (download completed)").arg(Utils::formatUnit(bytesTotal)));
	}
	else
	{
		const int progress = ((bytesReceived > 0 && bytesTotal > 0) ? qFloor((static_cast<qreal>(bytesReceived) / bytesTotal) * 100) : 0);

		m_ui->sizeTextLabelWidget->setText(tr("%1 (%n% downloaded)", "", progress).arg(Utils::formatUnit(bytesTotal)));
	}
}

}
