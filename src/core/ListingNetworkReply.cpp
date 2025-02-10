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

#include "ListingNetworkReply.h"
#include "Application.h"
#include "SessionsManager.h"
#include "ThemesManager.h"
#include "Utils.h"

#include <QtCore/QBuffer>
#include <QtCore/QRegularExpression>
#include <QtCore/QtMath>
#include <QtWidgets/QFileIconProvider>

namespace Otter
{

ListingNetworkReply::ListingNetworkReply(const QNetworkRequest &request, QObject *parent) : QNetworkReply(parent)
{
	setRequest(request);
}

QByteArray ListingNetworkReply::createListing(const QString &title, const QVector<ListingNetworkReply::NavigationEntry> &navigation, const QVector<ListingNetworkReply::ListingEntry> &entries)
{
	const QRegularExpression entryExpression(QLatin1String("<!--entry:begin-->(.*)<!--entry:end-->"), (QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption));
	QFile file(SessionsManager::getReadableDataPath(QLatin1String("files/listing.html")));
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	QString navigationHtml;
	QString entriesHtml;
	QString listingTemplate(stream.readAll());
	const QString entryTemplate(entryExpression.match(listingTemplate).captured(1));

	listingTemplate.replace(entryExpression, QLatin1String("{entries}"));

	for (int i = 0; i < navigation.count(); ++i)
	{
		const NavigationEntry entry(navigation.at(i));

		navigationHtml.append(QStringLiteral("<a href=\"%1\">%2</a>").arg(entry.url.toString(), entry.name) + ((i < (navigation.count() - 1)) ? QLatin1String("&shy;") : QString()));
	}

	QHash<QString, QIcon> icons;
	const QFileIconProvider iconProvider;

	for (int i = 0; i < entries.count(); ++i)
	{
		const ListingEntry &entry(entries.at(i));

		if (!icons.contains(entry.mimeType.name()))
		{
			QIcon icon;

			switch (entry.type)
			{
				case ListingEntry::DirectoryType:
					icon = iconProvider.icon(QFileIconProvider::Folder);

					break;
				case ListingEntry::DriveType:
					icon = iconProvider.icon(QFileIconProvider::Drive);

					break;
				case ListingEntry::FileType:
					icon = iconProvider.icon(QFileIconProvider::File);

					break;
				default:
					break;
			}

			icon = QIcon::fromTheme(entry.mimeType.iconName(), icon);

			if (icon.isNull())
			{
				switch (entry.type)
				{
					case ListingEntry::DriveType:
					case ListingEntry::DirectoryType:
						icon = ThemesManager::createIcon(QLatin1String("inode-directory"), false);

						break;
					case ListingEntry::FileType:
						icon = ThemesManager::createIcon(QLatin1String("unknown"), false);

						break;
					default:
						icon = ThemesManager::createIcon((entry.isSymlink ? QLatin1String("link") : QLatin1String("unknown")), false);

						break;
				}
			}

			icons[entry.mimeType.name()] = icon;
		}

		QStringList classes;

		if (entry.type == ListingEntry::DirectoryType || entry.type == ListingEntry::FileType)
		{
			classes.append(QLatin1String("directory"));
		}
		else
		{
			classes.append(QLatin1String("file"));
		}

		if (entry.isSymlink)
		{
			classes.append(QLatin1String("link"));
		}

		classes.append(QLatin1String("icon_") + Utils::createIdentifier(entry.mimeType.name()));

		QHash<QString, QString> variables;
		variables[QLatin1String("class")] = classes.join(QLatin1Char(' '));
		variables[QLatin1String("url")] = entry.url.toString().toHtmlEscaped();
		variables[QLatin1String("mimeType")] = entry.mimeType.name().toHtmlEscaped();
		variables[QLatin1String("name")] = entry.name.toHtmlEscaped();
		variables[QLatin1String("comment")] = entry.mimeType.comment().toHtmlEscaped();
		variables[QLatin1String("size")] = ((entry.type == ListingEntry::FileType) ? Utils::formatUnit(entry.size, false, 2) : QString());
		variables[QLatin1String("lastModified")] = Utils::formatDateTime(entry.timeModified).toHtmlEscaped();

		entriesHtml.append(Utils::substitutePlaceholders(entryTemplate, variables));
	}

	QString styleHtml;
	const int iconSize(16 * qCeil(Application::getInstance()->devicePixelRatio()));
	QHash<QString, QIcon>::iterator iterator;

	for (iterator = icons.begin(); iterator != icons.end(); ++iterator)
	{
		styleHtml.append(QStringLiteral("tr td:first-child.icon_%1\n{\n\tbackground-image:url(\"%2\");\n}\n").arg(Utils::createIdentifier(iterator.key()), Utils::savePixmapAsDataUri(iterator.value().pixmap(iconSize, iconSize))));
	}

	QHash<QString, QString> variables;
	variables[QLatin1String("title")] = title.toHtmlEscaped();
	variables[QLatin1String("description")] = tr("Directory Contents").toHtmlEscaped();
	variables[QLatin1String("dir")] = (Application::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"));
	variables[QLatin1String("style")] = styleHtml;
	variables[QLatin1String("navigation")] = navigationHtml;
	variables[QLatin1String("entries")] = entriesHtml;
	variables[QLatin1String("headerName")] = tr("Name").toHtmlEscaped();
	variables[QLatin1String("headerType")] = tr("Type").toHtmlEscaped();
	variables[QLatin1String("headerSize")] = tr("Size").toHtmlEscaped();
	variables[QLatin1String("headerDate")] = tr("Date").toHtmlEscaped();

	return Utils::substitutePlaceholders(listingTemplate, variables).toUtf8();
}

}
