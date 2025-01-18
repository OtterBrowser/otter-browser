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

#ifndef OTTER_APPLICATIONCOMBOBOXWIDGET_H
#define OTTER_APPLICATIONCOMBOBOXWIDGET_H

#include <QtCore/QMimeType>
#include <QtWidgets/QComboBox>

namespace Otter
{

class ApplicationComboBoxWidget final : public QComboBox
{
	Q_OBJECT

public:
	explicit ApplicationComboBoxWidget(QWidget *parent = nullptr);

	void setCurrentCommand(const QString &command);
	void setMimeType(const QMimeType &mimeType);
	void setAlwaysShowDefaultApplication(bool show);
	QString getCommand() const;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void handleIndexChanged(int index);

private:
	int m_previousIndex;
	bool m_alwaysShowDefaultApplication;

signals:
	void currentCommandChanged();
};

}

#endif
