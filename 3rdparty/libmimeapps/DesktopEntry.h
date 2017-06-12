/*
libmimeapps, an implementation of
http://standards.freedesktop.org/mime-apps-spec/mime-apps-spec-latest.html

Copyright (c) 2015, Piotr Wójcik
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LIBMIMEAPPS_DESKTOPENTRY_H
#define LIBMIMEAPPS_DESKTOPENTRY_H

#include <string>
#include <vector>

namespace LibMimeApps
{

class DesktopEntry
{
public:
	enum class ParseOptions : unsigned int
	{
		Default = 0,
		NecessarilyUseUrl = (1 << 0)
	};

	DesktopEntry();
	explicit DesktopEntry(const std::string &baseDirectory, const std::string &relative, const std::string &language=std::string());
	bool execAllowMultipleUrl();
	bool execAllowRemoteUrl();
	static std::vector<std::string> parseExec(const std::string &executable, const std::vector<std::string> &urls=std::vector<std::string>(), ParseOptions options=ParseOptions::Default);
	std::vector<std::string> parseExec(const std::vector<std::string> &urls=std::vector<std::string>(), ParseOptions options=ParseOptions::Default);
	std::string name() const;
	std::string icon() const;
	std::string executable() const;
	std::string identifier() const;
	std::string path() const;
	std::vector<std::string> types() const;
	bool noDisplay() const;
	bool hidden() const;

private:
	std::string name_;
	std::string icon_;
	std::string executable_;
	std::string identifier_;
	std::string path_;
	std::vector<std::string> types_;
	bool noDisplay_;
	bool hidden_;
	bool allowMultiple_;
	bool allowRemote_;

	friend class Index;
};

bool isSet(const DesktopEntry::ParseOptions options, const DesktopEntry::ParseOptions searchedFlag);

}
#endif
