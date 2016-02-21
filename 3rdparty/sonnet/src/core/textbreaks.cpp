/*  This file is part of the KDE libraries
    Copyright (c) 2006 Jacob R Rideout <kde@jacobrideout.net>
    Copyright (c) 2006 Martin Sandsmark <martin.sandsmark@kde.org>
 
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
 
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
 
    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QDebug>

#include "textbreaks_p.h"


namespace Sonnet
{

class TextBreaksPrivate
{
public:
    TextBreaksPrivate()
    {}
    QString text;
};

TextBreaks::TextBreaks( const QString & text ) :
        d( new TextBreaksPrivate() )
{
    setText( text );
}

TextBreaks::~TextBreaks( )
{
    delete d;
}

QString TextBreaks::text() const
{
    return d->text;
}

void TextBreaks::setText( const QString & text )
{
    d->text = text;
}

static inline bool isWordSeparator(const QChar character)
{
    return character.isSpace() || character.isMark() || character.isPunct() || character.isSymbol();
}

TextBreaks::Positions TextBreaks::wordBreaks(const QString &text)
{
    Positions breaks;

    if (text.isEmpty()) {
        return breaks;
    }

    int i=0;
    do {
        if (i == 0 || isWordSeparator(text[i])) {
            Position pos;

            // Seek past leading separators
            while (i < text.length() && isWordSeparator(text[i])) {
                i++;
            }

            pos.start = i;
            while (i < text.length() && !isWordSeparator(text[i])) {
                i++;
            }
            pos.length = i - pos.start;

            if (pos.length > 0) {
                breaks.append(pos);
            }
        } else {
            i++;
        }
    } while (i < text.length());

    return breaks;
}

static inline bool isSentenceSeparator(const QChar &character)
{
    return character.isMark() || character.isPunct() || character.category() == QChar::Separator_Paragraph;
}

TextBreaks::Positions TextBreaks::sentenceBreaks(const QString & text)
{
    Positions breaks;

    if (text.isEmpty())
        return breaks;

    int i=0;
    do {
        if (i == 0 || isSentenceSeparator(text[i])) {
            Position pos;

            while (i < text.length() && !text[i].isLetter()) {
                i++;
            }

            pos.start = i;
            do {
                i++;
            } while (i < text.length() && !isSentenceSeparator(text[i]));
            pos.length = i - pos.start;

            // null-terminated, hence more than 1
            if (pos.length > 1)
                breaks.append(pos);
        } else {
            i++;
        }
    } while (i < text.length());

    return breaks;
}


TextBreaks::Positions TextBreaks::wordBreaks( ) const
{
    return wordBreaks(d->text);
}

TextBreaks::Positions TextBreaks::sentenceBreaks( ) const
{
    return sentenceBreaks(d->text);
}

}
