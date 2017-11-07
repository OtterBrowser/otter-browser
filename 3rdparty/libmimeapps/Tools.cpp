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

#include "Tools.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>

namespace LibMimeApps
{

bool startsWith(const std::string &str, const std::string &prefix)
{
	if (str.size() < prefix.size())
	{
		return false;
	}

	return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool endsWith(std::string const &str, std::string const &suffix)
{
	if (str.size() < suffix.size())
	{
		return false;
	}

	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
	std::vector<std::string> chunks;
	std::stringstream stream(str);
	std::string item;

	while (std::getline(stream, item, delimiter))
	{
		chunks.push_back(item);
	}

	return chunks;
}

bool match(std::string const& text, std::string const& pattern)
{
	return std::search(text.begin(), text.end(), pattern.begin(), pattern.end()) != text.end();
}

std::vector<file> directoryEntries(const std::string &directory)
{
	std::vector<file> result;
	DIR *stream = opendir(directory.c_str());

	if (stream == NULL)
	{
		return result;
	}

	struct dirent *entry;
	struct stat info;
	std::string name;
	std::string path;

	while ((entry = readdir(stream)) != NULL)
	{
		name = entry->d_name;
		path = directory + name;

		stat(path.c_str(), &info);

		if (name.size() > 0 && name.at(0) != '.')
		{
			if (S_ISREG(info.st_mode))
			{
				result.push_back(file(name, FileType::File));
			}
			else if (S_ISDIR(info.st_mode))
			{
				result.push_back(file(name, FileType::Directory));
			}
		}
	}

	closedir(stream);

	return result;
}

std::vector<std::string> unfoldVariable(const std::string &string)
{
	std::vector<std::string> result;
	size_t begin = string.find('$');

	if (begin == std::string::npos)
	{
		result.push_back(string);

		return result;
	}

	size_t end = begin+1;

	while (end < string.size() && (std::isalnum(string.at(end)) || string.at(end) == '_'))
	{
		++end;
	}

	std::string name = string.substr(begin+1, end-(begin+1));

	std::vector<std::string> values = getVariableValues(name);

	for (std::vector<std::string>::iterator value = values.begin(); value != values.end(); ++value)
	{
		result.push_back(string.substr(0, begin) + (*value) + string.substr(end));
	}

	return result;
}

std::vector<std::string> getVariableValues(const std::string &name)
{
	std::string value;
	char *env = std::getenv(name.c_str());

	if (env && std::strlen(env) > 0)
	{
		value = std::string(env);
	}
	else
	{
		std::map<std::string, std::string> defaultVariableValues;
		defaultVariableValues["XDG_DATA_HOME"] = std::string("$HOME/.local/share");
		defaultVariableValues["XDG_CONFIG_HOME"] = std::string("$HOME/.config");
		defaultVariableValues["XDG_DATA_DIRS"] = std::string("/usr/local/share/:/usr/share/");
		defaultVariableValues["XDG_CONFIG_DIRS"] = std::string("/etc/xdg");

		if (defaultVariableValues.find(name) != defaultVariableValues.end())
		{
			value = defaultVariableValues.at(name);
		}
	}

	return split(value, ':');
}

std::string getLocaleValue(const ConfigReader &config, const std::string &group, const std::string &key, const std::string &language)
{
	lang wanted(language);
	std::vector<std::string> localeKeys;

	if (wanted.country.size() > 0 && wanted.modifier.size() > 0)
	{
		localeKeys.push_back(wanted.language + "_" + wanted.country + "@" + wanted.modifier);
	}

	if (wanted.country.size() > 0)
	{
		localeKeys.push_back(wanted.language + "_" + wanted.country);
	}

	if (wanted.modifier.size() > 0)
	{
		localeKeys.push_back(wanted.language + "@" + wanted.modifier);
	}

	localeKeys.push_back(wanted.language);

	for (std::vector<std::string>::size_type i = 0; i < localeKeys.size(); ++i)
	{
		std::string localeKey = key + "[" + localeKeys[i] + "]";

		if (config.hasKey(group, localeKey))
		{
			return config.value(group, localeKey);
		}
	}

	return config.value(group, key);
}

std::string alnums(const std::string &string, size_t begin)
{
	if (begin == std::string::npos || begin >= string.size())
	{
		return std::string();
	}

	size_t end = begin;

	while (end < string.size() && std::isalnum(string.at(end)))
	{
		++end;
	}

	return string.substr(begin, end-begin);
}

lang::lang(const std::string &string):
	language(alnums(string, 0))
{
	if (match(string, "_"))
	{
		country = alnums(string, string.find('_')+1);
	}

	if (match(string, "@"))
	{
		modifier = alnums(string, string.find('@')+1);
	}
}

}
