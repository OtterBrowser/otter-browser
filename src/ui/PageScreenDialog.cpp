/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
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
#include "PageScreenDialog.h"
#include "ui_PageScreenDialog.h"
#include "../core/Application.h"
#include "../core/SettingsManager.h"

#ifdef OTTER_ENABLE_QTWEBKIT

#include <QMessageBox>
#include <QWebFrame>
#include <QWebView>
#include <QTimer>
#include <QPushButton>
#include <QCloseEvent>
#include <QPrinter>

#include <QtConcurrent/QtConcurrentRun>

namespace Otter
{

PageScreenDialog::PageScreenDialog(QWebView* view, QWidget* parent)
	: Dialog(parent)
	, ui(new Ui::PageScreenDialog)
	, m_spinnerAnimation(this)
	, m_view(view)
	, m_imageScaling(0)
{
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	m_formats.append(QStringLiteral("PNG"));
	m_formats.append(QStringLiteral("BMP"));
	m_formats.append(QStringLiteral("JPG"));
	m_formats.append(QStringLiteral("PPM"));
	m_formats.append(QStringLiteral("TIFF"));
	m_formats.append(QStringLiteral("PDF"));

	foreach (const QString &format, m_formats)
	{
		ui->formats->addItem(tr("Save as %1").arg(format));
	}

	m_pageTitle = m_view->title();

	const QString name = Utils::filterCharsFromFilename(m_pageTitle);
	const QString path = SettingsManager::getOption(SettingsManager::Paths_SaveFileOption).toString();
	ui->location->setText(QString("%1/%2.png").arg(path, name));

	connect(&m_spinnerAnimation, &Animation::frameChanged, ui->label, [&]()
	{
		ui->label->setPixmap(m_spinnerAnimation.getCurrentPixmap());
	});
	m_spinnerAnimation.start();

	connect(ui->changeLocation, SIGNAL(clicked()), this, SLOT(changeLocation()));
	connect(ui->formats, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged()));
	connect(ui->buttonBox->button(QDialogButtonBox::Save), SIGNAL(clicked()), this, SLOT(dialogAccepted()));
	connect(ui->buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(close()));

	QTimer::singleShot(200, this, SLOT(createThumbnail()));
}

void PageScreenDialog::formatChanged()
{
	QString text = ui->location->text();
	int pos = text.lastIndexOf(QLatin1Char('.'));

	if (pos > -1)
	{
		text = text.left(pos + 1) + m_formats.at(ui->formats->currentIndex()).toLower();
	}
	else
	{
		text.append(QLatin1Char('.') + m_formats.at(ui->formats->currentIndex()).toLower());
	}

	ui->location->setText(text);
}

void PageScreenDialog::changeLocation()
{
	const QString name = Utils::filterCharsFromFilename(m_pageTitle);
	const QString path = Utils::getSavePath(name + "." + m_formats.at(ui->formats->currentIndex()).toLower(),
	                                        SettingsManager::getOption(SettingsManager::Paths_SaveFileOption).toString(),
	                                        {}, true).path;
	if (!path.isEmpty())
	{
		ui->location->setText(path);
	}
}

void PageScreenDialog::dialogAccepted()
{
	if (!ui->location->text().isEmpty())
	{
		if (QFile::exists(ui->location->text()))
		{
			const QString text = tr("File '%1' already exists. Do you want to overwrite it?").arg(ui->location->text());
			QMessageBox::StandardButton button = QMessageBox::warning(this, tr("File already exists"), text,
			                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button != QMessageBox::Yes)
			{
				return;
			}
		}

		QApplication::setOverrideCursor(Qt::WaitCursor);

		const QString format = m_formats.at(ui->formats->currentIndex());
		if (format == QLatin1String("PDF"))
		{
			saveAsDocument(format);
		}
		else
		{
			saveAsImage(format);
		}

		QApplication::restoreOverrideCursor();

		close();
	}
}

void PageScreenDialog::saveAsImage(const QString &format)
{
	const QString suffix = QLatin1Char('.') + format.toLower();

	QString pathWithoutSuffix = ui->location->text();
	if (pathWithoutSuffix.endsWith(suffix, Qt::CaseInsensitive))
	{
		pathWithoutSuffix = pathWithoutSuffix.mid(0, pathWithoutSuffix.length() - suffix.length());
	}

	if (m_pageImages.count() == 1)
	{
		m_pageImages.first().save(pathWithoutSuffix + suffix, format.toUtf8());
	}
	else
	{
		int part = 1;
		foreach (const QImage &image, m_pageImages)
		{
			const QString fileName = pathWithoutSuffix + ".part" + QString::number(part);
			image.save(fileName + suffix, format.toUtf8());
			part++;
		}
	}
}

void PageScreenDialog::saveAsDocument(const QString &format)
{
	const QString suffix = QLatin1Char('.') + format.toLower();

	QString pathWithoutSuffix = ui->location->text();
	if (pathWithoutSuffix.endsWith(suffix, Qt::CaseInsensitive))
	{
		pathWithoutSuffix = pathWithoutSuffix.mid(0, pathWithoutSuffix.length() - suffix.length());
	}

	QPrinter printer;
	printer.setCreator(QStringLiteral("%1 %2").arg(QGuiApplication::applicationDisplayName(), Application::getFullVersion()));
	printer.setOutputFileName(pathWithoutSuffix + suffix);
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setPaperSize(m_pageImages.first().size(), QPrinter::DevicePixel);
	printer.setPageMargins(0, 0, 0, 0, QPrinter::DevicePixel);
	printer.setFullPage(true);

	QPainter painter;
	painter.begin(&printer);

	for (int i = 0; i < m_pageImages.size(); ++i)
	{
		const QImage image = m_pageImages.at(i);
		painter.drawImage(0, 0, image);

		if (i != m_pageImages.size() - 1)
		{
			printer.newPage();
		}
	}

	painter.end();
}

void PageScreenDialog::createThumbnail()
{
	QWebPage* page = m_view->page();

	const int heightLimit = 20000;
	const QPoint originalScrollPosition = page->mainFrame()->scrollPosition();
	const QSize originalSize = page->viewportSize();
	const QSize frameSize = page->mainFrame()->contentsSize();
	const int verticalScrollbarSize = page->mainFrame()->scrollBarGeometry(Qt::Vertical).width();
	const int horizontalScrollbarSize = page->mainFrame()->scrollBarGeometry(Qt::Horizontal).height();

	int yPosition = 0;
	bool canScroll = frameSize.height() > heightLimit;

	// We will split rendering page into smaller parts to avoid infinite loops
	// or crashes.

	do
	{
		int remainingHeight = frameSize.height() - yPosition;
		if (remainingHeight <= 0)
		{
			break;
		}

		QSize size(frameSize.width(),
		           remainingHeight > heightLimit ? heightLimit : remainingHeight);
		page->setViewportSize(size);
		page->mainFrame()->scroll(0, qMax(0, yPosition - horizontalScrollbarSize));

		QImage image(page->viewportSize().width() - verticalScrollbarSize,
		             page->viewportSize().height() - horizontalScrollbarSize,
		             QImage::Format_ARGB32_Premultiplied);
		QPainter painter(&image);
		page->mainFrame()->render(&painter);
		painter.end();

		m_pageImages.append(image);

		canScroll = remainingHeight > heightLimit;
		yPosition += size.height();
	}
	while (canScroll);

	page->setViewportSize(originalSize);
	page->mainFrame()->setScrollBarValue(Qt::Vertical, originalScrollPosition.y());
	page->mainFrame()->setScrollBarValue(Qt::Horizontal, originalScrollPosition.x());

	m_imageScaling = new QFutureWatcher<QImage>(this);
	m_imageScaling->setFuture(QtConcurrent::run(this, &PageScreenDialog::scaleImage));
	connect(m_imageScaling, SIGNAL(finished()), SLOT(showImage()));
}

QImage PageScreenDialog::scaleImage()
{
	QVector<QImage> scaledImages;
	int sumHeight = 0;

	foreach (const QImage &image, m_pageImages)
	{
		QImage scaled = image.scaledToWidth(450, Qt::SmoothTransformation);

		scaledImages.append(scaled);
		sumHeight += scaled.height();
	}

	QImage finalImage(QSize(450, sumHeight), QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&finalImage);

	int offset = 0;
	foreach (const QImage &image, scaledImages)
	{
		painter.drawImage(0, offset, image);
		offset += image.height();
	}

	return finalImage;
}

void PageScreenDialog::showImage()
{
	m_spinnerAnimation.stop();

	ui->label->setPixmap(QPixmap::fromImage(m_imageScaling->result()));
}

PageScreenDialog::~PageScreenDialog()
{
	delete ui;
}

}

#endif //OTTER_ENABLE_QTWEBKIT
