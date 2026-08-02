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

#if !defined(IniBuilder_H)
#define	IniBuilder_H

#include <string>
#include <utility>
#include <vector>

// Builds a .ini file for a test to read. The starting point is a
// configuration with every mode and every network enabled and every value
// valid, so a test only has to describe the one thing it wants to be wrong.
class CIniBuilder {
public:
	// A complete, valid configuration.
	CIniBuilder();

	// Nothing at all, for testing what the defaults do.
	static CIniBuilder empty();

	// Add the key, or replace it if the section already has it.
	CIniBuilder& set(const std::string& section, const std::string& key, const std::string& value);
	CIniBuilder& set(const std::string& section, const std::string& key, unsigned int value);

	// Take the key out, which leaves CConf holding its built in default. This
	// is how an operator forgetting a line is modelled - writing "Key=" with
	// nothing after it does not reach CConf at all.
	CIniBuilder& remove(const std::string& section, const std::string& key);
	CIniBuilder& removeSection(const std::string& section);

	// Turn a mode and its network off, used to isolate one mode at a time.
	CIniBuilder& disableAllModes();

	std::string text() const;

	// Write to a temporary file and return its path. The file is removed when
	// the test run finishes.
	std::string write() const;

private:
	typedef std::pair<std::string, std::string>           KEYVALUE;
	typedef std::pair<std::string, std::vector<KEYVALUE>> SECTION;

	std::vector<SECTION> m_sections;

	std::vector<KEYVALUE>* find(const std::string& section);
};

#endif
