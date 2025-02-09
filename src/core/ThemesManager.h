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

#ifndef OTTER_THEMESMANAGER_H
#define OTTER_THEMESMANAGER_H

#include "AddonsManager.h"

#ifdef Q_OS_WIN32
#include <QtCore/QAbstractNativeEventFilter>
#endif
#include <QtCore/QMap>
#include <QtCore/QMimeDatabase>
#include <QtWidgets/QFileIconProvider>
#include <QtWidgets/QStyle>

namespace Otter
{

class Animation;
class Style;

class ColorScheme final : public QObject, public Addon
{
	Q_OBJECT

public:
	enum ColorRole
	{
		NoRole = 0,
		WindowRole,
		WindowTextRole,
		BaseRole,
		AlternateBaseRole,
		ToolTipBaseRole,
		ToolTipTextRole,
		TextRole,
		ButtonRole,
		ButtonTextRole,
		SidebarRole,
		SidebarTextRole,
		TabRole,
		TabTextRole,
		BrightTextRole,
		LightRole,
		MidlightRole,
		DarkRole,
		MidRole,
		ShadowRole,
		HighlightRole,
		HighlightedTextRole,
		LinkRole,
		LinkVisitedRole
	};

	Q_ENUM(ColorRole)

	struct ColorRoleInformation final
	{
		QColor active;
		QColor disabled;
		QColor inactive;

		bool isValid() const
		{
			return (active.isValid() && disabled.isValid() && inactive.isValid());
		}
	};

	explicit ColorScheme(const QString &name, QObject *parent = nullptr);

	QString getName() const override;
	QString getTitle() const override;
	ColorRoleInformation getColor(ColorRole role) const;

private:
	QString m_name;
	QString m_title;
	QMap<ColorRole, ColorRoleInformation> m_colors;

	static int m_colorRoleEnumerator;
};

#ifdef Q_OS_WIN32
class ThemesManager final : public QObject, public QAbstractNativeEventFilter
#else
class ThemesManager final : public QObject
#endif
{
	Q_OBJECT

public:
	enum IconContext
	{
		GenericContext = 0,
		ContextMenuContext,
		MenuBarContext,
		ToolBarContext
	};

	static void createInstance();
	static ThemesManager* getInstance();
	static ColorScheme* getColorScheme();
	static Style* getStyle();
	static Style* createStyle(const QString &name);
	static Animation* createAnimation(const QString &name = QLatin1String("spinner"), QObject *parent = nullptr);
	static QIcon createIcon(const QString &name, bool fromTheme = true, IconContext context = GenericContext);
	static QIcon getFileTypeIcon(const QString &path);
	static QIcon getFileTypeIcon(const QMimeType &mimeType);

protected:
	explicit ThemesManager(QObject *parent);

	bool eventFilter(QObject *object, QEvent *event) override;
#ifdef Q_OS_WIN32
	bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
#endif

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	static ThemesManager *m_instance;
	static ColorScheme *m_colorScheme;
	static QWidget *m_probeWidget;
	static QString m_iconThemePath;
	static QFileIconProvider m_fileIconProvider;
	static QMimeDatabase m_mimeDatabase;
	static bool m_useSystemIconTheme;

signals:
	void iconThemeChanged();
	void widgetStyleChanged();
};

}

#endif
