#ifndef OTTER_UTILS_H
#define OTTER_UTILS_H

#include <QtCore/QString>
#include <QtGui/QIcon>

namespace Otter
{

namespace Utils
{

QString formatTime(int value);
QString formatUnit(qint64 value, bool isSpeed = false, int precision = 1);
QIcon getIcon(const QLatin1String &name, bool fromTheme = true);

}

}

#endif
