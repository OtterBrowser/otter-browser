/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "QtWebKitPlugin.h"
#include "QtWebKitNotificationPresenter.h"
#ifdef OTTER_ENABLE_SPELLCHECK
#include "QtWebKitSpellChecker.h"
#endif

#include <QtCore/QtPlugin>

namespace Otter
{

QtWebKitPlugin::QtWebKitPlugin() = default;

QObject* QtWebKitPlugin::createExtension(Extension extension) const
{
	if (extension == Notifications)
	{
		return new QtWebKitNotificationPresenter();
	}

#ifdef OTTER_ENABLE_SPELLCHECK
	if (extension == SpellChecker)
	{
		return new QtWebKitSpellChecker();
	}
#endif

	return nullptr;
}

bool QtWebKitPlugin::supportsExtension(Extension extension) const
{
#ifdef OTTER_ENABLE_SPELLCHECK
	return (extension == Notifications || extension == SpellChecker);
#else
	return (extension == Notifications);
#endif
}

}

Q_IMPORT_PLUGIN(QtWebKitPlugin)
