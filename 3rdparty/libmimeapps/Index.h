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

#ifndef LIBMIMEAPPS_INDEX_H
#define LIBMIMEAPPS_INDEX_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "DesktopEntry.h"
#include "Tools.h"

namespace LibMimeApps
{

class Index
{
public:
	Index();
	explicit Index(const std::string &language);
	~Index();

	std::vector<DesktopEntry> appsForMime(const std::string &type) const;

protected:
	struct lookupDirectory {
		bool withSubdirectories;
		std::string path;

		lookupDirectory(bool withSubdirectories, const std::string &path):
			withSubdirectories(withSubdirectories),
			path(path)
		{
		}
	};

	void findDirectories();
	void createBase();
	void processDirectory(const lookupDirectory &baseDirectory, const std::string &relative);
	void processDesktopInDirectory(const std::string &baseDirectory, const std::string &relative, const std::vector<file> &content);
	void processMimeApps(const std::string &path);
	void processDesktopFile(const std::string &baseDirectory, const std::string &relative);
	void addApplication(DesktopEntry *entry);
	void addToType(const std::string &type, DesktopEntry *entry);
	void removeApplication(const std::string &entryId);
	void removeFromType(const std::string &type, const std::string &entryId);
	void removeUnused();
	static std::list<std::string> resolveVariable(const std::string &name);
	static std::vector<lookupDirectory> initDirectoryPatterns();

private:
	static std::vector<lookupDirectory> directoryPatterns_;
	std::map<std::string, DesktopEntry*> knownApplications_;
	std::map<std::string, std::list<DesktopEntry*> > applicationsCache_;
	std::vector<lookupDirectory> directories_;
	std::string language_;
};

}

#endif
