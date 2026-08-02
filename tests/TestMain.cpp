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

#include "Log.h"
#include "Version.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

	void usage(const char* name)
	{
		::fprintf(stdout,
			  "Usage: %s [options]\n"
			  "\n"
			  "  --list              list the tests and do not run them\n"
			  "  --filter <text>     only run tests whose name contains <text>\n"
			  "  --bench             also run the bench tests, which read the real .ini\n"
			  "                      file and talk to the gateways on this machine\n"
			  "  --ini <file>        the .ini file the bench tests read (default MMDVM-Host.ini)\n"
			  "  --exclusive         the host is stopped, so a network port that is already\n"
			  "                      bound is a failure rather than something to skip\n"
			  "  --timeout <ms>      how long to wait for a gateway to answer (default 500)\n"
			  "  --verbose           let the host's own logging through to the console\n"
			  "  --help              this text\n",
			  name);
	}

}

int main(int argc, char** argv)
{
	std::string filter;
	bool bench   = false;
	bool list    = false;
	bool verbose = false;

	CTestRegistry& registry = CTestRegistry::instance();

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "--help" || arg == "-h") {
			usage(argv[0]);
			return 0;
		} else if (arg == "--list") {
			list = true;
		} else if (arg == "--bench") {
			bench = true;
		} else if (arg == "--exclusive") {
			registry.setExclusive(true);
		} else if (arg == "--verbose") {
			verbose = true;
		} else if (arg == "--filter" && (i + 1) < argc) {
			filter = argv[++i];
		} else if (arg == "--ini" && (i + 1) < argc) {
			registry.setIniFile(argv[++i]);
		} else if (arg == "--timeout" && (i + 1) < argc) {
			registry.setTimeout((unsigned int)::atoi(argv[++i]));
		} else {
			::fprintf(stderr, "Unknown option '%s'\n\n", arg.c_str());
			usage(argv[0]);
			return 2;
		}
	}

	// A display level of zero keeps the host's own logging quiet, and no MQTT
	// connection is ever made, so nothing is published.
	::LogInitialise(verbose ? 1U : 0U, 0U);

	const std::vector<CTestCase>& tests = registry.tests();

	if (list) {
		for (const auto& test : tests) {
			if (!filter.empty() && test.name.find(filter) == std::string::npos)
				continue;

			::fprintf(stdout, "%-52s %s\n", test.name.c_str(), test.kind == TEST_KIND::BENCH ? "(bench)" : "");
		}

		return 0;
	}

	::fprintf(stdout, "MMDVM-Host tests, version %s\n", VERSION);
	if (bench)
		::fprintf(stdout, "Bench tests are enabled, reading %s\n", registry.iniFile().c_str());
	::fprintf(stdout, "\n");

	unsigned int passed  = 0U;
	unsigned int failed  = 0U;
	unsigned int skipped = 0U;

	std::vector<std::string> failures;

	for (const auto& test : tests) {
		if (!filter.empty() && test.name.find(filter) == std::string::npos)
			continue;

		if (test.kind == TEST_KIND::BENCH && !bench)
			continue;

		registry.clearNotes();

		std::string result;
		std::string detail;

		try {
			test.func();
			result = "PASS";
			passed++;
		} catch (const CTestFailure& failure) {
			result = "FAIL";
			detail = failure.message + " (" + failure.file + ":" + std::to_string(failure.line) + ")";
			failures.push_back(test.name + ": " + detail);
			failed++;
		} catch (const CTestSkip& skip) {
			result = "SKIP";
			detail = skip.reason;
			skipped++;
		} catch (const std::exception& e) {
			result = "FAIL";
			detail = std::string("unexpected exception - ") + e.what();
			failures.push_back(test.name + ": " + detail);
			failed++;
		}

		::fprintf(stdout, "%-4s %s\n", result.c_str(), test.name.c_str());

		for (const auto& note : registry.notes())
			::fprintf(stdout, "       %s\n", note.c_str());

		if (!detail.empty())
			::fprintf(stdout, "       %s\n", detail.c_str());

		::fflush(stdout);
	}

	::fprintf(stdout, "\n%u passed, %u failed, %u skipped\n", passed, failed, skipped);

	if (!failures.empty()) {
		::fprintf(stdout, "\nFailures:\n");
		for (const auto& failure : failures)
			::fprintf(stdout, "  %s\n", failure.c_str());
	}

	return failed > 0U ? 1 : 0;
}
