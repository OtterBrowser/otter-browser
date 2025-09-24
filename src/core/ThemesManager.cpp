/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
#include "../ui/Animation.h"
#include "../ui/Style.h"

#ifdef Q_OS_WIN32
#include <QtCore/QAbstractEventDispatcher>
#endif
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtGui/QIcon>
#include <QtWidgets/QWidget>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace Otter
{

int ColorScheme::m_colorRoleEnumerator(-1);

ColorScheme::ColorScheme(const QString &name, QObject *parent) : QObject(parent),
	m_name(name)
{
	if (m_colorRoleEnumerator < 0)
	{
		m_colorRoleEnumerator = staticMetaObject.indexOfEnumerator(QLatin1String("ColorRole").data());
	}

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("colors/") + name + QLatin1String(".json")));

	if (!QFile::exists(path))
	{
		return;
	}

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	const EnumeratorMapper enumeratorMapper(staticMetaObject.enumerator(m_colorRoleEnumerator), QLatin1String("Role"));
	const QJsonArray colorsArray(QJsonDocument::fromJson(file.readAll()).array());

	for (int i = 0; i < colorsArray.count(); ++i)
	{
		const QJsonObject colorRoleObject(colorsArray.at(i).toObject());
		ColorRoleInformation colorRoleInformation;
		colorRoleInformation.active = QColor(colorRoleObject.value(QLatin1String("active")).toString());
		colorRoleInformation.disabled = QColor(colorRoleObject.value(QLatin1String("disabled")).toString());
		colorRoleInformation.inactive = QColor(colorRoleObject.value(QLatin1String("inactive")).toString());

		m_colors[static_cast<ColorRole>(enumeratorMapper.mapToValue(colorRoleObject.value(QLatin1String("role")).toString()))] = colorRoleInformation;
	}

	file.close();
}

QString ColorScheme::getName() const
{
	return m_name;
}

QString ColorScheme::getTitle() const
{
	return m_title;
}

ColorScheme::ColorRoleInformation ColorScheme::getColor(ColorRole role) const
{
	if (!m_colors.contains(role))
	{
		switch (role)
		{
			case SidebarRole:
			case TabRole:
				role = WindowRole;

				break;
			case SidebarTextRole:
			case TabTextRole:
				role = WindowTextRole;

				break;
			default:
				break;
		}
	}

	return m_colors.value(role);
}

ThemesManager* ThemesManager::m_instance(nullptr);
ColorScheme* ThemesManager::m_colorScheme(nullptr);
QWidget* ThemesManager::m_probeWidget(nullptr);
QString ThemesManager::m_iconThemePath(QLatin1String(":/icons/theme/"));
QFileIconProvider ThemesManager::m_fileIconProvider;
QMimeDatabase ThemesManager::m_mimeDatabase;
bool ThemesManager::m_useSystemIconTheme(false);

ThemesManager::ThemesManager(QObject *parent) : QObject(parent)
{
	m_useSystemIconTheme = SettingsManager::getOption(SettingsManager::Interface_UseSystemIconThemeOption).toBool();

	handleOptionChanged(SettingsManager::Interface_IconThemePathOption, SettingsManager::getOption(SettingsManager::Interface_IconThemePathOption));

	connect(SettingsManager::getInstance(), &SettingsManager::optionChanged, this, &ThemesManager::handleOptionChanged);
}

void ThemesManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new ThemesManager(QCoreApplication::instance());
		m_colorScheme = new ColorScheme(SettingsManager::getOption(SettingsManager::Interface_ColorSchemeOption).toString(), m_instance);
		m_probeWidget = new QWidget();
		m_probeWidget->hide();
		m_probeWidget->setAttribute(Qt::WA_DontShowOnScreen, true);
		m_probeWidget->installEventFilter(m_instance);

#ifdef Q_OS_WIN32
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

			break;
		default:
			break;
	}
}

ThemesManager* ThemesManager::getInstance()
{
	return m_instance;
}

ColorScheme* ThemesManager::getColorScheme()
{
	return m_colorScheme;
}

Style* ThemesManager::getStyle()
{
	Style *style(qobject_cast<Style*>(QApplication::style()));

	if (style)
	{
		return style;
	}

	return QApplication::style()->findChild<Style*>();
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

Animation* ThemesManager::createAnimation(const QString &name, QObject *parent)
{
	if (!parent)
	{
		parent = QCoreApplication::instance();
	}

	QString animationPath;
	const QString iconPath(m_iconThemePath + name);
	const QString svgPath(iconPath + QLatin1String(".svg"));

	if (QFile::exists(svgPath))
	{
		animationPath = svgPath;
	}
	else
	{
		const QString gifPath(iconPath + QLatin1String(".gif"));

		if (QFile::exists(gifPath))
		{
			animationPath = gifPath;
		}
	}

	if (animationPath.isEmpty())
	{
		return new SpinnerAnimation(parent);
	}

	return new GenericAnimation(animationPath, parent);
}

QIcon ThemesManager::createIcon(const QString &name, bool fromTheme, IconContext context)
{
	Q_UNUSED(context)

	if (name.isEmpty())
	{
		return {};
	}

	if (name.startsWith(QLatin1String("data:image/")))
	{
		return {Utils::loadPixmapFromDataUri(name)};
	}

	if (m_useSystemIconTheme && fromTheme && QIcon::hasThemeIcon(name))
	{
		return QIcon::fromTheme(name);
	}

	const QString iconPath((!fromTheme && name == QLatin1String("otter-browser")) ? QLatin1String(":/icons/otter-browser") : m_iconThemePath + name);
	const QString svgPath(iconPath + QLatin1String(".svg"));
	const QString rasterPath(iconPath + QLatin1String(".png"));

	if (QFile::exists(svgPath))
	{
		return QIcon(svgPath);
	}

	if (QFile::exists(rasterPath))
	{
		return QIcon(rasterPath);
	}

	return {};
}

QIcon ThemesManager::getFileTypeIcon(const QString &path)
{
	const QString iconName(m_mimeDatabase.mimeTypeForFile(path).iconName());

	return QIcon::fromTheme(iconName, m_fileIconProvider.icon(QFileInfo(path)));
}

QIcon ThemesManager::getFileTypeIcon(const QMimeType &mimeType)
{
	const QString iconName(mimeType.iconName());

	return QIcon::fromTheme(iconName, m_fileIconProvider.icon(QFileIconProvider::File));
}

bool ThemesManager::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_probeWidget && event->type() == QEvent::StyleChange)
	{
		if (!QApplication::style()->inherits("Otter::Style"))
		{
			const QList<QStyle*> children(QApplication::style()->findChildren<QStyle*>());
			bool hasMatch(false);

			for (int i = 0; i < children.count(); ++i)
			{
				if (children.at(i)->inherits("Otter::Style"))
				{
					hasMatch = true;

					break;
				}
			}

			if (!hasMatch)
			{
				QApplication::setStyle(createStyle(SettingsManager::getOption(SettingsManager::Interface_WidgetStyleOption).toString()));
			}
		}

		emit widgetStyleChanged();
	}

	return QObject::eventFilter(object, event);
}

#ifdef Q_OS_WIN32
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
