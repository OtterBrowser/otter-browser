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

#ifndef OTTER_THEMESMANAGER_H
#define OTTER_THEMESMANAGER_H

#include <QtCore/QObject>
#if defined(Q_OS_WIN32)
#include <QtCore/QAbstractNativeEventFilter>
#endif
#include <QtWidgets/QStyle>

namespace Otter
{

class Style;

#if defined(Q_OS_WIN32)
class ThemesManager final : public QObject, public QAbstractNativeEventFilter
#else
class ThemesManager final : public QObject
#endif
{
	Q_OBJECT

public:
	static void createInstance(QObject *parent = nullptr);
	static ThemesManager* getInstance();
	static Style* createStyle(const QString &name);
	static QIcon getIcon(const QString &name, bool fromTheme = true);

protected:
	explicit ThemesManager(QObject *parent = nullptr);

	bool eventFilter(QObject *object, QEvent *event) override;
#if defined(Q_OS_WIN32)
	bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
#endif

protected slots:
	void handleOptionChanged(int identifier, const QVariant &value);

private:
	static ThemesManager *m_instance;
	static QWidget *m_probeWidget;
	static QString m_iconThemePath;
	static bool m_useSystemIconTheme;

signals:
	void iconThemeChanged();
	void widgetStyleChanged();
};

}

#endif
