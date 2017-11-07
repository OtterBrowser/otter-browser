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

#include "DesktopEntry.h"

#include <algorithm>

#include "ConfigReader.h"
#include "Tools.h"

namespace LibMimeApps
{

DesktopEntry::DesktopEntry():
	noDisplay_(false),
	hidden_(false),
	allowMultiple_(false),
	allowRemote_(false)
{
}

DesktopEntry::DesktopEntry(const std::string &baseDirectory, const std::string &relative, const std::string &language):
	identifier_(relative),
	path_(baseDirectory + relative),
	noDisplay_(false),
	hidden_(false),
	allowMultiple_(false),
	allowRemote_(false)
{

	ConfigReader config(path_);

	std::replace(identifier_.begin(), identifier_.end(), '/', '-');

	name_ = getLocaleValue(config, std::string("Desktop Entry"), std::string("Name"), language);
	icon_ = getLocaleValue(config, std::string("Desktop Entry"), std::string("Icon"), language);
	executable_ = config.value(std::string("Desktop Entry"), std::string("Exec"));
	types_ = split(config.value(std::string("Desktop Entry"), std::string("MimeType")), ';');
	noDisplay_ = (config.value(std::string("Desktop Entry"), std::string("NoDisplay")) == std::string("true"));
	hidden_ = (config.value(std::string("Desktop Entry"), std::string("Hidden")) == std::string("true"));
}

bool DesktopEntry::execAllowMultipleUrl()
{
	parseExec();

	return allowMultiple_;
}

bool DesktopEntry::execAllowRemoteUrl()
{
	parseExec();

	return allowRemote_;
}

std::vector<std::string> DesktopEntry::parseExec(const std::string &executable, const std::vector<std::string> &urls, ParseOptions options)
{
	DesktopEntry entry;
	entry.executable_ = executable;

	return entry.parseExec(urls, options);
}

std::vector<std::string> DesktopEntry::parseExec(const std::vector<std::string> &urls, ParseOptions options)
{
	std::vector<std::string> result;
	bool quoted = false;
	bool urlUsed = false;

	result.push_back(std::string());

	for (std::string::size_type i = 0; i < executable_.size();++i)
	{
		if (executable_.at(i) == ' ' && !quoted)
		{
			if (result.back().size() > 0)
			{
				result.push_back(std::string());
			}
		}
		else if (executable_.at(i) == '"')
		{
			quoted = !quoted;
		}
		else if (executable_.at(i) == '\\')
		{
			++i;
			result.back().push_back(executable_.at(i));
		}
		else if (executable_.at(i) == '%' && !quoted)
		{
			++i;
			switch (executable_.at(i))
			{
			case 'u':
				allowRemote_ = true;
				//@fallthrough@
			case 'f':
				if (urls.size() > 0)
				{
					result.back() += urls.front();
				}
				urlUsed = true;
				break;

			case 'U':
				allowRemote_ = true;
				//@fallthrough@
			case 'F':
				allowMultiple_ = true;

				if (urls.size() > 0)
				{
					result.back() += urls.front();
				}

				for (std::vector<std::string>::size_type i = 1; i < urls.size(); ++i)
				{
					result.push_back(urls.at(i));
				}
				urlUsed = true;
				break;

			case 'i':
				if (icon_.size() > 0)
				{
					result.push_back(std::string("--icon"));
					result.push_back(icon_);
				}
				break;

			case 'c':
				result.push_back(name_);
				break;

			case 'k':
				result.push_back(path_);
				break;

			case '%':
				result.back().push_back('%');
				break;

			default:
				break;
			}

		}
		else
		{
			result.back().push_back(executable_.at(i));
		}
	}

	if (!urlUsed && isSet(options, ParseOptions::NecessarilyUseUrl))
	{
		result.insert(result.end(), urls.begin(), urls.end());
	}

	return result;
}

std::string DesktopEntry::name() const
{
	return name_;
}

std::string DesktopEntry::icon() const
{
	return icon_;
}

std::string DesktopEntry::executable() const
{
	return executable_;
}

std::string DesktopEntry::identifier() const
{
	return identifier_;
}

std::string DesktopEntry::path() const
{
	return path_;
}

std::vector<std::string> DesktopEntry::types() const
{
	return types_;
}

bool DesktopEntry::noDisplay() const
{
	return noDisplay_;
}

bool DesktopEntry::hidden() const
{
	return hidden_;
}

bool isSet(const DesktopEntry::ParseOptions options, const DesktopEntry::ParseOptions searchedFlag)
{
	const unsigned int intOptions = static_cast<unsigned int>(options);
	const unsigned int intSearchedFlag = static_cast<unsigned int>(searchedFlag);

	return ((intOptions & intSearchedFlag) == intSearchedFlag);
}

}
