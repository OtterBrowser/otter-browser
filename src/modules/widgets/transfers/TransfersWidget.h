/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_TRANSFERSWIDGET_H
#define OTTER_TRANSFERSWIDGET_H

#include "../../../ui/ToolButtonWidget.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>

namespace Otter
{

class ProgressBarWidget;
class Transfer;
class Window;

class TransfersWidget final : public ToolButtonWidget
{
	Q_OBJECT

public:
	explicit TransfersWidget(const ToolBarsManager::ToolBarDefinition::Entry &definition, QWidget *parent = nullptr);

	QString getToolTip() const override;
	QIcon getIcon() const override;

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void populateMenu();
	void updateState();

private:
	QIcon m_icon;
};

class TransferActionWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit TransferActionWidget(Transfer *transfer, QWidget *parent = nullptr);

	Transfer* getTransfer() const;
	bool event(QEvent *event) override;

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

protected slots:
	void updateState();

private:
	Transfer *m_transfer;
	QLabel *m_detailsLabel;
	QLabel *m_fileNameLabel;
	QLabel *m_iconLabel;
	ProgressBarWidget *m_progressBar;
	QToolButton *m_toolButton;
	QWidget *m_centralWidget;
};

}

#endif
