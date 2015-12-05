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

#include "ConfigReader.h"

#include <fstream>

#include "Tools.h"

namespace LibMimeApps
{

ConfigReader::ConfigReader(const std::string &path)
{
	std::ifstream file(path.c_str());
	std::string line;
	std::string group;

	while (std::getline(file, line))
	{
		if (line.size() == 0 || line.at(0) == '#')
		{
			continue;
		}
		else if (line.at(0) == '[')
		{
			group = line.substr(1, line.find(']')-1);

			if (values_.find(group) == values_.end())
			{
				values_[group] = std::map<std::string, std::string>();
			}
		}
		else
		{
			size_t pos = line.find('=');

			if ((pos != std::string::npos) && (pos != (line.size()-1)))
			{
				std::string key(line.substr(0, pos));
				std::string value = unescape(line.substr(pos+1));

				values_[group][key] = value;
			}
		}
	}
}

std::vector<std::string> ConfigReader::groups() const
{
	std::vector<std::string> result;

	for (std::map< std::string, std::map<std::string, std::string> >::const_iterator it = values_.begin(); it != values_.end(); ++it)
	{
		result.push_back(it->first);
	}

	return result;
}

bool ConfigReader::hasGroup(const std::string &group) const
{
	return (values_.find(group) != values_.end());
}

std::vector<std::string> ConfigReader::keys(const std::string &group) const
{
	std::vector<std::string> result;

	if (!hasGroup(group))
	{
		return result;
	}

	for (std::map<std::string, std::string>::const_iterator it = values_.at(group).begin(); it != values_.at(group).end(); ++it)
	{
		result.push_back(it->first);
	}

	return result;
}

bool ConfigReader::hasKey(const std::string &group, const std::string &key) const
{
	return (hasGroup(group) && (values_.at(group).find(key) != values_.at(group).end()));
}

std::string ConfigReader::value(const std::string &group, const std::string &key) const
{
	if (!hasKey(group, key))
	{
		return std::string();
	}

	return values_.at(group).at(key);
}

std::string ConfigReader::unescape(const std::string &string)
{
	std::string result;

	for (std::string::size_type i = 0; i < string.length(); ++i)
	{
		if (string.at(i) == '\\')
		{
			++i;
			switch (string.at(i))
			{
			case 's':
				result.push_back(' ');
				break;

			case 'n':
				result.push_back('\n');
				break;

			case 't':
				result.push_back('\t');
				break;

			case 'r':
				result.push_back('\r');
				break;

			case '\\':
				result.push_back('\\');
				break;

			default:
				result.push_back('\\');
				result.push_back(string.at(i));
			}
		}
		else
		{
			result.push_back(string.at(i));
		}
	}

	return result;
}

}
