/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_USERSCRIPTSPAGE_H
#define OTTER_USERSCRIPTSPAGE_H

#include "AddonsPage.h"

namespace Otter
{

class UserScriptsPage final : public AddonsPage
{
	Q_OBJECT

public:
	enum ReplaceMode
	{
		UnknownMode = 0,
		ReplaceAllMode,
		IgnoreAllMode
	};

	explicit UserScriptsPage(bool needsDetails, QWidget *parent);

	QString getTitle() const override;

public slots:
	void addAddon() override;
	void save() override;

protected:
	void delayedLoad() override;
	QIcon getFallbackIcon() const override;
	QVariant getAddonIdentifier(Addon *addon) const override;
	QVector<UserScript*> getSelectedUserScripts() const;
	bool canOpenAddons() const override;
	bool canReloadAddons() const override;

protected slots:
	void openAddons() override;
	void reloadAddons() override;
	void removeAddons() override;
	void updateDetails() override;

private:
	QStringList m_addonsToAdd;
	QStringList m_addonsToRemove;
};

}

#endif
