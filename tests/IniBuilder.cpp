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

#include "IniBuilder.h"
#include "TestFramework.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace {

	std::vector<std::string> m_files;

	void removeTempFiles()
	{
		for (const auto& file : m_files)
			::unlink(file.c_str());

		m_files.clear();
	}

	std::string tempDirectory()
	{
		const char* dir = ::getenv("TMPDIR");

		return (dir != nullptr && dir[0] != '\0') ? std::string(dir) : std::string("/tmp");
	}

}

CIniBuilder::CIniBuilder() :
m_sections()
{
	// Note that CConf splits a line on the first space, tab or '=', so the
	// keys here must not be written with spaces around the '='. A key with an
	// empty value is discarded by CConf, which is why remove() exists.
	set("General", "Callsign", "G9BF");
	set("General", "Id", "1234567");
	set("General", "Timeout", "180");
	set("General", "Duplex", "1");
	set("General", "Daemon", "0");

	set("Log", "MQTTLevel", "0");
	set("Log", "DisplayLevel", "0");

	set("MQTT", "Host", "127.0.0.1");
	set("MQTT", "Port", "1883");
	set("MQTT", "Auth", "0");
	set("MQTT", "Keepalive", "60");
	set("MQTT", "Name", "host");

	set("CW Id", "Enable", "1");
	set("CW Id", "Time", "10");

	set("DMR Id Lookup", "File", "DMRIds.dat");
	set("DMR Id Lookup", "Time", "24");

	set("NXDN Id Lookup", "File", "NXDN.csv");
	set("NXDN Id Lookup", "Time", "24");

	set("Modem", "Protocol", "uart");
	set("Modem", "UARTPort", "/dev/ttyACM0");
	set("Modem", "UARTSpeed", "115200");
	// The frequencies live in [Modem], not in the [Info] section they were read
	// from historically: b7d15b8 ("Remove the DMRC startup message.") took the
	// whole [Info] section out along with the position and description fields
	// that only the DMRC message and the display used.
	set("Modem", "RXFrequency", "430000000");
	set("Modem", "TXFrequency", "439000000");
	set("Modem", "TXInvert", "1");
	set("Modem", "RXInvert", "0");
	set("Modem", "PTTInvert", "0");
	set("Modem", "TXDelay", "100");
	set("Modem", "RXLevel", "50");
	set("Modem", "TXLevel", "50");
	set("Modem", "RFLevel", "100");

	set("Transparent Data", "Enable", "0");

	set("D-Star", "Enable", "1");
	set("D-Star", "Module", "C");
	set("D-Star", "SelfOnly", "0");

	set("DMR", "Enable", "1");
	set("DMR", "ColorCode", "1");
	set("DMR", "SelfOnly", "0");
	set("DMR", "CallHang", "3");
	set("DMR", "TXHang", "4");

	set("System Fusion", "Enable", "1");
	set("System Fusion", "SelfOnly", "0");
	set("System Fusion", "TXHang", "4");

	set("P25", "Enable", "1");
	set("P25", "NAC", "293");
	set("P25", "SelfOnly", "0");
	set("P25", "TXHang", "5");

	set("NXDN", "Enable", "1");
	set("NXDN", "Id", "12345");
	set("NXDN", "RAN", "1");
	set("NXDN", "SelfOnly", "0");
	set("NXDN", "TXHang", "5");

	set("M17", "Enable", "1");
	set("M17", "CAN", "0");
	set("M17", "SelfOnly", "0");
	set("M17", "TXHang", "5");

	set("POCSAG", "Enable", "1");
	set("POCSAG", "Frequency", "439987500");

	set("FM", "Enable", "1");
	set("FM", "CallsignSpeed", "20");
	set("FM", "CallsignTime", "10");
	set("FM", "CallsignAtStart", "1");
	set("FM", "CallsignAtEnd", "1");
	set("FM", "AccessMode", "1");
	set("FM", "CTCSSFrequency", "88.4");
	set("FM", "CTCSSThreshold", "30");
	set("FM", "HangTime", "7");

	set("D-Star Network", "Enable", "1");
	set("D-Star Network", "LocalAddress", "127.0.0.1");
	set("D-Star Network", "LocalPort", "20011");
	set("D-Star Network", "GatewayAddress", "127.0.0.1");
	set("D-Star Network", "GatewayPort", "20010");
	set("D-Star Network", "Debug", "0");

	set("DMR Network", "Enable", "1");
	set("DMR Network", "LocalAddress", "127.0.0.1");
	set("DMR Network", "LocalPort", "62032");
	set("DMR Network", "GatewayAddress", "127.0.0.1");
	set("DMR Network", "GatewayPort", "62031");
	set("DMR Network", "Jitter", "360");
	set("DMR Network", "Slot1", "1");
	set("DMR Network", "Slot2", "1");
	set("DMR Network", "Debug", "0");

	set("System Fusion Network", "Enable", "1");
	set("System Fusion Network", "LocalAddress", "127.0.0.1");
	set("System Fusion Network", "LocalPort", "3200");
	set("System Fusion Network", "GatewayAddress", "127.0.0.1");
	set("System Fusion Network", "GatewayPort", "4200");
	set("System Fusion Network", "Debug", "0");

	set("P25 Network", "Enable", "1");
	set("P25 Network", "LocalAddress", "127.0.0.1");
	set("P25 Network", "LocalPort", "32010");
	set("P25 Network", "GatewayAddress", "127.0.0.1");
	set("P25 Network", "GatewayPort", "42020");
	set("P25 Network", "Debug", "0");

	set("NXDN Network", "Enable", "1");
	set("NXDN Network", "Protocol", "Icom");
	set("NXDN Network", "LocalAddress", "127.0.0.1");
	set("NXDN Network", "LocalPort", "14021");
	set("NXDN Network", "GatewayAddress", "127.0.0.1");
	set("NXDN Network", "GatewayPort", "14020");
	set("NXDN Network", "Debug", "0");

	set("M17 Network", "Enable", "1");
	set("M17 Network", "LocalAddress", "127.0.0.1");
	set("M17 Network", "LocalPort", "17011");
	set("M17 Network", "GatewayAddress", "127.0.0.1");
	set("M17 Network", "GatewayPort", "17010");
	set("M17 Network", "Debug", "0");

	set("POCSAG Network", "Enable", "1");
	set("POCSAG Network", "LocalAddress", "127.0.0.1");
	set("POCSAG Network", "LocalPort", "3800");
	set("POCSAG Network", "GatewayAddress", "127.0.0.1");
	set("POCSAG Network", "GatewayPort", "4800");
	set("POCSAG Network", "Debug", "0");

	set("FM Network", "Enable", "1");
	set("FM Network", "LocalAddress", "127.0.0.1");
	set("FM Network", "LocalPort", "3810");
	set("FM Network", "GatewayAddress", "127.0.0.1");
	set("FM Network", "GatewayPort", "4810");
	set("FM Network", "PreEmphasis", "1");
	set("FM Network", "DeEmphasis", "1");
	set("FM Network", "TXAudioGain", "1.0");
	set("FM Network", "RXAudioGain", "1.0");
	set("FM Network", "Debug", "0");

	set("Lock File", "Enable", "0");

	set("Remote Control", "Enable", "0");
}

CIniBuilder CIniBuilder::empty()
{
	CIniBuilder builder;
	builder.m_sections.clear();

	return builder;
}

std::vector<CIniBuilder::KEYVALUE>* CIniBuilder::find(const std::string& section)
{
	for (auto& it : m_sections) {
		if (it.first == section)
			return &it.second;
	}

	m_sections.push_back(std::make_pair(section, std::vector<KEYVALUE>()));

	return &m_sections.back().second;
}

CIniBuilder& CIniBuilder::set(const std::string& section, const std::string& key, const std::string& value)
{
	std::vector<KEYVALUE>* values = find(section);

	for (auto& it : *values) {
		if (it.first == key) {
			it.second = value;
			return *this;
		}
	}

	values->push_back(std::make_pair(key, value));

	return *this;
}

CIniBuilder& CIniBuilder::set(const std::string& section, const std::string& key, unsigned int value)
{
	return set(section, key, std::to_string(value));
}

CIniBuilder& CIniBuilder::remove(const std::string& section, const std::string& key)
{
	std::vector<KEYVALUE>* values = find(section);

	for (auto it = values->begin(); it != values->end(); ++it) {
		if (it->first == key) {
			values->erase(it);
			return *this;
		}
	}

	return *this;
}

CIniBuilder& CIniBuilder::removeSection(const std::string& section)
{
	for (auto it = m_sections.begin(); it != m_sections.end(); ++it) {
		if (it->first == section) {
			m_sections.erase(it);
			return *this;
		}
	}

	return *this;
}

CIniBuilder& CIniBuilder::disableAllModes()
{
	const char* MODES[] = { "D-Star", "DMR", "System Fusion", "P25", "NXDN", "M17", "POCSAG", "FM" };

	for (const char* mode : MODES) {
		set(mode, "Enable", "0");
		set(std::string(mode) + " Network", "Enable", "0");
	}

	return *this;
}

std::string CIniBuilder::text() const
{
	std::string text;

	for (const auto& section : m_sections) {
		text += "[" + section.first + "]\n";

		for (const auto& value : section.second)
			text += value.first + "=" + value.second + "\n";

		text += "\n";
	}

	return text;
}

std::string CIniBuilder::write() const
{
	if (m_files.empty())
		::atexit(removeTempFiles);

	std::string path = tempDirectory() + "/mmdvm-host-test-XXXXXX";

	std::vector<char> name(path.begin(), path.end());
	name.push_back('\0');

	int fd = ::mkstemp(name.data());
	if (fd == -1)
		TestFail("cannot create a temporary .ini file in " + tempDirectory(), __FILE__, __LINE__);

	std::string contents = text();

	ssize_t n = ::write(fd, contents.c_str(), contents.length());
	::close(fd);

	if (n != ssize_t(contents.length()))
		TestFail("cannot write the temporary .ini file", __FILE__, __LINE__);

	std::string file(name.data());
	m_files.push_back(file);

	return file;
}
