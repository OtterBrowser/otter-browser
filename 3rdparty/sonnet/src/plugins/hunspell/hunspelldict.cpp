/**
 * kspell_hunspelldict.cpp
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
#include "hunspelldict.h"
#include "hunspelldebug.h"

#include <QFileInfo>
#include <QDebug>
#include <QTextCodec>
#include <QStringBuilder>

using namespace Sonnet;

HunspellDict::HunspellDict(const QString &lang)
    : SpellerPlugin(QFileInfo(lang).baseName())
    , m_speller(0)
    , m_codec(0)
{
    qCDebug(SONNET_HUNSPELL) << " HunspellDict::HunspellDict( const QString& lang ):" << lang;

    if (QFileInfo(lang).exists()) {
        m_speller = new Hunspell(lang.toLatin1().constData(), (lang.left(lang.length() - 4) + QLatin1String(".dic")).toLatin1().constData());
        m_codec = QTextCodec::codecForName(m_speller->get_dic_encoding());
        if (!m_codec) {
            qWarning() << "Failed to find a text codec for name" << m_speller->get_dic_encoding() << "defaulting to locale text codec";
            m_codec = QTextCodec::codecForLocale();
            Q_ASSERT(m_codec);
        }
    }
    qCDebug(SONNET_HUNSPELL) << " dddddd " << m_speller;
}

HunspellDict::~HunspellDict()
{
    delete m_speller;
}

QByteArray HunspellDict::toDictEncoding(const QString& word) const
{
    return m_codec->fromUnicode(word);
}

bool HunspellDict::isCorrect(const QString &word) const
{
    qCDebug(SONNET_HUNSPELL) << " isCorrect :" << word;
    if (!m_speller) {
        return false;
    }
    const bool result = m_speller->spell(toDictEncoding(word).toStdString());
    qCDebug(SONNET_HUNSPELL) << " result :" << result;
    return result;
}

QStringList HunspellDict::suggest(const QString &word) const
{
    if (!m_speller) {
        return QStringList();
    }
    QStringList lst;
    std::vector<std::string> suggestions = m_speller->suggest(toDictEncoding(word).toStdString());
    for (uint i = 0; i < suggestions.size(); ++i) {
        lst << m_codec->toUnicode(QByteArray::fromStdString(suggestions[i]));
    }
    return lst;
}

bool HunspellDict::storeReplacement(const QString &bad,
                                    const QString &good)
{
    Q_UNUSED(bad);
    Q_UNUSED(good);
    if (!m_speller) {
        return false;
    }
    qCDebug(SONNET_HUNSPELL) << "HunspellDict::storeReplacement not implemented";
    return false;
}

bool HunspellDict::addToPersonal(const QString &word)
{
    if (!m_speller) {
        return false;
    }
    m_speller->add(toDictEncoding(word).constData());
    return false;
}

bool HunspellDict::addToSession(const QString &word)
{
    Q_UNUSED(word);
    if (!m_speller) {
        return false;
    }
    qCDebug(SONNET_HUNSPELL) << " bool HunspellDict::addToSession not implemented";
    return false;
}
