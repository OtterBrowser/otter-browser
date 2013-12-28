#include "Utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTime>
#include <QtCore/QtMath>

namespace Otter
{

namespace Utils
{

QString formatTime(int value)
{
	QTime time(0, 0);
	time = time.addSecs(value);

	if (value > 3600)
	{
		QString string = time.toString(QLatin1String("hh:mm:ss"));

		if (value > 86400)
		{
			string = QCoreApplication::translate("utils", "%n days %1", "", (qFloor((qreal) value / 86400))).arg(string);
		}

		return string;
	}

	return time.toString(QLatin1String("mm:ss"));
}

QString formatUnit(qint64 value, bool isSpeed, int precision)
{
	if (value < 0)
	{
		return QString('?');
	}

	if (value > 1024)
	{
		if (value > 1048576)
		{
			if (value > 1073741824)
			{
				return QCoreApplication::translate("utils", (isSpeed ? "%1 GB/s" : "%1 GB")).arg((value / 1073741824.0), 0, 'f', precision);
			}

			return QCoreApplication::translate("utils", (isSpeed ? "%1 MB/s" : "%1 MB")).arg((value / 1048576.0), 0, 'f', precision);
		}

		return QCoreApplication::translate("utils", (isSpeed ? "%1 KB/s" : "%1 KB")).arg((value / 1024.0), 0, 'f', precision);
	}

	return QCoreApplication::translate("utils", (isSpeed ? "%1 B/s" : "%1 B")).arg(value);
}

QIcon getIcon(const QString &name, bool fromTheme)
{
	const QIcon icon(QString(":/icons/%1.png").arg(name));

	return (fromTheme ? QIcon::fromTheme(name, icon) : icon);
}

}

}
