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

#ifndef OTTER_PROGRESSBARWIDGET_H
#define OTTER_PROGRESSBARWIDGET_H

#include <QtWidgets/QProgressBar>

namespace Otter
{

class ProgressBarWidget final : public QProgressBar
{
	Q_OBJECT

public:
	enum StyleMode
	{
		NormalMode = 0,
		ThinMode
	};

	explicit ProgressBarWidget(QWidget *parent = nullptr);

	void setMode(StyleMode mode);
	void setHasError(bool hasError);
	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;
	bool hasError() const;

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	StyleMode m_mode;
	bool m_hasError;
};

}

#endif
