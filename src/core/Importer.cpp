/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "Importer.h"

namespace Otter
{

Importer::Importer(QObject *parent) : QObject(parent), Addon()
{
}

void Importer::cancel()
{
}

void Importer::notifyImportStarted(int type, int total)
{
	emit importStarted(static_cast<ImportType>(type), total);
}

void Importer::notifyImportProgress(int type, int total, int amount)
{
	emit importProgress(static_cast<ImportType>(type), total, amount);
}

void Importer::notifyImportFinished(int type, int result, int total)
{
	emit importFinished(static_cast<ImportType>(type), static_cast<ImportResult>(result), total);
}

Addon::AddonType Importer::getType() const
{
	return ImporterType;
}

bool Importer::canCancel()
{
	return false;
}

}
