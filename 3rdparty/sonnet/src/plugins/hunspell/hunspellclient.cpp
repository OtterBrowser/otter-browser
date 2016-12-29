/**
 * kspell_hunspellclient.cpp
 *
 * Copyright (C)  2009  Montel Laurent <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "hunspellclient.h"
#include "hunspelldict.h"
#include "hunspelldebug.h"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>

using namespace Sonnet;

HunspellClient::HunspellClient(QObject *parent)
    : Client(parent)
{
    qCDebug(SONNET_HUNSPELL) << " HunspellClient::HunspellClient";
    const QString AFF_MASK = QStringLiteral("*.aff");

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#ifdef Q_OS_MAC
    QDir lodir(QStringLiteral("/Applications/LibreOffice.app/Contents/Resources/extensions"));
#endif
#ifdef Q_OS_WIN
    QDir lodir(QStringLiteral("C:/Program Files (x86)/LibreOffice 5/share/extensions"));
#endif
    const QString DIR_MASK = QStringLiteral("dict-*");
    if (lodir.exists()) {
        foreach (const QString &d, lodir.entryList(QStringList(DIR_MASK), QDir::Dirs)) {
            QDir dictDir(lodir.absoluteFilePath(d));
            foreach (const QString &dict, dictDir.entryList(QStringList(AFF_MASK), QDir::Files)) {
                m_dictionaries[dict.left(dict.length() - 4)] = dictDir.absoluteFilePath(dict);
            }
        }
    }
#endif

    QStringList directories;

#ifdef Q_OS_MAC
    directories << QLatin1String("/System/Library/Spelling/");
#else
    directories << QLatin1String("/usr/share/hunspell/") << QLatin1String("/usr/local/share/hunspell/") << QLatin1String("/usr/share/myspell/") << QLatin1String("/usr/share/myspell/dicts/") << QLatin1String("/usr/local/share/mozilla-dicts/");
#endif

    const QString otterDirectory(qgetenv("OTTER_DICTIONARIES"));

    if (!otterDirectory.isEmpty()) {
        directories.append(otterDirectory);
    }

    directories.append(QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1String("dictionaries"));

    for (int i = 0; i < directories.count(); ++i) {
        QDir dir(directories.at(i));

        if (dir.exists()) {
            foreach (const QString &dict, dir.entryList(QStringList(AFF_MASK), QDir::Files)) {
                m_dictionaries[dict.left(dict.length() - 4)] = dir.absoluteFilePath(dict);
            }
        }
    }
}

HunspellClient::~HunspellClient()
{
}

SpellerPlugin *HunspellClient::createSpeller(const QString &language)
{
    qCDebug(SONNET_HUNSPELL) << " SpellerPlugin *HunspellClient::createSpeller(const QString &language) ;" << language;
    return new HunspellDict(m_dictionaries.value(language));
}

QStringList HunspellClient::languages() const
{
    return m_dictionaries.keys();
}

