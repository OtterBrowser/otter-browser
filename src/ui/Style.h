/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_STYLE_H
#define OTTER_STYLE_H

#include <QtWidgets/QProxyStyle>

namespace Otter
{

class Style : public QProxyStyle
{
	Q_OBJECT

public:
	enum StyleElement
	{
		TabAudioAudibleElement = 0,
		TabAudioMutedElement,
		ItemCloseElement
	};

	explicit Style(const QString &name);

	void drawDropZone(const QLine &line, QPainter *painter);
	virtual QString getName() const;
	QRect subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget = nullptr) const override;
	int pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
	int styleHint(QStyle::StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;

protected slots:
	void optionChanged(int identifier, const QVariant &value);

private:
	bool m_areToolTipsEnabled;
};

}

#endif
