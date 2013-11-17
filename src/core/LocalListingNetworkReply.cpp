#include "LocalListingNetworkReply.h"

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

	QFile file(":/files/listing.html");
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
	variables["title"] = QFileInfo(request.url().toLocalFile()).canonicalFilePath();
	variables["description"] = tr("Directory Contents");
	variables["dir"] = (QGuiApplication::isLeftToRight() ? "ltr" : "rtl");
	variables["navigation"] = navigation.join(QString());
	variables["header_name"] = tr("Name");
	variables["header_type"] = tr("Type");
	variables["header_size"] = tr("Size");
	variables["header_date"] = tr("Date");
	variables["body"] = QString();

	QMimeDatabase database;

	for (int i = 0; i < entries.count(); ++i)
	{
		const QMimeType mimeType = database.mimeTypeForFile(entries.at(i).canonicalFilePath());
		QString size;

		if (!entries.at(i).isDir())
		{
			if (entries.at(i).size() > 1024)
			{
				if (entries.at(i).size() > 1048576)
				{
					if (entries.at(i).size() > 1073741824)
					{
						size = QString("%1 GB").arg((entries.at(i).size() / 1073741824.0), 0, 'f', 2);
					}
					else
					{
						size = QString("%1 MB").arg((entries.at(i).size() / 1048576.0), 0, 'f', 2);
					}
				}
				else
				{
					size = QString("%1 KB").arg((entries.at(i).size() / 1024.0), 0, 'f', 2);
				}
			}
			else
			{
				size = QString("%1 B").arg(entries.at(i).size());
			}
		}

		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		QIcon::fromTheme(mimeType.iconName(), QIcon(entries.at(i).isDir() ? ":icons/inode-directory.png" : ":/icons/unknown.png")).pixmap(16, 16).save(&buffer, "PNG");

		variables["body"].append(QString("<tr>\n<td><a href=\"file://%1\"><img src=\"data:image/png;base64,%2\" alt=\"\"> %3</a></td>\n<td>%4</td>\n<td>%5</td>\n<td>%6</td>\n</tr>\n").arg(entries.at(i).filePath()).arg(QString(byteArray.toBase64())).arg(entries.at(i).fileName()).arg(mimeType.comment()).arg(size).arg(entries.at(i).lastModified().toString()));
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
