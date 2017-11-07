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

#include "Index.h"

#include "ConfigReader.h"

namespace LibMimeApps
{

std::vector<Index::lookupDirectory>Index::directoryPatterns_ = Index::initDirectoryPatterns();

Index::Index()
{
	findDirectories();
	createBase();
}

Index::Index(const std::string &language):
	language_(language)
{
	findDirectories();
	createBase();
}

Index::~Index()
{
	std::map<std::string, DesktopEntry*>::iterator application;

	for (application = knownApplications_.begin(); application != knownApplications_.end(); ++application)
	{
		delete application->second;
	}
}

std::vector<DesktopEntry> Index::appsForMime(const std::string &type) const
{
	std::vector<DesktopEntry> result;

	if (applicationsCache_.find(type) != applicationsCache_.end())
	{
		std::list<DesktopEntry*> list = applicationsCache_.at(type);
		for (std::list<DesktopEntry*>::iterator entry = list.begin(); entry != list.end(); ++entry)
		{
			result.push_back(**entry);
		}
	}

	return result;
}

void Index::findDirectories()
{
	directories_ = directoryPatterns_;

	bool again;

	do
	{
		std::vector<lookupDirectory> directories;

		again = false;

		for (std::vector<lookupDirectory>::size_type i = 0; i < directories_.size(); ++i)
		{
			std::vector<std::string> unfolded = unfoldVariable(directories_.at(i).path);

			if (unfolded.size() > 0 && unfolded.at(0) != directories_.at(i).path)
			{
				again = true;
			}

			for (std::vector<std::string>::size_type j = 0; j < unfolded.size(); ++j)
			{
				directories.push_back(lookupDirectory(directories_.at(i).withSubdirectories, unfolded.at(j)));
			}
		}

		directories_.swap(directories);
		directories.clear();
	} while (again);
}

void Index::createBase()
{
	for (std::vector<std::string>::size_type i = 0; i < directories_.size(); ++i)
	{
		processDirectory(directories_.at(i), std::string());
	}

	removeUnused();
}

void Index::processDirectory(const lookupDirectory &baseDirectory, const std::string &relative)
{
	std::string directory = baseDirectory.path + relative;
	std::vector<file> content = directoryEntries(directory);

	if (baseDirectory.withSubdirectories)
	{
		for (std::vector<file>::size_type i = 0; i < content.size(); ++i)
		{
			if (content.at(i).type == FileType::Directory)
			{
				processDirectory(baseDirectory, relative + content.at(i).name + "/");
			}
		}
	}

	processDesktopInDirectory(baseDirectory.path, relative, content);
	processMimeApps(directory + "mimeapps.list");
}

void Index::processDesktopInDirectory(const std::string &baseDirectory, const std::string &relative, const std::vector<file> &content)
{
	for (std::vector<std::string>::size_type i = 0; i < content.size(); ++i)
	{
		if (content.at(i).type == FileType::File && endsWith(content.at(i).name, std::string(".desktop")))
		{
			processDesktopFile(baseDirectory, relative + content.at(i).name);
		}
	}
}

void Index::processMimeApps(const std::string &path)
{
	ConfigReader config(path);
	std::vector<std::string> types;

	types = config.keys("Added Associations");

	for (std::vector<std::string>::size_type i = 0; i < types.size(); ++i)
	{
		std::vector<std::string> identifiers = split(config.value("Added Associations", types.at(i)), ';');

		for (std::vector<std::string>::reverse_iterator it = identifiers.rbegin(); it != identifiers.rend(); ++it)
		{
			if (knownApplications_.find(*it) != knownApplications_.end())
			{
				addToType(types[i], knownApplications_.at(*it));
			}
		}
	}

	types = config.keys("Removed Associations");

	for (std::vector<std::string>::size_type i = 0; i < types.size(); ++i)
	{
		std::vector<std::string> identifiers = split(config.value("Removed Associations", types.at(i)), ';');

		for (std::vector<std::string>::size_type j = 0; j < identifiers.size(); ++j)
		{
			removeFromType(types[i], identifiers[j]);
		}
	}
}

void Index::processDesktopFile(const std::string &baseDirectory, const std::string &relative)
{
	DesktopEntry *entry = new DesktopEntry(baseDirectory, relative, language_);

	if (entry->hidden())
	{
		removeApplication(entry->identifier());
	}
	else if (!entry->noDisplay())
	{
		addApplication(entry);
	}
	else
	{
		delete entry;
	}
}

void Index::addApplication(DesktopEntry *entry)
{
	removeApplication(entry->identifier());

	knownApplications_[entry->identifier()] = entry;

	for (std::vector<std::string>::size_type i = 0; i < entry->types().size(); ++i)
	{
		addToType(entry->types().at(i), entry);
	}
}

void Index::addToType(const std::string &type, DesktopEntry *entry)
{
	if (applicationsCache_.find(type) != applicationsCache_.end())
	{
		removeFromType(type, entry->identifier());
	}

	applicationsCache_[type].push_front(entry);
	entry->types_.push_back(type);
}

void Index::removeApplication(const std::string &entryId)
{
	for (std::map<std::string, std::list<DesktopEntry*> >::iterator type = applicationsCache_.begin(); type != applicationsCache_.end(); ++type)
	{
		removeFromType(type->first, entryId);
	}
}

void Index::removeFromType(const std::string &type, const std::string &entryId)
{
	if (applicationsCache_.find(type) != applicationsCache_.end())
	{
		for (std::list<DesktopEntry*>::iterator it = applicationsCache_.at(type).begin(); it != applicationsCache_.at(type).end();)
		{
			if ((*it)->identifier() == entryId)
			{
				applicationsCache_.at(type).erase(it++);
			}
			else
			{
				++it;
			}
		}
	}

	if (knownApplications_.find(entryId) != knownApplications_.end())
	{
		std::vector<std::string> &types = knownApplications_.at(entryId)->types_;

		for (std::vector<std::string>::iterator it = types.begin(); it != types.end();)
		{
			if (*it == type)
			{
				if (it == types.begin())
				{
					types.erase(it);

					it = types.begin();
				}
				else
				{
					std::vector<std::string>::iterator temp = it;
					--temp;

					types.erase(it);

					it = ++temp;
				}

			}
			else
			{
				++it;
			}
		}
	}
}

void Index::removeUnused()
{
	std::map<std::string, DesktopEntry*>::iterator application;

	for (application = knownApplications_.begin(); application != knownApplications_.end();)
	{
		if (application->second->types().empty())
		{
			delete application->second;

			knownApplications_.erase(application++);
		}
		else
		{
			++application;
		}
	}
}

std::vector<Index::lookupDirectory> Index::initDirectoryPatterns()
{
	std::vector<lookupDirectory> result;

	result.push_back(lookupDirectory(true, "$XDG_DATA_DIRS/applications/"));
	result.push_back(lookupDirectory(true, "$XDG_DATA_HOME/applications/"));
	result.push_back(lookupDirectory(false, "$XDG_CONFIG_DIRS/"));
	result.push_back(lookupDirectory(false, "$XDG_CONFIG_HOME/"));

	return result;
}

}
