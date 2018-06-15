/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TEXTBROWSERWIDGET_H
#define OTTER_TEXTBROWSERWIDGET_H

#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtWidgets/QTextBrowser>

namespace Otter
{

class TextBrowserWidget final : public QTextBrowser
{
public:
	enum ImagesPolicy
	{
		NoImages = 0,
		OnlyCachedImages,
		AllImages
	};

	explicit TextBrowserWidget(QWidget *parent = nullptr);

	void setImagesPolicy(ImagesPolicy policy);
	QVariant loadResource(int type, const QUrl &url) override;

private:
	QSet<QUrl> m_resources;
	ImagesPolicy m_imagesPolicy;
};

}

#endif
