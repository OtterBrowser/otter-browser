/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TransferDialog.h"
#include "../core/HandlersManager.h"
#include "../core/TransfersManager.h"
#include "../core/Utils.h"

#include "ui_TransferDialog.h"

#include <QtCore/QFileInfo>

namespace Otter
{

TransferDialog::TransferDialog(Transfer *transfer, QWidget *parent) : Dialog(parent),
	m_transfer(transfer),
	m_ui(new Ui::TransferDialog)
{
	const QPixmap pixmap(transfer->getIcon().pixmap(16, 16));
	QString fileName(transfer->getSuggestedFileName());

	if (fileName.isEmpty())
	{
		fileName = tr("unknown file");
	}

	m_ui->setupUi(this);

	if (pixmap.isNull())
	{
		m_ui->iconLabel->hide();
	}
	else
	{
		m_ui->iconLabel->setPixmap(pixmap);
	}

	m_ui->nameTextLabelWidget->setText(fileName);
	m_ui->typeTextLabelWidget->setText(transfer->getMimeType().comment());
	m_ui->fromTextLabelWidget->setText(Utils::extractHost(transfer->getSource()));
	m_ui->openWithComboBoxWidget->setMimeType(transfer->getMimeType());

	setProgress(m_transfer->getBytesReceived(), m_transfer->getBytesTotal());
	setWindowTitle(tr("Opening %1").arg(fileName));

	connect(transfer, &Transfer::progressChanged, this, &TransferDialog::setProgress);
	connect(m_ui->buttonBox, &QDialogButtonBox::clicked, this, &TransferDialog::handleButtonClicked);
}

TransferDialog::~TransferDialog()
{
	delete m_ui;
}

void TransferDialog::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void TransferDialog::handleButtonClicked(QAbstractButton *button)
{
	const QDialogButtonBox::StandardButton standardButton(m_ui->buttonBox->standardButton(button));

	if (!m_transfer || (standardButton != QDialogButtonBox::Open && standardButton != QDialogButtonBox::Save))
	{
		if (m_transfer)
		{
			m_transfer->cancel();

			if (m_ui->rememberChoiceCheckBox->isChecked())
			{
				HandlersManager::MimeTypeHandlerDefinition definition;
				definition.transferMode = HandlersManager::MimeTypeHandlerDefinition::IgnoreTransfer;

				HandlersManager::setMimeTypeHandler(m_transfer->getMimeType(), definition);
			}
		}

		reject();

		return;
	}

	switch (standardButton)
	{
		case QDialogButtonBox::Open:
			m_transfer->setOpenCommand(m_ui->openWithComboBoxWidget->getCommand());

			if (m_ui->rememberChoiceCheckBox->isChecked())
			{
				HandlersManager::MimeTypeHandlerDefinition definition;
				definition.transferMode = HandlersManager::MimeTypeHandlerDefinition::OpenTransfer;
				definition.openCommand = m_ui->openWithComboBoxWidget->getCommand();

				HandlersManager::setMimeTypeHandler(m_transfer->getMimeType(), definition);
			}

			break;
		case QDialogButtonBox::Save:
			{
				QWidget *dialog(parentWidget());

				while (dialog)
				{
					if (dialog->inherits("Otter::ContentsDialog"))
					{
						dialog->hide();

						break;
					}

					dialog = dialog->parentWidget();
				}

				const QString path(Utils::getSavePath(m_transfer->getSuggestedFileName()).path);

				if (path.isEmpty())
				{
					m_transfer->cancel();

					reject();

					return;
				}

				m_transfer->setTarget(path, true);

				if (m_ui->rememberChoiceCheckBox->isChecked())
				{
					HandlersManager::MimeTypeHandlerDefinition definition;
					definition.transferMode = HandlersManager::MimeTypeHandlerDefinition::SaveTransfer;
					definition.downloadsPath = QFileInfo(path).canonicalPath();

					HandlersManager::setMimeTypeHandler(m_transfer->getMimeType(), definition);
				}
			}

			break;
		default:
			break;
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
		m_ui->sizeTextLabelWidget->setText(tr("%1 (%2% downloaded)").arg(Utils::formatUnit(bytesTotal)).arg(Utils::calculatePercent(bytesReceived, bytesTotal), 0, 'f', 1));
	}
}

}
