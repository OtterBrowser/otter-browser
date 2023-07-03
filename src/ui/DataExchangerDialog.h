/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 - 2015 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2015 - 2023 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_DATAEXCHANGERDIALOG_H
#define OTTER_DATAEXCHANGERDIALOG_H

#include "Dialog.h"
#include "../core/DataExchanger.h"

namespace Otter
{

namespace Ui
{
	class DataExchangerDialog;
}

class DataExchangerDialog final : public Dialog
{
	Q_OBJECT

public:
	~DataExchangerDialog();

	static void createDialog(const QString &exchangerName, QWidget *parent);

protected:
	explicit DataExchangerDialog(ExportDataExchanger *exporter, QWidget *parent);
	explicit DataExchangerDialog(ImportDataExchanger *importer, QWidget *parent);

	void closeEvent(QCloseEvent *event) override;
	void changeEvent(QEvent *event) override;
	void setupResults(DataExchanger *exchanger);

protected slots:
	void handleExchangeProgress(DataExchanger::ExchangeType type, int total, int amount);
	void handleExchangeFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total);
	void handleExportFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total);
	void handleImportFinished(DataExchanger::ExchangeType type, DataExchanger::OperationResult result, int total);

private:
	ExportDataExchanger *m_exporter;
	ImportDataExchanger *m_importer;
	Ui::DataExchangerDialog *m_ui;
};

}

#endif
