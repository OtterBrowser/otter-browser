/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2012 by Lindsay Mathieson <lindsay dot mathieson at gmail dot com>
* Copyright (C) 2012 by Andrea Diamantini <adjam7 at gmail dot com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "QtWebKitSpellChecker.h"

#include <QtCore/QTextBoundaryFinder>

namespace Otter
{

QtWebKitSpellChecker::QtWebKitSpellChecker() : QWebSpellChecker(),
	m_speller(new Sonnet::Speller())
{
}

QtWebKitSpellChecker::~QtWebKitSpellChecker()
{
	delete m_speller;
}

void QtWebKitSpellChecker::toggleContinousSpellChecking()
{
}

void QtWebKitSpellChecker::toggleGrammarChecking()
{
}

void QtWebKitSpellChecker::checkSpellingOfString(const QString &word, int *misspellingLocation, int *misspellingLength)
{
	if (misspellingLocation == NULL || misspellingLength == NULL)
	{
		return;
	}

	*misspellingLocation = -1;
	*misspellingLength = 0;

	QTextBoundaryFinder finder(QTextBoundaryFinder::Word, word);
	QTextBoundaryFinder::BoundaryReasons boundary(finder.boundaryReasons());
	int start = finder.position();
	int end = finder.position();
	bool inWord = boundary.testFlag(QTextBoundaryFinder::StartOfItem);

	while (finder.toNextBoundary() > 0)
	{
		boundary = finder.boundaryReasons();

		if (boundary.testFlag(QTextBoundaryFinder::EndOfItem) && inWord)
		{
			end = finder.position();

			const QString string = finder.string().mid(start, (end - start));

			if (isValidWord(string))
			{
				if (m_speller->isMisspelled(string))
				{
					*misspellingLocation = start;
					*misspellingLength = (end - start);
				}

				return;
			}

			inWord = false;
		}

		if (boundary.testFlag(QTextBoundaryFinder::StartOfItem))
		{
			start = finder.position();
			inWord = true;
		}
	}
}

void QtWebKitSpellChecker::checkGrammarOfString(const QString &word, QList<QWebSpellChecker::GrammarDetail> &detail, int *badGrammarLocation, int *badGrammarLength)
{
	Q_UNUSED(word)
	Q_UNUSED(detail)
	Q_UNUSED(badGrammarLocation)
	Q_UNUSED(badGrammarLength)
}

void QtWebKitSpellChecker::learnWord(const QString &word)
{
	m_speller->addToPersonal(word);
}

void QtWebKitSpellChecker::ignoreWordInSpellDocument(const QString &word)
{
	m_speller->addToSession(word);
}

void QtWebKitSpellChecker::guessesForWord(const QString &word, const QString &context, QStringList &guesses)
{
	Q_UNUSED(context);

	guesses = m_speller->suggest(word);
}

QString QtWebKitSpellChecker::autoCorrectSuggestionForMisspelledWord(const QString &word)
{
	return QString();
}

bool QtWebKitSpellChecker::isContinousSpellCheckingEnabled() const
{
	return true;
}

bool QtWebKitSpellChecker::isGrammarCheckingEnabled()
{
	return false;
}

bool QtWebKitSpellChecker::isValidWord(const QString &string)
{
	if (string.isEmpty() || (string.length() == 1 && !string.at(0).isLetter()))
	{
		return false;
	}

	for (int i = 0; i < string.length(); ++i)
	{
		if (!string.at(i).isNumber())
		{
			return true;
		}
	}

	return false;
}

}
