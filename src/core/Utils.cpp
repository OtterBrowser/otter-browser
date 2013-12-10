#include "Utils.h"

#include <QtCore/QCoreApplication>

namespace Otter
{

namespace Utils
{

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

}

}
