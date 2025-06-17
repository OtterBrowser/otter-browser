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

#ifndef OTTER_OPMLIMPORTDATAEXCHANGERWIDGET_H
#define OTTER_OPMLIMPORTDATAEXCHANGERWIDGET_H

#include "../../../core/FeedsModel.h"

#include <QtWidgets/QWidget>

namespace Otter
{

namespace Ui
{
	class OpmlImportOptionsWidget;
}

class OpmlImportOptionsWidget final : public QWidget
{
public:
	explicit OpmlImportOptionsWidget(QWidget *parent = nullptr);
	~OpmlImportOptionsWidget();

	FeedsModel::Entry* getTargetFolder() const;
	bool areDuplicatesAllowed() const;

private:
	Ui::OpmlImportOptionsWidget *m_ui;
};

}

#endif
