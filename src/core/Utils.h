#ifndef OTTER_UTILS_H
#define OTTER_UTILS_H

#include <QtCore/QString>

namespace Otter
{

namespace Utils
{

QString formatUnit(qint64 value, bool isSpeed = false, int precision = 1);

}

}

#endif
