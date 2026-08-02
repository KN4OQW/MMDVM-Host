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

#include "Conf.h"

// A value of "" in the .ini file is the only way to blank a setting that has a
// non-empty built in default, because a key with nothing after the '=' never
// reaches CConf at all.
static const char* const BLANK = "\"\"";

TEST("conf.parse.missing_file_is_rejected")
{
	CConf conf("/nonexistent/there-is-no-such-ini-file.ini");

	ASSERT_FALSE(conf.read());
}

TEST("conf.parse.values_are_read_back")
{
	CIniBuilder ini;
	ini.set("General", "Callsign", "g9bf");		// deliberately lower case
	ini.set("General", "Id", "1234567");
	ini.set("Modem", "RXFrequency", "430100000");
	ini.set("Modem", "TXFrequency", "439100000");
	ini.set("P25", "NAC", "3F1");			// the NAC is hexadecimal
	ini.set("DMR", "ColorCode", "7");
	ini.set("M17", "CAN", "9");

	std::string file = ini.write();

	CConf conf(file);
	ASSERT_TRUE(conf.read());

	ASSERT_STREQ("G9BF", conf.getCallsign());
	ASSERT_EQ(1234567U, conf.getId());
	ASSERT_EQ(430100000U, conf.getModemRXFrequency());
	ASSERT_EQ(439100000U, conf.getModemTXFrequency());
	ASSERT_EQ(0x3F1U, conf.getP25NAC());
	ASSERT_EQ(7U, conf.getDMRColorCode());
	ASSERT_EQ(9U, conf.getM17CAN());

	// One Id in the General section feeds both the DMR and the P25 mode.
	ASSERT_EQ(1234567U, conf.getDMRId());
	ASSERT_EQ(1234567U, conf.getP25Id());
}

TEST("conf.parse.key_with_no_value_keeps_the_default")
{
	// This is the trap an operator falls into when they comment a value out by
	// deleting it rather than the whole line: the default stays in force.
	CIniBuilder ini;
	ini.set("General", "Timeout", "");

	std::string file = ini.write();

	CConf conf(file);
	ASSERT_TRUE(conf.read());

	ASSERT_EQ(120U, conf.getTimeout());
}

TEST("conf.validate.a_complete_configuration_is_clean")
{
	CIniBuilder ini;

	AssertNoIssues(Validate(ini));
}

TEST("conf.general.a_missing_callsign_fails")
{
	CIniBuilder ini;
	ini.remove("General", "Callsign");

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[General] Callsign");
}

TEST("conf.general.a_blank_callsign_fails")
{
	CIniBuilder ini;
	ini.set("General", "Callsign", BLANK);

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[General] Callsign");
}

TEST("conf.general.an_over_long_callsign_fails")
{
	CIniBuilder ini;
	ini.set("General", "Callsign", "G9BFTEST1");

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[General] Callsign");
}

TEST("conf.general.a_callsign_with_punctuation_fails")
{
	CIniBuilder ini;
	ini.set("General", "Callsign", "G9BF!");

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[General] Callsign");
}

TEST("conf.general.a_callsign_without_a_digit_warns")
{
	CIniBuilder ini;
	ini.set("General", "Callsign", "GBBF");

	AssertWarning(Validate(ini, CONF_MODE_GENERAL), "[General] Callsign");
	AssertNoFailures(Validate(ini, CONF_MODE_GENERAL));
}

TEST("conf.general.a_missing_rx_frequency_fails")
{
	CIniBuilder ini;
	ini.remove("Modem", "RXFrequency");

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[Modem] RXFrequency");
}

TEST("conf.general.a_missing_tx_frequency_fails")
{
	CIniBuilder ini;
	ini.remove("Modem", "TXFrequency");

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[Modem] TXFrequency");
}

TEST("conf.general.a_missing_modem_section_fails_both_frequencies")
{
	CIniBuilder ini;
	ini.removeSection("Modem");

	std::vector<CConfIssue> issues = Validate(ini, CONF_MODE_GENERAL);

	AssertFailure(issues, "[Modem] RXFrequency");
	AssertFailure(issues, "[Modem] TXFrequency");
}

TEST("conf.general.an_out_of_band_frequency_warns")
{
	CIniBuilder ini;
	ini.set("Modem", "RXFrequency", "14200000");	// 14 MHz, not something a modem will do
	ini.set("Modem", "TXFrequency", "14200000");

	std::vector<CConfIssue> issues = Validate(ini, CONF_MODE_GENERAL);

	AssertWarning(issues, "[Modem] RXFrequency");
	AssertWarning(issues, "[Modem] TXFrequency");
}

TEST("conf.general.duplex_on_one_frequency_warns")
{
	CIniBuilder ini;
	ini.set("General", "Duplex", "1");
	ini.set("Modem", "RXFrequency", "430000000");
	ini.set("Modem", "TXFrequency", "430000000");

	AssertWarning(Validate(ini, CONF_MODE_GENERAL), "[General] Duplex");
}

TEST("conf.general.a_disabled_timeout_warns")
{
	CIniBuilder ini;
	ini.set("General", "Timeout", "0");

	AssertWarning(Validate(ini, CONF_MODE_GENERAL), "[General] Timeout");
}

TEST("conf.general.a_blank_mqtt_host_fails")
{
	CIniBuilder ini;
	ini.set("MQTT", "Host", BLANK);

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[MQTT] Host");
}

TEST("conf.general.cw_identification_without_a_callsign_fails")
{
	CIniBuilder ini;
	ini.set("CW Id", "Enable", "1");
	ini.remove("General", "Callsign");

	AssertFailure(Validate(ini, CONF_MODE_GENERAL), "[CW Id] Callsign");
}

TEST("conf.modem.an_unknown_protocol_fails")
{
	CIniBuilder ini;
	ini.set("Modem", "Protocol", "serial");

	AssertFailure(Validate(ini, CONF_MODE_MODEM), "[Modem] Protocol");
}

TEST("conf.modem.uart_without_a_port_fails")
{
	CIniBuilder ini;
	ini.set("Modem", "Protocol", "uart");
	ini.remove("Modem", "UARTPort");

	AssertFailure(Validate(ini, CONF_MODE_MODEM), "[Modem] UARTPort");
}

TEST("conf.modem.udp_without_an_address_fails")
{
	CIniBuilder ini;
	ini.set("Modem", "Protocol", "udp");
	ini.set("Modem", "ModemPort", "3334");
	ini.set("Modem", "LocalPort", "3335");

	AssertFailure(Validate(ini, CONF_MODE_MODEM), "[Modem] ModemAddress");
}

TEST("conf.modem.udp_without_ports_fails")
{
	CIniBuilder ini;
	ini.set("Modem", "Protocol", "udp");
	ini.set("Modem", "ModemAddress", "192.168.2.100");

	std::vector<CConfIssue> issues = Validate(ini, CONF_MODE_MODEM);

	AssertFailure(issues, "[Modem] ModemPort");
	AssertFailure(issues, "[Modem] LocalPort");
}

TEST("conf.modem.i2c_without_a_port_fails")
{
	CIniBuilder ini;
	ini.set("Modem", "Protocol", "i2c");

	AssertFailure(Validate(ini, CONF_MODE_MODEM), "[Modem] I2CPort");
}

TEST("conf.modem.the_null_modem_warns_when_modes_are_enabled")
{
	CIniBuilder ini;
	ini.set("Modem", "Protocol", "null");

	AssertWarning(Validate(ini, CONF_MODE_MODEM), "[Modem] Protocol");
}

TEST("conf.modem.the_null_modem_is_quiet_with_no_modes")
{
	CIniBuilder ini;
	ini.disableAllModes();
	ini.set("Modem", "Protocol", "null");

	AssertNoIssues(Validate(ini, CONF_MODE_MODEM));
}
