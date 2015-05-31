/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ImagePropertiesDialog.h"

#include "ui_ImagePropertiesDialog.h"

#include <QtCore/QBuffer>
#include <QtCore/QMimeDatabase>
#include <QtGui/QImageReader>

namespace Otter
{

ImagePropertiesDialog::ImagePropertiesDialog(const QUrl &url, const QVariantMap &properties, QIODevice *device, QWidget *parent) : QDialog(parent),
	m_ui(new Ui::ImagePropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->addressLabelWidget->setText(url.toString());
	m_ui->sizeLabelWidget->setText(tr("Unknown"));
	m_ui->typeLabelWidget->setText(tr("Unknown"));
	m_ui->fileSizeLabelWidget->setText(tr("Unknown"));
	m_ui->alternativeTextLabelWidget->setText(properties.value(QLatin1String("alternativeText")).toString());
	m_ui->longDescriptionLabelWidget->setText(properties.value(QLatin1String("longDescription")).toString());

	QByteArray array;
	QImage image;
	int frames = 1;

	if (url.scheme() == QLatin1String("data") && !device)
	{
		const QString data = url.path();

		array = QByteArray::fromBase64(data.mid(data.indexOf(QLatin1String("base64,")) + 7).toUtf8());

		device = new QBuffer(&array, this);
	}

	if (device)
	{
		QString size;

		if (device->size() > 1024)
		{
			if (device->size() > 1048576)
			{
				if (device->size() > 1073741824)
				{
					size = tr("%1 GB (%2 bytes)").arg((device->size() / 1073741824.0), 0, 'f', 2).arg(device->size());
				}
				else
				{
					size = tr("%1 MB (%2 bytes)").arg((device->size() / 1048576.0), 0, 'f', 2).arg(device->size());
				}
			}
			else
			{
				size = tr("%1 KB (%2 bytes)").arg((device->size() / 1024.0), 0, 'f', 2).arg(device->size());
			}
		}
		else
		{
			size = tr("%1 B").arg(device->size());
		}

		m_ui->fileSizeLabelWidget->setText(size);

		QImageReader reader(device);

		frames = reader.imageCount();

		image = reader.read();
	}

	if (!image.isNull())
	{
		if (frames > 1)
		{
			m_ui->sizeLabelWidget->setText(tr("%1 x %2 pixels @ %3 bits per pixel in %n frames", "", frames).arg(image.width()).arg(image.height()).arg(image.depth()));
		}
		else
		{
			m_ui->sizeLabelWidget->setText(tr("%1 x %2 pixels @ %3 bits per pixel").arg(image.width()).arg(image.height()).arg(image.depth()));
		}
	}
	else if (properties.contains(QLatin1String("width")))
	{
		if (properties.contains(QLatin1String("depth")))
		{
			m_ui->sizeLabelWidget->setText(tr("%1 x %2 pixels @ %3 bits per pixel").arg(properties.value(QLatin1String("width")).toInt()).arg(properties.value(QLatin1String("height")).toInt()).arg(properties.value(QLatin1String("depth")).toInt()));
		}
		else
		{
			m_ui->sizeLabelWidget->setText(tr("%1 x %2 pixels").arg(properties.value(QLatin1String("width")).toInt()).arg(properties.value(QLatin1String("height")).toInt()));
		}
	}

	if (device)
	{
		device->reset();

		m_ui->typeLabelWidget->setText(QMimeDatabase().mimeTypeForData(device).comment());

		device->deleteLater();
	}

	setMinimumWidth(400);
	adjustSize();
}

ImagePropertiesDialog::~ImagePropertiesDialog()
{
	delete m_ui;
}

void ImagePropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		adjustSize();
	}
}

void ImagePropertiesDialog::setButtonsVisible(bool visible)
{
	m_ui->buttonBox->setVisible(visible);
}

}
