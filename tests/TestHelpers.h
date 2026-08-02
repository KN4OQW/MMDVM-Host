/*
 *   Copyright (C) 2025 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(TestHelpers_H)
#define	TestHelpers_H

#include "ConfValidator.h"
#include "IniBuilder.h"

#include <string>
#include <vector>

// Write the configuration out, read it back through CConf and validate it.
extern std::vector<CConfIssue> Validate(const CIniBuilder& ini);
extern std::vector<CConfIssue> Validate(const CIniBuilder& ini, const std::string& mode);

// A configuration with only the named mode, and its network, turned on.
extern CIniBuilder OnlyMode(const std::string& section);

// The [<mode>] section name for a mode, which is not always the mode name.
extern std::string SectionOf(const std::string& mode);
extern std::string NetworkSectionOf(const std::string& mode);

extern void AssertNoFailures(const std::vector<CConfIssue>& issues);
extern void AssertFailure(const std::vector<CConfIssue>& issues, const std::string& key);
extern void AssertWarning(const std::vector<CConfIssue>& issues, const std::string& key);
extern void AssertNoIssues(const std::vector<CConfIssue>& issues);

extern std::string Describe(const std::vector<CConfIssue>& issues);

#endif
