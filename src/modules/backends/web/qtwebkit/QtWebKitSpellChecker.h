/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#ifndef QTWEBKITSPELLCHECKER_H
#define QTWEBKITSPELLCHECKER_H

#include "qwebkitplatformplugin.h"
#include "../../../../../3rdparty/sonnet/src/core/speller.h"

namespace Otter
{

class QtWebKitSpellChecker : public QWebSpellChecker
{
	Q_OBJECT

public:
	explicit QtWebKitSpellChecker();
	~QtWebKitSpellChecker();

	void toggleContinousSpellChecking();
	void toggleGrammarChecking();
	void checkSpellingOfString(const QString &word, int *misspellingLocation, int *misspellingLength);
	void checkGrammarOfString(const QString &word, QList<GrammarDetail> &detail, int *badGrammarLocation, int *badGrammarLength);
	void learnWord(const QString &word);
	void ignoreWordInSpellDocument(const QString &word);
	void guessesForWord(const QString &word, const QString &context, QStringList &guesses);
	QString autoCorrectSuggestionForMisspelledWord(const QString &word);
	bool isContinousSpellCheckingEnabled() const;
	bool isGrammarCheckingEnabled();

protected:
	static bool isValidWord(const QString &string);

protected slots:
	void setDictionary(const QString &dictionary);

private:
	Sonnet::Speller *m_speller;
};

}

#endif
