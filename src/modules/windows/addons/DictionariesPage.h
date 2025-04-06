/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2022 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef OTTER_DICTIONARIESPAGE_H
#define OTTER_DICTIONARIESPAGE_H

#include "AddonsPage.h"
#include "../../../core/SpellCheckManager.h"

namespace Otter
{

class DictionariesPage final : public AddonsPage
{
	Q_OBJECT

public:
	explicit DictionariesPage(bool needsDetails, QWidget *parent);

	QString getTitle() const override;

public slots:
	void addAddon() override;
	void save() override;

protected:
	void addAddonEntry(Addon *addon, const QMap<int, QVariant> &metaData = {}) override;
	void updateAddonEntry(Addon *addon) override;
	void delayedLoad() override;
	QVariant getAddonIdentifier(Addon *addon) const override;
	QStringList getSelectedDictionaries() const;
	QVector<ModelColumn> getModelColumns() const override;
	bool canOpenAddons() const override;

protected slots:
	void openAddons() override;
	void removeAddons() override;
	void updateDetails() override;

private:
	QStringList m_filesToRemove;
	QVector<SpellCheckManager::DictionaryInformation> m_dictionariesToAdd;
};

}

#endif
