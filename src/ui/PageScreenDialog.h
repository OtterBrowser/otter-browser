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

#ifndef PAGESCREEN_H
#define PAGESCREEN_H

#ifdef OTTER_ENABLE_QTWEBKIT

#include <QFutureWatcher>
#include "Dialog.h"
#include "../ui/Animation.h"

class QWebView;

namespace Otter
{

namespace Ui
{
class PageScreenDialog;
}

class PageScreenDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit PageScreenDialog(QWebView* view, QWidget* parent);
	~PageScreenDialog();

	QImage scaleImage();

private slots:
	void createThumbnail();
	void showImage();

	void formatChanged();
	void changeLocation();
	void dialogAccepted();

private:
	void saveAsImage(const QString &format);
	void saveAsDocument(const QString &format);

	void createPixmap();

	Ui::PageScreenDialog* ui;
	SpinnerAnimation m_spinnerAnimation;
	QWebView* m_view;
	QString m_pageTitle;

	QFutureWatcher<QImage>* m_imageScaling;
	QVector<QImage> m_pageImages;
	QStringList m_formats;
};

}

#endif //OTTER_ENABLE_QTWEBKIT
#endif // PAGESCREEN_H
