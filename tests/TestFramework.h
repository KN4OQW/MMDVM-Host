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

#if !defined(TestFramework_H)
#define	TestFramework_H

#include <string>
#include <vector>

// A test is either hermetic - it touches nothing outside its own temporary
// files and can run anywhere - or it is a bench test, which reads the real
// configuration and talks to the real gateways on the machine it runs on.
enum class TEST_KIND {
	UNIT,
	BENCH
};

typedef void (*TESTFUNC)();

struct CTestFailure {
	std::string  message;
	std::string  file;
	unsigned int line;
};

struct CTestSkip {
	std::string reason;
};

struct CTestCase {
	std::string name;
	TEST_KIND   kind;
	TESTFUNC    func;
};

class CTestRegistry {
public:
	static CTestRegistry& instance();

	void add(const CTestCase& test);

	const std::vector<CTestCase>& tests() const;

	// Settings the runner fills in from the command line, read by the tests.
	std::string    iniFile() const;
	void           setIniFile(const std::string& file);
	bool           exclusive() const;
	void           setExclusive(bool exclusive);
	unsigned int   timeout() const;
	void           setTimeout(unsigned int ms);

	// Extra detail attached to the test that is running, printed after its
	// result line.
	void note(const std::string& text);
	const std::vector<std::string>& notes() const;
	void clearNotes();

private:
	CTestRegistry();

	std::vector<CTestCase>   m_tests;
	std::vector<std::string> m_notes;
	std::string              m_iniFile;
	bool                     m_exclusive;
	unsigned int             m_timeout;
};

class CTestRegistrar {
public:
	CTestRegistrar(const std::string& name, TEST_KIND kind, TESTFUNC func);
};

// Test definition. TEST() is hermetic, BENCH_TEST() needs the real radio,
// configuration or gateways and only runs when --bench is given. The name is a
// string so that it can be dotted, which is what --filter selects on.
#define	TEST_PASTE2(a, b)	a##b
#define	TEST_PASTE(a, b)	TEST_PASTE2(a, b)

#define	TEST_IMPL(name, kind, id)						\
	static void id();							\
	static CTestRegistrar TEST_PASTE(registrar_, id)(name, kind, id);	\
	static void id()

#define	TEST(name)		TEST_IMPL(name, TEST_KIND::UNIT,  TEST_PASTE(test_, __LINE__))
#define	BENCH_TEST(name)	TEST_IMPL(name, TEST_KIND::BENCH, TEST_PASTE(test_, __LINE__))

extern void TestFail(const std::string& message, const char* file, unsigned int line);
extern void TestSkip(const std::string& reason);
extern void TestNote(const std::string& text);
extern std::string TestFormat(const char* fmt, ...);

#define	ASSERT_TRUE(expr)							\
	do {									\
		if (!(expr))							\
			TestFail("expected true: " #expr, __FILE__, __LINE__);	\
	} while (false)

#define	ASSERT_FALSE(expr)							\
	do {									\
		if ((expr))							\
			TestFail("expected false: " #expr, __FILE__, __LINE__);	\
	} while (false)

#define	ASSERT_EQ(expected, actual)						\
	do {									\
		auto e_ = (expected);						\
		auto a_ = (actual);						\
		if (!(e_ == a_))						\
			TestFail(TestFormat("%s == %s, but got %s vs %s", #expected, #actual, \
					    std::to_string(e_).c_str(), std::to_string(a_).c_str()), \
				 __FILE__, __LINE__);				\
	} while (false)

#define	ASSERT_STREQ(expected, actual)						\
	do {									\
		std::string e_ = (expected);					\
		std::string a_ = (actual);					\
		if (e_ != a_)							\
			TestFail(TestFormat("%s == %s, but got \"%s\" vs \"%s\"", #expected, #actual, \
					    e_.c_str(), a_.c_str()),		\
				 __FILE__, __LINE__);				\
	} while (false)

#define	ASSERT_MSG(expr, message)						\
	do {									\
		if (!(expr))							\
			TestFail(message, __FILE__, __LINE__);			\
	} while (false)

#endif
