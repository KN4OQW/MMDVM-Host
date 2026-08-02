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
#include "TestHelpers.h"

static const char* const BLANK = "\"\"";

// Every mode goes through the same three sets of checks, so they live here and
// each mode's tests call them. A mode that does not need one of them simply
// does not call it.

// The mode on its own, with everything it needs, must be clean.
static void checkCleanAlone(const std::string& mode)
{
	CIniBuilder ini = OnlyMode(mode);

	AssertNoIssues(Validate(ini, mode));
}

// A mode is useless without somewhere to transmit and receive.
static void checkNeedsFrequencies(const std::string& mode)
{
	CIniBuilder ini = OnlyMode(mode);
	ini.remove("Modem", "RXFrequency");
	ini.remove("Modem", "TXFrequency");

	std::vector<CConfIssue> issues = Validate(ini, CONF_MODE_GENERAL);

	AssertFailure(issues, "[Modem] RXFrequency");
	AssertFailure(issues, "[Modem] TXFrequency");
}

// Everything the host needs before it will open a gateway connection.
static void checkNetworkRules(const std::string& mode)
{
	std::string section = NetworkSectionOf(mode);

	{
		CIniBuilder ini = OnlyMode(mode);
		ini.remove(section, "GatewayAddress");

		AssertFailure(Validate(ini, mode), "[" + section + "] GatewayAddress");
	}

	{
		CIniBuilder ini = OnlyMode(mode);
		ini.remove(section, "GatewayPort");

		AssertFailure(Validate(ini, mode), "[" + section + "] GatewayPort");
	}

	{
		CIniBuilder ini = OnlyMode(mode);
		ini.remove(section, "LocalPort");

		AssertFailure(Validate(ini, mode), "[" + section + "] LocalPort");
	}

	{
		// Both ends on the same address and port, so the host would send to
		// itself and never reach the gateway.
		CIniBuilder ini = OnlyMode(mode);
		ini.set(section, "LocalAddress", "127.0.0.1");
		ini.set(section, "GatewayAddress", "127.0.0.1");
		ini.set(section, "LocalPort", "41000");
		ini.set(section, "GatewayPort", "41000");

		AssertFailure(Validate(ini, mode), "[" + section + "] LocalPort");
	}

	{
		// A network section that is switched off is not the host's problem,
		// however wrong it is.
		CIniBuilder ini = OnlyMode(mode);
		ini.set(section, "Enable", "0");
		ini.remove(section, "GatewayAddress");
		ini.remove(section, "GatewayPort");
		ini.remove(section, "LocalPort");

		AssertNoFailures(Validate(ini, mode));
	}
}

// A mode that puts the callsign on the air, or into a gateway registration,
// cannot work without one.
static void checkNeedsCallsign(const std::string& mode)
{
	CIniBuilder ini = OnlyMode(mode);
	ini.remove("General", "Callsign");

	AssertFailure(Validate(ini, mode), "[General] Callsign");
}

// A mode that is switched off must not have anything to say, however badly it
// is configured.
static void checkQuietWhenDisabled(const std::string& mode, const std::string& key, const std::string& value)
{
	CIniBuilder ini = OnlyMode(mode);
	ini.set(SectionOf(mode), "Enable", "0");
	ini.set(NetworkSectionOf(mode), "Enable", "0");
	ini.set(SectionOf(mode), key, value);

	AssertNoIssues(Validate(ini, mode));
}


// ---------------------------------------------------------------- D-Star ---

#if defined(USE_DSTAR)

TEST("dstar.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_DSTAR);
}

TEST("dstar.needs_frequencies")
{
	checkNeedsFrequencies(CONF_MODE_DSTAR);
}

TEST("dstar.needs_a_callsign")
{
	checkNeedsCallsign(CONF_MODE_DSTAR);
}

TEST("dstar.needs_a_module")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_DSTAR);
	ini.set("D-Star", "Module", BLANK);

	AssertFailure(Validate(ini, CONF_MODE_DSTAR), "[D-Star] Module");
}

TEST("dstar.rejects_a_module_outside_a_to_d")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_DSTAR);
	ini.set("D-Star", "Module", "E");

	AssertFailure(Validate(ini, CONF_MODE_DSTAR), "[D-Star] Module");
}

TEST("dstar.rejects_a_multi_letter_module")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_DSTAR);
	ini.set("D-Star", "Module", "CC");

	AssertFailure(Validate(ini, CONF_MODE_DSTAR), "[D-Star] Module");
}

TEST("dstar.network_rules")
{
	checkNetworkRules(CONF_MODE_DSTAR);
}

TEST("dstar.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_DSTAR, "Module", "E");
}


#endif


// ------------------------------------------------------------------- DMR ---

#if defined(USE_DMR)

TEST("dmr.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_DMR);
}

TEST("dmr.needs_frequencies")
{
	checkNeedsFrequencies(CONF_MODE_DMR);
}

TEST("dmr.needs_an_id")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_DMR);
	ini.remove("General", "Id");

	AssertFailure(Validate(ini, CONF_MODE_DMR), "[General] Id");
}

TEST("dmr.rejects_an_id_the_network_will_abort_on")
{
	// CDMRNetwork asserts that the id is greater than 1000, so a small id
	// takes the whole host down rather than failing cleanly.
	CIniBuilder ini = OnlyMode(CONF_MODE_DMR);
	ini.set("General", "Id", "999");

	AssertFailure(Validate(ini, CONF_MODE_DMR), "[General] Id");
}

TEST("dmr.rejects_an_id_wider_than_the_protocol_allows")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_DMR);
	ini.set("General", "Id", "16777216");

	AssertFailure(Validate(ini, CONF_MODE_DMR), "[General] Id");
}

TEST("dmr.rejects_a_colour_code_above_fifteen")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_DMR);
	ini.set("DMR", "ColorCode", "16");

	AssertFailure(Validate(ini, CONF_MODE_DMR), "[DMR] ColorCode");
}

TEST("dmr.accepts_the_full_colour_code_range")
{
	for (unsigned int colorCode = 0U; colorCode <= 15U; colorCode++) {
		CIniBuilder ini = OnlyMode(CONF_MODE_DMR);
		ini.set("DMR", "ColorCode", colorCode);

		AssertNoIssues(Validate(ini, CONF_MODE_DMR));
	}
}

TEST("dmr.warns_when_both_network_slots_are_off")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_DMR);
	ini.set("DMR Network", "Slot1", "0");
	ini.set("DMR Network", "Slot2", "0");

	AssertWarning(Validate(ini, CONF_MODE_DMR), "[DMR Network] Slot1");
}

TEST("dmr.network_rules")
{
	checkNetworkRules(CONF_MODE_DMR);
}

TEST("dmr.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_DMR, "ColorCode", "99");
}


#endif


// ---------------------------------------------------------- System Fusion ---

#if defined(USE_YSF)

TEST("ysf.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_YSF);
}

TEST("ysf.needs_frequencies")
{
	checkNeedsFrequencies(CONF_MODE_YSF);
}

TEST("ysf.needs_a_callsign")
{
	// The callsign goes into every poll the gateway sees.
	checkNeedsCallsign(CONF_MODE_YSF);
}

TEST("ysf.network_rules")
{
	checkNetworkRules(CONF_MODE_YSF);
}

TEST("ysf.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_YSF, "TXHang", "4");
}


#endif


// ------------------------------------------------------------------- P25 ---

#if defined(USE_P25)

TEST("p25.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_P25);
}

TEST("p25.needs_frequencies")
{
	checkNeedsFrequencies(CONF_MODE_P25);
}

TEST("p25.needs_an_id")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_P25);
	ini.remove("General", "Id");

	AssertFailure(Validate(ini, CONF_MODE_P25), "[General] Id");
}

TEST("p25.rejects_a_nac_above_fff")
{
	// The NAC is read as hexadecimal, so this is 0x1000.
	CIniBuilder ini = OnlyMode(CONF_MODE_P25);
	ini.set("P25", "NAC", "1000");

	AssertFailure(Validate(ini, CONF_MODE_P25), "[P25] NAC");
}

TEST("p25.accepts_the_top_of_the_nac_range")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_P25);
	ini.set("P25", "NAC", "FFF");

	AssertNoIssues(Validate(ini, CONF_MODE_P25));
}

TEST("p25.network_rules")
{
	checkNetworkRules(CONF_MODE_P25);
}

TEST("p25.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_P25, "NAC", "FFFF");
}


#endif


// ------------------------------------------------------------------ NXDN ---

#if defined(USE_NXDN)

TEST("nxdn.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_NXDN);
}

TEST("nxdn.needs_frequencies")
{
	checkNeedsFrequencies(CONF_MODE_NXDN);
}

TEST("nxdn.needs_an_id")
{
	// NXDN has its own id, and the one in the General section does not feed it.
	CIniBuilder ini = OnlyMode(CONF_MODE_NXDN);
	ini.remove("NXDN", "Id");

	AssertFailure(Validate(ini, CONF_MODE_NXDN), "[NXDN] Id");
}

TEST("nxdn.rejects_a_ran_above_sixty_three")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_NXDN);
	ini.set("NXDN", "RAN", "64");

	AssertFailure(Validate(ini, CONF_MODE_NXDN), "[NXDN] RAN");
}

TEST("nxdn.rejects_an_unknown_network_protocol")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_NXDN);
	ini.set("NXDN Network", "Protocol", "icom");	// the host matches on case

	AssertFailure(Validate(ini, CONF_MODE_NXDN), "[NXDN Network] Protocol");
}

TEST("nxdn.accepts_the_kenwood_protocol")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_NXDN);
	ini.set("NXDN Network", "Protocol", "Kenwood");

	AssertNoIssues(Validate(ini, CONF_MODE_NXDN));
}

TEST("nxdn.network_rules")
{
	checkNetworkRules(CONF_MODE_NXDN);
}

TEST("nxdn.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_NXDN, "RAN", "99");
}


#endif


// ------------------------------------------------------------------- M17 ---

#if defined(USE_M17)

TEST("m17.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_M17);
}

TEST("m17.needs_frequencies")
{
	checkNeedsFrequencies(CONF_MODE_M17);
}

TEST("m17.needs_a_callsign")
{
	checkNeedsCallsign(CONF_MODE_M17);
}

TEST("m17.rejects_a_can_above_fifteen")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_M17);
	ini.set("M17", "CAN", "16");

	AssertFailure(Validate(ini, CONF_MODE_M17), "[M17] CAN");
}

TEST("m17.accepts_the_full_can_range")
{
	for (unsigned int can = 0U; can <= 15U; can++) {
		CIniBuilder ini = OnlyMode(CONF_MODE_M17);
		ini.set("M17", "CAN", can);

		AssertNoIssues(Validate(ini, CONF_MODE_M17));
	}
}

TEST("m17.network_rules")
{
	checkNetworkRules(CONF_MODE_M17);
}

TEST("m17.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_M17, "CAN", "99");
}


#endif


// ---------------------------------------------------------------- POCSAG ---

#if defined(USE_POCSAG)

TEST("pocsag.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_POCSAG);
}

TEST("pocsag.the_frequency_falls_back_to_the_transmit_frequency")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_POCSAG);
	ini.remove("POCSAG", "Frequency");

	AssertNoIssues(Validate(ini, CONF_MODE_POCSAG));
}

TEST("pocsag.without_any_frequency_fails")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_POCSAG);
	ini.remove("POCSAG", "Frequency");
	ini.remove("Modem", "TXFrequency");

	AssertFailure(Validate(ini, CONF_MODE_POCSAG), "[POCSAG] Frequency");
}

TEST("pocsag.an_out_of_band_frequency_warns")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_POCSAG);
	ini.set("POCSAG", "Frequency", "27185000");

	AssertWarning(Validate(ini, CONF_MODE_POCSAG), "[POCSAG] Frequency");
}

TEST("pocsag.network_rules")
{
	checkNetworkRules(CONF_MODE_POCSAG);
}

TEST("pocsag.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_POCSAG, "Frequency", "0");
}


#endif


// -------------------------------------------------------------------- FM ---

#if defined(USE_FM)

TEST("fm.enabled_alone_is_clean")
{
	checkCleanAlone(CONF_MODE_FM);
}

TEST("fm.needs_frequencies")
{
	checkNeedsFrequencies(CONF_MODE_FM);
}

TEST("fm.needs_a_callsign_for_the_network")
{
	// CFMNetwork asserts on an empty callsign, so this aborts the host.
	CIniBuilder ini = OnlyMode(CONF_MODE_FM);
	ini.remove("General", "Callsign");

	AssertFailure(Validate(ini, CONF_MODE_FM), "[FM] Callsign");
}

TEST("fm.needs_a_callsign_for_identification")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_FM);
	ini.set("FM Network", "Enable", "0");
	ini.set("FM", "CallsignAtStart", "1");
	ini.remove("General", "Callsign");

	AssertFailure(Validate(ini, CONF_MODE_FM), "[FM] Callsign");
}

TEST("fm.rejects_an_unknown_access_mode")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_FM);
	ini.set("FM", "AccessMode", "4");

	AssertFailure(Validate(ini, CONF_MODE_FM), "[FM] AccessMode");
}

TEST("fm.ctcss_access_without_a_ctcss_tone_fails")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_FM);
	ini.set("FM", "AccessMode", "1");
	ini.set("FM", "CTCSSFrequency", "0.0");

	AssertFailure(Validate(ini, CONF_MODE_FM), "[FM] CTCSSFrequency");
}

TEST("fm.carrier_access_needs_no_ctcss_tone")
{
	CIniBuilder ini = OnlyMode(CONF_MODE_FM);
	ini.set("FM", "AccessMode", "0");
	ini.set("FM", "CTCSSFrequency", "0.0");

	AssertNoIssues(Validate(ini, CONF_MODE_FM));
}

TEST("fm.network_rules")
{
	checkNetworkRules(CONF_MODE_FM);
}

TEST("fm.is_quiet_when_disabled")
{
	checkQuietWhenDisabled(CONF_MODE_FM, "AccessMode", "9");
}


#endif


// ----------------------------------------------------------------- Ports ---

#if defined(USE_DSTAR) && defined(USE_M17)
TEST("ports.two_modes_on_one_local_port_fails")
{
	CIniBuilder ini;
	ini.set("M17 Network", "LocalPort", "20011");	// the D-Star network's port

	std::vector<CConfIssue> issues = Validate(ini);

	AssertFailure(issues, "[M17] LocalPort");
}

#endif

#if defined(USE_M17)
TEST("ports.a_disabled_mode_does_not_clash")
{
	CIniBuilder ini;
	ini.set("M17", "Enable", "0");
	ini.set("M17 Network", "Enable", "0");
	ini.set("M17 Network", "LocalPort", "20011");

	AssertNoFailures(Validate(ini));
}

TEST("ports.the_udp_modem_clashes_with_a_mode")
{
	CIniBuilder ini;
	ini.set("Modem", "Protocol", "udp");
	ini.set("Modem", "ModemAddress", "192.168.2.100");
	ini.set("Modem", "ModemPort", "3334");
	ini.set("Modem", "LocalPort", "17011");		// the M17 network's port

	AssertFailure(Validate(ini), "[Modem] LocalPort");
}

#endif

#if defined(USE_FM)
TEST("ports.transparent_data_clashes_with_a_mode")
{
	CIniBuilder ini;
	ini.set("Transparent Data", "Enable", "1");
	ini.set("Transparent Data", "RemoteAddress", "127.0.0.1");
	ini.set("Transparent Data", "RemotePort", "40094");
	ini.set("Transparent Data", "LocalPort", "3810");	// the FM network's port

	AssertFailure(Validate(ini), "[General] LocalPort");
}
#endif
