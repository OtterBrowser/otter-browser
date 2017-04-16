/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef QTWEBKITPLUGIN_H
#define QTWEBKITPLUGIN_H

#include "qwebkitplatformplugin.h"

namespace Otter
{

class QtWebKitPlugin final : public QObject, public QWebKitPlatformPlugin
{
	Q_OBJECT
	Q_INTERFACES(QWebKitPlatformPlugin)
	Q_PLUGIN_METADATA(IID "org.qtwebkit.QtWebKit.QtWebKitPlugin")

public:
	explicit QtWebKitPlugin();

	QObject* createExtension(Extension extension) const override;
	bool supportsExtension(Extension extension) const override;
};

}

#endif
