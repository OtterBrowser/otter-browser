#include "CookieJar.h"
#include "SettingsManager.h"

namespace Otter
{

CookieJar::CookieJar(QObject *parent) : QNetworkCookieJar(parent)
{
}

}
