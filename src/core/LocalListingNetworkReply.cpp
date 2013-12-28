#include "LocalListingNetworkReply.h"
#include "Utils.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>

namespace Otter
{

LocalListingNetworkReply::LocalListingNetworkReply(QObject *parent, const QNetworkRequest &request) : QNetworkReply(parent),
	m_offset(0)
{
	setRequest(request);

	open(QIODevice::ReadOnly | QIODevice::Unbuffered);

	QFile file(QLatin1String(":/files/listing.html"));
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	QDir directory(request.url().toLocalFile());
	const QFileInfoList entries = directory.entryInfoList((QDir::AllEntries | QDir::Hidden), (QDir::Name | QDir::DirsFirst));
	QStringList navigation;

	do
	{
		navigation.prepend(QString("<a href=\"%1\">%2</a>%3").arg(directory.canonicalPath()).arg(directory.dirName().isEmpty() ? QString('/') : directory.dirName()).arg(directory.dirName().isEmpty() ? QString() : QString('/')));
	}
	while (directory.cdUp());

	QHash<QString, QString> variables;
	variables[QLatin1String("title")] = QFileInfo(request.url().toLocalFile()).canonicalFilePath();
	variables[QLatin1String("description")] = tr("Directory Contents");
	variables[QLatin1String("dir")] = (QGuiApplication::isLeftToRight() ? QLatin1String("ltr") : QLatin1String("rtl"));
	variables[QLatin1String("navigation")] = navigation.join(QString());
	variables[QLatin1String("header_name")] = tr("Name");
	variables[QLatin1String("header_type")] = tr("Type");
	variables[QLatin1String("header_size")] = tr("Size");
	variables[QLatin1String("header_date")] = tr("Date");
	variables[QLatin1String("body")] = QString();

	QMimeDatabase database;

	for (int i = 0; i < entries.count(); ++i)
	{
		const QMimeType mimeType = database.mimeTypeForFile(entries.at(i).canonicalFilePath());
		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		QIcon::fromTheme(mimeType.iconName(), Utils::getIcon(entries.at(i).isDir() ? "inode-directory" : "unknown")).pixmap(16, 16).save(&buffer, "PNG");

		variables[QLatin1String("body")].append(QString("<tr>\n<td><a href=\"file://%1\"><img src=\"data:image/png;base64,%2\" alt=\"\"> %3</a></td>\n<td>%4</td>\n<td>%5</td>\n<td>%6</td>\n</tr>\n").arg(entries.at(i).filePath()).arg(QString(byteArray.toBase64())).arg(entries.at(i).fileName()).arg(mimeType.comment()).arg(entries.at(i).isDir() ? QString() : Utils::formatUnit(entries.at(i).size(), false, 2)).arg(entries.at(i).lastModified().toString()));
	}

	QString html = stream.readAll();
	QHash<QString, QString>::iterator iterator;

	for (iterator = variables.begin(); iterator != variables.end(); ++iterator)
	{
		html.replace(QString("{%1}").arg(iterator.key()), iterator.value());
	}

	m_content = html.toUtf8();

	setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/html; charset=UTF-8"));
	setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_content.size()));

	QTimer::singleShot(0, this, SIGNAL(readyRead()));
	QTimer::singleShot(0, this, SIGNAL(finished()));
}

void LocalListingNetworkReply::abort()
{
}

qint64 LocalListingNetworkReply::bytesAvailable() const
{
	return (m_content.size() - m_offset);
}

qint64 LocalListingNetworkReply::readData(char *data, qint64 maxSize)
{
	if (m_offset < m_content.size())
	{
		qint64 number = qMin(maxSize, m_content.size() - m_offset);

		memcpy(data, (m_content.constData() + m_offset), number);

		m_offset += number;

		return number;
	}

	return -1;
}

bool LocalListingNetworkReply::isSequential() const
{
	return true;
}

}
