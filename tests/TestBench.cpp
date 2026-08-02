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

// The bench tests read the .ini file this host actually runs with and talk to
// the gateways on this machine. They are only run when --bench is given.
//
// A gateway probe never binds the mode's own local port, so it is safe to run
// these with the host running. The port tests do try the real port, and report
// a port the running host is holding as a skip unless --exclusive says the host
// has been stopped.

#include "TestFramework.h"
#include "GatewayProbe.h"

#include "ConfValidator.h"
#include "Conf.h"

#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace {

	// The .ini file is read once and shared by every bench test.
	CConf& benchConf()
	{
		static CConf* conf = nullptr;
		static bool   read = false;

		if (conf == nullptr) {
			conf = new CConf(CTestRegistry::instance().iniFile());
			read = conf->read();
		}

		if (!read)
			TestSkip("cannot read " + CTestRegistry::instance().iniFile() + ", pass --ini with the path to the host's .ini file");

		return *conf;
	}

	struct CGateway {
		bool                       networkEnabled;
		std::string                gatewayAddress;
		unsigned short             gatewayPort;
		std::string                localAddress;
		unsigned short             localPort;
		std::vector<unsigned char> poll;
	};

	// The poll or keepalive each mode's gateway expects to see. Where the
	// protocol has no such thing an empty datagram is sent instead, which every
	// gateway ignores but which is still enough for the system to tell us
	// whether anything is listening at all.
	std::vector<unsigned char> pollFrame(const std::string& mode, const CConf& conf)
	{
		std::vector<unsigned char> poll;

		if (mode == CONF_MODE_DSTAR) {
			// DSRP, poll with text, as CDStarNetwork sends.
			const std::string text = "MMDVM-Host";

			poll.push_back('D');
			poll.push_back('S');
			poll.push_back('R');
			poll.push_back('P');
			poll.push_back(0x0AU);

			for (char c : text)
				poll.push_back((unsigned char)c);

			poll.push_back(0x00U);
		} else if (mode == CONF_MODE_YSF) {
			// YSFP and the ten character callsign, as CYSFNetwork sends.
			std::string callsign = conf.getCallsign();
			callsign.resize(10U, ' ');

			poll.push_back('Y');
			poll.push_back('S');
			poll.push_back('F');
			poll.push_back('P');

			for (char c : callsign)
				poll.push_back((unsigned char)c);
		} else if (mode == CONF_MODE_M17) {
			poll.push_back('P');
			poll.push_back('I');
			poll.push_back('N');
			poll.push_back('G');
		} else if (mode == CONF_MODE_FM) {
			poll.push_back('F');
			poll.push_back('M');
			poll.push_back('P');
		}

		return poll;
	}

	CGateway gatewayFor(const std::string& mode, const CConf& conf)
	{
		CGateway gateway;
		gateway.networkEnabled = false;
		gateway.gatewayPort    = 0U;
		gateway.localPort      = 0U;

#if defined(USE_DSTAR)
		if (mode == CONF_MODE_DSTAR) {
			gateway.networkEnabled = conf.getDStarNetworkEnabled();
			gateway.gatewayAddress = conf.getDStarGatewayAddress();
			gateway.gatewayPort    = conf.getDStarGatewayPort();
			gateway.localAddress   = conf.getDStarLocalAddress();
			gateway.localPort      = conf.getDStarLocalPort();
		}
#endif
#if defined(USE_DMR)
		if (mode == CONF_MODE_DMR) {
			gateway.networkEnabled = conf.getDMRNetworkEnabled();
			gateway.gatewayAddress = conf.getDMRNetworkGatewayAddress();
			gateway.gatewayPort    = conf.getDMRNetworkGatewayPort();
			gateway.localAddress   = conf.getDMRNetworkLocalAddress();
			gateway.localPort      = conf.getDMRNetworkLocalPort();
		}
#endif
#if defined(USE_YSF)
		if (mode == CONF_MODE_YSF) {
			gateway.networkEnabled = conf.getFusionNetworkEnabled();
			gateway.gatewayAddress = conf.getFusionNetworkGatewayAddress();
			gateway.gatewayPort    = conf.getFusionNetworkGatewayPort();
			gateway.localAddress   = conf.getFusionNetworkLocalAddress();
			gateway.localPort      = conf.getFusionNetworkLocalPort();
		}
#endif
#if defined(USE_P25)
		if (mode == CONF_MODE_P25) {
			gateway.networkEnabled = conf.getP25NetworkEnabled();
			gateway.gatewayAddress = conf.getP25GatewayAddress();
			gateway.gatewayPort    = conf.getP25GatewayPort();
			gateway.localAddress   = conf.getP25LocalAddress();
			gateway.localPort      = conf.getP25LocalPort();
		}
#endif
#if defined(USE_NXDN)
		if (mode == CONF_MODE_NXDN) {
			gateway.networkEnabled = conf.getNXDNNetworkEnabled();
			gateway.gatewayAddress = conf.getNXDNGatewayAddress();
			gateway.gatewayPort    = conf.getNXDNGatewayPort();
			gateway.localAddress   = conf.getNXDNLocalAddress();
			gateway.localPort      = conf.getNXDNLocalPort();
		}
#endif
#if defined(USE_M17)
		if (mode == CONF_MODE_M17) {
			gateway.networkEnabled = conf.getM17NetworkEnabled();
			gateway.gatewayAddress = conf.getM17GatewayAddress();
			gateway.gatewayPort    = conf.getM17GatewayPort();
			gateway.localAddress   = conf.getM17LocalAddress();
			gateway.localPort      = conf.getM17LocalPort();
		}
#endif
#if defined(USE_POCSAG)
		if (mode == CONF_MODE_POCSAG) {
			gateway.networkEnabled = conf.getPOCSAGNetworkEnabled();
			gateway.gatewayAddress = conf.getPOCSAGGatewayAddress();
			gateway.gatewayPort    = conf.getPOCSAGGatewayPort();
			gateway.localAddress   = conf.getPOCSAGLocalAddress();
			gateway.localPort      = conf.getPOCSAGLocalPort();
		}
#endif
#if defined(USE_FM)
		if (mode == CONF_MODE_FM) {
			gateway.networkEnabled = conf.getFMNetworkEnabled();
			gateway.gatewayAddress = conf.getFMGatewayAddress();
			gateway.gatewayPort    = conf.getFMGatewayPort();
			gateway.localAddress   = conf.getFMLocalAddress();
			gateway.localPort      = conf.getFMLocalPort();
		}
#endif

		gateway.poll = pollFrame(mode, conf);

		return gateway;
	}

	// Everything the validator has to say about one mode, as it stands on this
	// machine. Warnings are reported but do not fail the test.
	void checkModeConfig(const std::string& mode)
	{
		CConf& conf = benchConf();

		CConfValidator validator(conf);

		if (!validator.isModeEnabled(mode))
			TestSkip(mode + " is not enabled in " + CTestRegistry::instance().iniFile());

		std::vector<CConfIssue> issues = validator.validate(mode);

		std::string failures;

		for (const auto& issue : issues) {
			TestNote(CConfValidator::format(issue));

			if (issue.severity == CONF_SEVERITY::FAILURE) {
				if (!failures.empty())
					failures += "; ";

				failures += issue.key;
			}
		}

		ASSERT_MSG(failures.empty(), mode + " will not work as configured: " + failures);
	}

	void checkGateway(const std::string& mode)
	{
		CConf& conf = benchConf();

		CConfValidator validator(conf);

		if (!validator.isModeEnabled(mode))
			TestSkip(mode + " is not enabled");

		CGateway gateway = gatewayFor(mode, conf);

		if (!gateway.networkEnabled)
			TestSkip("the " + mode + " network is not enabled");

		if (gateway.gatewayAddress.empty() || gateway.gatewayPort == 0U)
			TestFail("the " + mode + " network is enabled but has no gateway address or port", __FILE__, __LINE__);

		// A local port of zero, so that a running host keeps its own.
		CProbeReport report = ProbeGateway(gateway.gatewayAddress, gateway.gatewayPort, "", 0U,
						   gateway.poll.data(), (unsigned int)gateway.poll.size(),
						   CTestRegistry::instance().timeout());

		TestNote(mode + " gateway " + gateway.gatewayAddress + ":" + std::to_string(gateway.gatewayPort) +
			 " - " + ProbeResultName(report.result) + (report.detail.empty() ? "" : " (" + report.detail + ")"));

		switch (report.result) {
		case PROBE_RESULT::REPLIED:
			break;

		case PROBE_RESULT::NO_REPLY:
			// The gateway took the datagram without complaint. Several of
			// these protocols never answer a poll, so this is as far as the
			// check can go.
			if (!gateway.poll.empty())
				TestNote("the poll was accepted but not answered, which is normal for a gateway that only replies to traffic");
			break;

		case PROBE_RESULT::UNREACHABLE:
			TestFail("nothing is listening on the " + mode + " gateway port, so the gateway is not running or is on a different port", __FILE__, __LINE__);
			break;

		case PROBE_RESULT::RESOLVE_FAILED:
			TestFail("the " + mode + " gateway address will not resolve - " + report.detail, __FILE__, __LINE__);
			break;

		default:
			TestFail("the " + mode + " gateway could not be reached - " + report.detail, __FILE__, __LINE__);
			break;
		}
	}

	void checkLocalPort(const std::string& mode)
	{
		CConf& conf = benchConf();

		CConfValidator validator(conf);

		if (!validator.isModeEnabled(mode))
			TestSkip(mode + " is not enabled");

		CGateway gateway = gatewayFor(mode, conf);

		if (!gateway.networkEnabled)
			TestSkip("the " + mode + " network is not enabled");

		CBindReport report = ProbeLocalPort(gateway.localAddress, gateway.localPort);

		TestNote(mode + " local port - " + report.detail);

		switch (report.result) {
		case BIND_RESULT::FREE:
			break;

		case BIND_RESULT::IN_USE:
			if (CTestRegistry::instance().exclusive())
				TestFail("the " + mode + " local port is already in use and the host is meant to be stopped", __FILE__, __LINE__);
			else
				TestSkip("the local port is in use, which is expected while the host is running - pass --exclusive with the host stopped to check it properly");
			break;

		default:
			TestFail("the " + mode + " local port cannot be bound - " + report.detail, __FILE__, __LINE__);
			break;
		}
	}

}

BENCH_TEST("bench.conf.the_ini_file_can_be_read")
{
	benchConf();

	TestNote("read " + CTestRegistry::instance().iniFile());
}

BENCH_TEST("bench.conf.general")
{
	CConf& conf = benchConf();

	CConfValidator validator(conf);

	std::vector<CConfIssue> issues = validator.validate(CONF_MODE_GENERAL);

	std::string failures;

	for (const auto& issue : issues) {
		TestNote(CConfValidator::format(issue));

		if (issue.severity == CONF_SEVERITY::FAILURE) {
			if (!failures.empty())
				failures += "; ";

			failures += issue.key;
		}
	}

	ASSERT_MSG(failures.empty(), "the general configuration is not usable: " + failures);
}

BENCH_TEST("bench.conf.modem")
{
	CConf& conf = benchConf();

	CConfValidator validator(conf);

	std::vector<CConfIssue> issues = validator.validate(CONF_MODE_MODEM);

	std::string failures;

	for (const auto& issue : issues) {
		TestNote(CConfValidator::format(issue));

		if (issue.severity == CONF_SEVERITY::FAILURE) {
			if (!failures.empty())
				failures += "; ";

			failures += issue.key;
		}
	}

	ASSERT_MSG(failures.empty(), "the modem configuration is not usable: " + failures);
}

BENCH_TEST("bench.modem.the_hardware_port_is_present")
{
	CConf& conf = benchConf();

	std::string protocol = conf.getModemProtocol();

	std::string port;
	if (protocol == "uart")
		port = conf.getModemUARTPort();
	else if (protocol == "i2c")
		port = conf.getModemI2CPort();
	else
		TestSkip("the modem uses the " + protocol + " protocol, so there is no device to look for");

	if (port.empty())
		TestFail("the " + protocol + " protocol is selected but no port is set", __FILE__, __LINE__);

	struct stat sb;
	if (::stat(port.c_str(), &sb) != 0)
		TestFail("the modem port " + port + " does not exist", __FILE__, __LINE__);

	TestNote("found " + port);

	if (::access(port.c_str(), R_OK | W_OK) != 0)
		TestFail("the modem port " + port + " cannot be opened for reading and writing, check the dialout group membership", __FILE__, __LINE__);
}

// Per mode. Each mode gets its configuration checked, its gateway probed and
// its local port checked.

#if defined(USE_DSTAR)
BENCH_TEST("bench.conf.dstar")     { checkModeConfig(CONF_MODE_DSTAR); }
BENCH_TEST("bench.gateway.dstar")  { checkGateway(CONF_MODE_DSTAR); }
BENCH_TEST("bench.ports.dstar")    { checkLocalPort(CONF_MODE_DSTAR); }
#endif

#if defined(USE_DMR)
BENCH_TEST("bench.conf.dmr")       { checkModeConfig(CONF_MODE_DMR); }
BENCH_TEST("bench.gateway.dmr")    { checkGateway(CONF_MODE_DMR); }
BENCH_TEST("bench.ports.dmr")      { checkLocalPort(CONF_MODE_DMR); }
#endif

#if defined(USE_YSF)
BENCH_TEST("bench.conf.ysf")       { checkModeConfig(CONF_MODE_YSF); }
BENCH_TEST("bench.gateway.ysf")    { checkGateway(CONF_MODE_YSF); }
BENCH_TEST("bench.ports.ysf")      { checkLocalPort(CONF_MODE_YSF); }
#endif

#if defined(USE_P25)
BENCH_TEST("bench.conf.p25")       { checkModeConfig(CONF_MODE_P25); }
BENCH_TEST("bench.gateway.p25")    { checkGateway(CONF_MODE_P25); }
BENCH_TEST("bench.ports.p25")      { checkLocalPort(CONF_MODE_P25); }
#endif

#if defined(USE_NXDN)
BENCH_TEST("bench.conf.nxdn")      { checkModeConfig(CONF_MODE_NXDN); }
BENCH_TEST("bench.gateway.nxdn")   { checkGateway(CONF_MODE_NXDN); }
BENCH_TEST("bench.ports.nxdn")     { checkLocalPort(CONF_MODE_NXDN); }
#endif

#if defined(USE_M17)
BENCH_TEST("bench.conf.m17")       { checkModeConfig(CONF_MODE_M17); }
BENCH_TEST("bench.gateway.m17")    { checkGateway(CONF_MODE_M17); }
BENCH_TEST("bench.ports.m17")      { checkLocalPort(CONF_MODE_M17); }
#endif

#if defined(USE_POCSAG)
BENCH_TEST("bench.conf.pocsag")    { checkModeConfig(CONF_MODE_POCSAG); }
BENCH_TEST("bench.gateway.pocsag") { checkGateway(CONF_MODE_POCSAG); }
BENCH_TEST("bench.ports.pocsag")   { checkLocalPort(CONF_MODE_POCSAG); }
#endif

#if defined(USE_FM)
BENCH_TEST("bench.conf.fm")        { checkModeConfig(CONF_MODE_FM); }
BENCH_TEST("bench.gateway.fm")     { checkGateway(CONF_MODE_FM); }
BENCH_TEST("bench.ports.fm")       { checkLocalPort(CONF_MODE_FM); }
#endif
