/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include <QtWidgets/QStyleOptionProgressBar>

namespace Otter
{

class Style : public QProxyStyle
{
	Q_OBJECT

public:
	enum ExtraStyleHint
	{
		InvalidHint = 0,
		CanAlignTabBarLabelHint
	};

	enum ExtraStyleElement
	{
		InvalidElement = 0,
		TabAudioAudibleElement,
		TabAudioMutedElement,
		ItemCloseElement
	};

	explicit Style(const QString &name);

	void drawIconOverlay(const QRect &iconRectangle, const QIcon &overlayIcon, QPainter *painter) const;
	void drawDropZone(const QLine &line, QPainter *painter) const;
	void drawToolBarEdge(const QStyleOption *option, QPainter *painter) const;
	void drawThinProgressBar(const QStyleOptionProgressBar *option, QPainter *painter) const;
	void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
	void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
	virtual QString getName() const;
	virtual QString getStyleSheet() const;
	QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget = nullptr) const override;
	QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &contentsSize, const QWidget *widget = nullptr) const override;
	int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
	int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;
	virtual int getExtraStyleHint(ExtraStyleHint hint) const;

private:
	bool m_areToolTipsEnabled;
	bool m_canAlignTabBarLabel;
};

}

#endif
