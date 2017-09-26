/**
 * highlighter.cpp
 *
 * Copyright (C)  2004  Zack Rusin <zack@kde.org>
 * Copyright (C)  2006  Laurent Montel <montel@kde.org>
 * Copyright (C)  2013  Martin Sandsmark <martin.sandsmark@org>
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

#include "highlighter.h"

#include "../core/speller.h"
#include "../core/loader_p.h"
#include "../core/tokenizer_p.h"
#include "../core/settings_p.h"

#include <QTextEdit>
#include <QTextCharFormat>
#include <QTimer>
#include <QColor>
#include <QHash>
#include <QTextCursor>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QMetaMethod>
#include <QPlainTextEdit>
#include <QDebug>

namespace Sonnet
{

class LanguageCache : public QTextBlockUserData {
public:
    QMap<QPair<int,int>, QString> languages;
    void invalidate(int pos) {
        QMutableMapIterator<QPair<int,int>, QString> it(languages);
        it.toBack();
        while (it.hasPrevious()) {
            it.previous();
            if (it.key().first+it.key().second >=pos) it.remove();
            else break;
        }
    }
};


class HighlighterPrivate
{
public:
    HighlighterPrivate(Highlighter *qq, const QColor &col)
        : textEdit(nullptr),
          plainTextEdit(nullptr),
          spellColor(col),
          q(qq)
    {
        tokenizer = new WordTokenizer();
        active = true;
        automatic = false;
        connected = false;
        wordCount = 0;
        errorCount = 0;
        intraWordEditing = false;
        completeRehighlightRequired = false;
        spellColor = spellColor.isValid() ? spellColor : Qt::red;

        loader = Loader::openLoader();
        loader->settings()->restore();

        spellchecker = new Sonnet::Speller();
        spellCheckerFound = spellchecker->isValid();
        rehighlightRequest = new QTimer(q);
        q->connect(rehighlightRequest, SIGNAL(timeout()),
                q, SLOT(slotRehighlight()));

        if (!spellCheckerFound) {
            return;
        }

        disablePercentage = loader->settings()->disablePercentageWordError();
        disableWordCount = loader->settings()->disableWordErrorCount();

        completeRehighlightRequired = true;
        rehighlightRequest->setInterval(0);
        rehighlightRequest->setSingleShot(true);
        rehighlightRequest->start();
    }

    ~HighlighterPrivate();
    WordTokenizer *tokenizer;
    Loader *loader;
    Speller *spellchecker;
    QTextEdit *textEdit;
    QPlainTextEdit *plainTextEdit;
    bool active;
    bool automatic;
    bool completeRehighlightRequired;
    bool intraWordEditing;
    bool spellCheckerFound; //cached d->dict->isValid() value
    bool connected;
    int disablePercentage;
    int disableWordCount;
    int wordCount, errorCount;
    QTimer *rehighlightRequest;
    QColor spellColor;
    Highlighter *q;
};

HighlighterPrivate::~HighlighterPrivate()
{
    delete spellchecker;
    delete tokenizer;
}

Highlighter::Highlighter(QTextEdit *edit,
                         const QColor &_col)
    : QSyntaxHighlighter(edit),
      d(new HighlighterPrivate(this, _col))
{
    d->textEdit = edit;
    d->textEdit->installEventFilter(this);
    d->textEdit->viewport()->installEventFilter(this);
}

Highlighter::Highlighter(QPlainTextEdit *edit, const QColor &col)
    : QSyntaxHighlighter(edit),
      d(new HighlighterPrivate(this, col))
{
    d->plainTextEdit = edit;
    setDocument(d->plainTextEdit->document());
    d->plainTextEdit->installEventFilter(this);
    d->plainTextEdit->viewport()->installEventFilter(this);
}

Highlighter::~Highlighter()
{
    delete d;
}

bool Highlighter::spellCheckerFound() const
{
    return d->spellCheckerFound;
}

void Highlighter::slotRehighlight()
{
    if (d->completeRehighlightRequired) {
        d->wordCount  = 0;
        d->errorCount = 0;
        rehighlight();

    } else {
        //rehighlight the current para only (undo/redo safe)
        QTextCursor cursor;
        if (d->textEdit)
            cursor = d->textEdit->textCursor();
        else
            cursor = d->plainTextEdit->textCursor();
        cursor.insertText(QString());
    }
    //if (d->checksDone == d->checksRequested)
    //d->completeRehighlightRequired = false;
    QTimer::singleShot(0, this, SLOT(slotAutoDetection()));
}

bool Highlighter::automatic() const
{
    return d->automatic;
}

bool Highlighter::intraWordEditing() const
{
    return d->intraWordEditing;
}

void Highlighter::setIntraWordEditing(bool editing)
{
    d->intraWordEditing = editing;
}

void Highlighter::setAutomatic(bool automatic)
{
    if (automatic == d->automatic) {
        return;
    }

    d->automatic = automatic;
    if (d->automatic) {
        slotAutoDetection();
    }
}

void Highlighter::slotAutoDetection()
{
    bool savedActive = d->active;

    //don't disable just because 1 of 4 is misspelled.
    if (d->automatic && d->wordCount >= 10) {
        // tme = Too many errors
        bool tme = (d->errorCount >= d->disableWordCount) && (
                       d->errorCount * 100 >= d->disablePercentage * d->wordCount);
        if (d->active && tme) {
            d->active = false;
        } else if (!d->active && !tme) {
            d->active = true;
        }
    }

    if (d->active != savedActive) {
        if (d->active) {
            emit activeChanged(tr("As-you-type spell checking enabled."));
        } else {
            qDebug() << "Sonnet: Disabling spell checking, too many errors";
            emit activeChanged(tr("Too many misspelled words. "
                                  "As-you-type spell checking disabled."));
        }

        d->completeRehighlightRequired = true;
        d->rehighlightRequest->setInterval(100);
        d->rehighlightRequest->setSingleShot(true);
    }
}

void Highlighter::setActive(bool active)
{
    if (active == d->active) {
        return;
    }
    d->active = active;
    rehighlight();

    if (d->active) {
        emit activeChanged(tr("As-you-type spell checking enabled."));
    } else {
        emit activeChanged(tr("As-you-type spell checking disabled."));
    }
}

bool Highlighter::isActive() const
{
    return d->active;
}

void Highlighter::contentsChange(int pos, int add, int rem)
{
    // Invalidate the cache where the text has changed
    const QTextBlock &lastBlock = document()->findBlock(pos + add - rem);
    QTextBlock block = document()->findBlock(pos);
    do {
        LanguageCache* cache=dynamic_cast<LanguageCache*>(block.userData());
        if (cache) cache->invalidate(pos-block.position());
        block = block.next();
    } while (block.isValid() && block < lastBlock);
}

void Highlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty() || !d->active || !d->spellCheckerFound) {
        return;
    }

    if (!d->connected) {
        connect(document(), SIGNAL(contentsChange(int,int,int)),
                SLOT(contentsChange(int,int,int)));
        d->connected = true;
    }
    QTextCursor cursor;
    if (d->textEdit) {
        cursor = d->textEdit->textCursor();
    } else {
        cursor = d->plainTextEdit->textCursor();
    }
    int index = cursor.position();

    const int lengthPosition = text.length() - 1;

    if ( index != lengthPosition ||
            ( lengthPosition > 0 && !text[lengthPosition-1].isLetter() ) ) {
        LanguageCache* cache=dynamic_cast<LanguageCache*>(currentBlockUserData());
        if (!cache) {
            cache = new LanguageCache;
            setCurrentBlockUserData(cache);
        }

        QStringRef sentence=&text;

        d->tokenizer->setBuffer(sentence.toString());
        int offset=sentence.position();
        while (d->tokenizer->hasNext()) {
            QStringRef word=d->tokenizer->next();
            if (!d->tokenizer->isSpellcheckable()) continue;
            ++d->wordCount;
            if (d->spellchecker->isMisspelled(word.toString())) {
                ++d->errorCount;
                setMisspelled(word.position()+offset, word.length());
            } else {
                unsetMisspelled(word.position()+offset, word.length());
            }
        }
    }
    //QTimer::singleShot( 0, this, SLOT(checkWords()) );
    setCurrentBlockState(0);
}

QString Highlighter::currentLanguage() const
{
    return d->spellchecker->language();
}

void Highlighter::setCurrentLanguage(const QString &lang)
{
    QString prevLang=d->spellchecker->language();
    d->spellchecker->setLanguage(lang);
    d->spellCheckerFound = d->spellchecker->isValid();
    if (!d->spellCheckerFound) {
        qDebug() << "No dictionary for \""
            << lang
            << "\" staying with the current language.";
        d->spellchecker->setLanguage(prevLang);
        return;
    }
    d->wordCount = 0;
    d->errorCount = 0;
    if (d->automatic) {
        d->rehighlightRequest->start(0);
    }
}

void Highlighter::setMisspelled(int start, int count)
{
    QTextCharFormat format;
    format.setFontUnderline(true);
    format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    format.setUnderlineColor(d->spellColor);
    setFormat(start, count, format);
}

void Highlighter::unsetMisspelled(int start, int count)
{
    setFormat(start, count, QTextCharFormat());
}

bool Highlighter::eventFilter(QObject *o, QEvent *e)
{
    if (!d->spellCheckerFound) {
        return false;
    }
    if ((o == d->textEdit || o == d->plainTextEdit)  && (e->type() == QEvent::KeyPress)) {
        QKeyEvent *k = static_cast<QKeyEvent *>(e);
        //d->autoReady = true;
        if (d->rehighlightRequest->isActive()) { // try to stay out of the users way
            d->rehighlightRequest->start(500);
        }
        if (k->key() == Qt::Key_Enter ||
                k->key() == Qt::Key_Return ||
                k->key() == Qt::Key_Up ||
                k->key() == Qt::Key_Down ||
                k->key() == Qt::Key_Left ||
                k->key() == Qt::Key_Right ||
                k->key() == Qt::Key_PageUp ||
                k->key() == Qt::Key_PageDown ||
                k->key() == Qt::Key_Home ||
                k->key() == Qt::Key_End ||
                ((k->modifiers() == Qt::ControlModifier) &&
                 ((k->key() == Qt::Key_A) ||
                  (k->key() == Qt::Key_B) ||
                  (k->key() == Qt::Key_E) ||
                  (k->key() == Qt::Key_N) ||
                  (k->key() == Qt::Key_P)))) {
            if (intraWordEditing()) {
                setIntraWordEditing(false);
                d->completeRehighlightRequired = true;
                d->rehighlightRequest->setInterval(500);
                d->rehighlightRequest->setSingleShot(true);
                d->rehighlightRequest->start();
            }
        } else {
            setIntraWordEditing(true);
        }
        if (k->key() == Qt::Key_Space ||
                k->key() == Qt::Key_Enter ||
                k->key() == Qt::Key_Return) {
            QTimer::singleShot(0, this, SLOT(slotAutoDetection()));
        }
    }

    else if ((( d->textEdit && ( o == d->textEdit->viewport())) || (d->plainTextEdit && (o == d->plainTextEdit->viewport()))) &&
             (e->type() == QEvent::MouseButtonPress)) {
        //d->autoReady = true;
        if (intraWordEditing()) {
            setIntraWordEditing(false);
            d->completeRehighlightRequired = true;
            d->rehighlightRequest->setInterval(0);
            d->rehighlightRequest->setSingleShot(true);
            d->rehighlightRequest->start();
        }
    }
    return false;
}

void Highlighter::addWordToDictionary(const QString &word)
{
    d->spellchecker->addToPersonal(word);
}

void Highlighter::ignoreWord(const QString &word)
{
    d->spellchecker->addToSession(word);
}

QStringList Highlighter::suggestionsForWord(const QString &word, int max)
{
    QStringList suggestions = d->spellchecker->suggest(word);
    if (max != -1) {
        while (suggestions.count() > max) {
            suggestions.removeLast();
        }
    }
    return suggestions;
}

bool Highlighter::isWordMisspelled(const QString &word)
{
    return d->spellchecker->isMisspelled(word);
}

void Highlighter::setMisspelledColor(const QColor &color)
{
    d->spellColor = color;
}

bool Highlighter::checkerEnabledByDefault() const
{
    return d->loader->settings()->checkerEnabledByDefault();
}

void Highlighter::setDocument(QTextDocument* document)
{
    d->connected = false;
    QSyntaxHighlighter::setDocument(document);
}

}
