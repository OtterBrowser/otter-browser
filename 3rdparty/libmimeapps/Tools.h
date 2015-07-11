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

#ifndef LIBMIMEAPPS_TOOLS_H
#define LIBMIMEAPPS_TOOLS_H

#include <string>
#include <vector>

#include "ConfigReader.h"

namespace LibMimeApps
{

namespace FileType
{

enum FileType {
	Directory,
	File
};

}

struct lang {
	std::string language;
	std::string country;
	std::string modifier;

	lang(){}
	explicit lang(const std::string &string);
};

struct file {
	std::string name;
	FileType::FileType type;

	file(const std::string &name, const FileType::FileType type):
		name(name),
		type(type)
	{
	}
};

bool startsWith(const std::string &str, const std::string &prefix);
bool endsWith(std::string const &str, std::string const &suffix);
std::vector<std::string> split(const std::string &str, const char delimiter);
bool match(std::string const& text, std::string const& pattern);
std::vector<file> directoryEntries(const std::string &directory);
std::vector<std::string> unfoldVariable(const std::string &string);
std::vector<std::string> getVariableValues(const std::string &name);
std::string getLocaleValue(const ConfigReader &config, const std::string &group, const std::string &key, const std::string &language);
std::string alnums(const std::string &string, size_t begin);

}

#endif
