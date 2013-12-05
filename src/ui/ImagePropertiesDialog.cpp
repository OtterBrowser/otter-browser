#include "ImagePropertiesDialog.h"

#include "ui_ImagePropertiesDialog.h"

#include <QtCore/QBuffer>
#include <QtCore/QMimeDatabase>
#include <QtGui/QImageReader>

namespace Otter
{

ImagePropertiesDialog::ImagePropertiesDialog(const QUrl &url, const QString &alternativeText, const QString &longDescription, const QPixmap &pixmap, QIODevice *device, QWidget *parent) : QDialog(parent),
	m_ui(new Ui::ImagePropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->addressValueLineEdit->setText(url.toString());
	m_ui->addressValueLineEdit->setCursorPosition(0);
	m_ui->alternativeTextValueLabel->setText(alternativeText.toHtmlEscaped());
	m_ui->longDescriptionValueLabel->setText(longDescription.toHtmlEscaped());

	QByteArray array;
	QImage image;
	int frames = 1;

	if (url.scheme() == "data" && !device)
	{
		const QString data = url.path();

		array = QByteArray::fromBase64(data.mid(data.indexOf("base64," + 7)).toUtf8());

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

		m_ui->fileSizeValueLabel->setText(size);

		QImageReader reader(device);

		frames = reader.imageCount();

		image = reader.read();
	}

	if (!image.isNull())
	{
		if (frames > 1)
		{
			m_ui->sizeValueLabel->setText(tr("%1 x %2 pixels @ %3 bits per pixel in %n frames", "", frames).arg(image.width()).arg(image.height()).arg(image.depth()));
		}
		else
		{
			m_ui->sizeValueLabel->setText(tr("%1 x %2 pixels @ %3 bits per pixel").arg(image.width()).arg(image.height()).arg(image.depth()));
		}
	}
	else if (!pixmap.isNull())
	{
		m_ui->sizeValueLabel->setText(tr("%1 x %2 pixels @ %3 bits per pixel").arg(pixmap.width()).arg(pixmap.height()).arg(pixmap.depth()));
	}

	if (device)
	{
		device->reset();

		m_ui->typeValueLabel->setText(QMimeDatabase().mimeTypeForData(device).comment());

		device->deleteLater();
	}
}

ImagePropertiesDialog::~ImagePropertiesDialog()
{
	delete m_ui;
}

void ImagePropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	switch (event->type())
	{
		case QEvent::LanguageChange:
			m_ui->retranslateUi(this);

			break;
		default:
			break;
	}
}

}
