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

#include "TestHelpers.h"
#include "TestFramework.h"

#include "Conf.h"

std::vector<CConfIssue> Validate(const CIniBuilder& ini)
{
	std::string file = ini.write();

	CConf conf(file);
	if (!conf.read())
		TestFail("the generated .ini file " + file + " could not be read back", __FILE__, __LINE__);

	CConfValidator validator(conf);

	return validator.validate();
}

std::vector<CConfIssue> Validate(const CIniBuilder& ini, const std::string& mode)
{
	std::string file = ini.write();

	CConf conf(file);
	if (!conf.read())
		TestFail("the generated .ini file " + file + " could not be read back", __FILE__, __LINE__);

	CConfValidator validator(conf);

	return validator.validate(mode);
}

std::string SectionOf(const std::string& mode)
{
	// The System Fusion section does not carry the name the rest of the host
	// uses for the mode.
	if (mode == CONF_MODE_YSF)
		return "System Fusion";

	return mode;
}

std::string NetworkSectionOf(const std::string& mode)
{
	return SectionOf(mode) + " Network";
}

CIniBuilder OnlyMode(const std::string& mode)
{
	CIniBuilder ini;
	ini.disableAllModes();
	ini.set(SectionOf(mode), "Enable", "1");
	ini.set(NetworkSectionOf(mode), "Enable", "1");

	return ini;
}

std::string Describe(const std::vector<CConfIssue>& issues)
{
	if (issues.empty())
		return "no issues";

	std::string text;

	for (const auto& issue : issues) {
		if (!text.empty())
			text += "; ";

		text += CConfValidator::format(issue);
	}

	return text;
}

void AssertNoFailures(const std::vector<CConfIssue>& issues)
{
	if (CConfValidator::count(issues, CONF_SEVERITY::FAILURE) > 0U)
		TestFail("expected no failures, but got " + Describe(issues), __FILE__, __LINE__);
}

void AssertNoIssues(const std::vector<CConfIssue>& issues)
{
	if (!issues.empty())
		TestFail("expected a clean configuration, but got " + Describe(issues), __FILE__, __LINE__);
}

void AssertFailure(const std::vector<CConfIssue>& issues, const std::string& key)
{
	if (!CConfValidator::has(issues, CONF_SEVERITY::FAILURE, key))
		TestFail("expected a failure for " + key + ", but got " + Describe(issues), __FILE__, __LINE__);
}

void AssertWarning(const std::vector<CConfIssue>& issues, const std::string& key)
{
	if (!CConfValidator::has(issues, CONF_SEVERITY::WARNING, key))
		TestFail("expected a warning for " + key + ", but got " + Describe(issues), __FILE__, __LINE__);
}
