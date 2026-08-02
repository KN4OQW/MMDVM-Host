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

#include "TestFramework.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>

CTestRegistry& CTestRegistry::instance()
{
	static CTestRegistry registry;

	return registry;
}

CTestRegistry::CTestRegistry() :
m_tests(),
m_notes(),
m_iniFile("MMDVM-Host.ini"),
m_exclusive(false),
m_timeout(500U)
{
}

void CTestRegistry::add(const CTestCase& test)
{
	m_tests.push_back(test);
}

const std::vector<CTestCase>& CTestRegistry::tests() const
{
	return m_tests;
}

std::string CTestRegistry::iniFile() const
{
	return m_iniFile;
}

void CTestRegistry::setIniFile(const std::string& file)
{
	m_iniFile = file;
}

bool CTestRegistry::exclusive() const
{
	return m_exclusive;
}

void CTestRegistry::setExclusive(bool exclusive)
{
	m_exclusive = exclusive;
}

unsigned int CTestRegistry::timeout() const
{
	return m_timeout;
}

void CTestRegistry::setTimeout(unsigned int ms)
{
	m_timeout = ms;
}

void CTestRegistry::note(const std::string& text)
{
	m_notes.push_back(text);
}

const std::vector<std::string>& CTestRegistry::notes() const
{
	return m_notes;
}

void CTestRegistry::clearNotes()
{
	m_notes.clear();
}

CTestRegistrar::CTestRegistrar(const std::string& name, TEST_KIND kind, TESTFUNC func)
{
	CTestCase test;
	test.name = name;
	test.kind = kind;
	test.func = func;

	CTestRegistry::instance().add(test);
}

void TestFail(const std::string& message, const char* file, unsigned int line)
{
	CTestFailure failure;
	failure.message = message;
	failure.file    = file;
	failure.line    = line;

	throw failure;
}

void TestSkip(const std::string& reason)
{
	CTestSkip skip;
	skip.reason = reason;

	throw skip;
}

void TestNote(const std::string& text)
{
	CTestRegistry::instance().note(text);
}

std::string TestFormat(const char* fmt, ...)
{
	char buffer[1024U];

	va_list vl;
	va_start(vl, fmt);
	::vsnprintf(buffer, sizeof(buffer), fmt, vl);
	va_end(vl);

	return std::string(buffer);
}
