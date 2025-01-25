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

#ifndef OTTER_OPMLIMPORTDATAEXCHANGER_H
#define OTTER_OPMLIMPORTDATAEXCHANGER_H

#include "../../../core/DataExchanger.h"
#include "../../../core/FeedsModel.h"

namespace Otter
{

class OpmlImportOptionsWidget;

class OpmlImportDataExchanger final : public ImportDataExchanger
{
	Q_OBJECT

public:
	explicit OpmlImportDataExchanger(QObject *parent = nullptr);

	QWidget* createOptionsWidget(QWidget *parent) override;
	QString getName() const override;
	QString getTitle() const override;
	QString getDescription() const override;
	QString getVersion() const override;
	QString getSuggestedPath(const QString &path = {}) const override;
	QString getGroup() const override;
	QUrl getHomePage() const override;
	QStringList getFileFilters() const override;
	ExchangeType getExchangeType() const override;
	bool hasOptions() const override;

public slots:
	bool importData(const QString &path) override;

protected:
	void importFolder(FeedsModel::Entry *source, FeedsModel::Entry *target, bool areDuplicatesAllowed);

private:
	OpmlImportOptionsWidget *m_optionsWidget;
};

}

#endif
