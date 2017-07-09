/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "ThemesManager.h"
#include "Application.h"
#include "PlatformIntegration.h"
#include "SettingsManager.h"
#include "../ui/Style.h"

#if defined(Q_OS_WIN32)
#include <QtCore/QAbstractEventDispatcher>
#endif
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QIcon>
#include <QtWidgets/QWidget>

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

namespace Otter
{

ThemesManager* ThemesManager::m_instance(nullptr);
QWidget* ThemesManager::m_probeWidget(nullptr);
QString ThemesManager::m_iconThemePath(QLatin1String(":/icons/theme/"));
bool ThemesManager::m_useSystemIconTheme(false);

ThemesManager::ThemesManager(QObject *parent) : QObject(parent)
{
	m_useSystemIconTheme = SettingsManager::getOption(SettingsManager::Interface_UseSystemIconThemeOption).toBool();

	handleOptionChanged(SettingsManager::Interface_IconThemePathOption, SettingsManager::getOption(SettingsManager::Interface_IconThemePathOption));

	connect(SettingsManager::getInstance(), SIGNAL(optionChanged(int,QVariant)), this, SLOT(handleOptionChanged(int,QVariant)));
}

void ThemesManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new ThemesManager(QCoreApplication::instance());
		m_probeWidget = new QWidget();
		m_probeWidget->hide();
		m_probeWidget->setAttribute(Qt::WA_DontShowOnScreen, true);
		m_probeWidget->installEventFilter(m_instance);

#if defined(Q_OS_WIN32)
		QAbstractEventDispatcher::instance()->installNativeEventFilter(m_instance);
#endif
	}
}

void ThemesManager::handleOptionChanged(int identifier, const QVariant &value)
{
	switch (identifier)
	{
		case SettingsManager::Interface_IconThemePathOption:
			{
				QString path(value.toString());

				if (path.isEmpty())
				{
					path = QLatin1String(":/icons/theme/");
				}
				else if (!path.endsWith(QDir::separator()))
				{
					path.append(QDir::separator());
				}

				if (path != m_iconThemePath)
				{
					m_iconThemePath = path;

					emit iconThemeChanged();
				}
			}

			break;
		case SettingsManager::Interface_UseSystemIconThemeOption:
			if (value.toBool() != m_useSystemIconTheme)
			{
				m_useSystemIconTheme = value.toBool();

				emit iconThemeChanged();
			}
		default:
			break;
	}
}

ThemesManager* ThemesManager::getInstance()
{
	return m_instance;
}

Style* ThemesManager::createStyle(const QString &name)
{
	Style *style(nullptr);
	const PlatformIntegration *integration(Application::getPlatformIntegration());

	if (integration)
	{
		style = integration->createStyle(name);
	}

	if (!style)
	{
		style = new Style(name);
	}

	return style;
}

QIcon ThemesManager::createIcon(const QString &name, bool fromTheme)
{
	if (m_useSystemIconTheme && fromTheme && QIcon::hasThemeIcon(name))
	{
		return QIcon::fromTheme(name);
	}

	const QString iconPath((!fromTheme && name == QLatin1String("otter-browser")) ? QLatin1String(":/icons/otter-browser") : m_iconThemePath + name);
	const QString svgPath(iconPath + QLatin1String(".svg"));

	if (QFile::exists(svgPath))
	{
		return QIcon(svgPath);
	}

	return QIcon(iconPath + QLatin1String(".png"));
}

bool ThemesManager::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_probeWidget && event->type() == QEvent::StyleChange)
	{
		if (!QApplication::style()->inherits("Otter::Style"))
		{
			const QList<QStyle*> children(QApplication::style()->findChildren<QStyle*>());
			bool hasFound(false);

			for (int i = 0; i < children.count(); ++i)
			{
				if (children.at(i)->inherits("Otter::Style"))
				{
					hasFound = true;

					break;
				}
			}

			if (!hasFound)
			{
				QApplication::setStyle(createStyle(SettingsManager::getOption(SettingsManager::Interface_WidgetStyleOption).toString()));
			}
		}

		emit widgetStyleChanged();
	}

	return QObject::eventFilter(object, event);
}

#if defined(Q_OS_WIN32)
bool ThemesManager::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
	Q_UNUSED(eventType)
	Q_UNUSED(result)

	const MSG *nativeMessage(static_cast<MSG*>(message));

	if (nativeMessage && nativeMessage->message == WM_THEMECHANGED)
	{
		emit widgetStyleChanged();
	}

	return false;
}
#endif

}
