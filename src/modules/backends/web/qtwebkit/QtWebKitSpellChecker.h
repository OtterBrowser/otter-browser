/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2025 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef QTWEBKITSPELLCHECKER_H
#define QTWEBKITSPELLCHECKER_H

#include "qwebkitplatformplugin.h"
#include "../../../../../3rdparty/sonnet/src/core/speller.h"

namespace Otter
{

class QtWebKitSpellChecker final : public QWebSpellChecker
{
	Q_OBJECT

public:
	explicit QtWebKitSpellChecker();

	void toggleContinousSpellChecking() override;
	void toggleGrammarChecking() override;
	void checkSpellingOfString(const QString &word, int *misspellingLocation, int *misspellingLength) override;
	void checkGrammarOfString(const QString &word, QList<GrammarDetail> &detail, int *badGrammarLocation, int *badGrammarLength) override;
	void learnWord(const QString &word) override;
	void ignoreWordInSpellDocument(const QString &word) override;
	void guessesForWord(const QString &word, const QString &context, QStringList &guesses) override;
	QString autoCorrectSuggestionForMisspelledWord(const QString &word) override;
	static QStringList getSuggestions(const QString &word);
	bool isContinousSpellCheckingEnabled() const override;
	bool isGrammarCheckingEnabled() override;

protected:
	static bool isValidWord(const QString &string);

protected slots:
	void setDictionary(const QString &dictionary);

private:
	static Sonnet::Speller *m_speller;
};

}

#endif
