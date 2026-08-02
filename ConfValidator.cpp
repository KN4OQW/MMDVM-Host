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

#include "ConfValidator.h"

#include <algorithm>
#include <cctype>

namespace {

	// The frequency range an MMDVM modem can be asked to work over. Outside
	// this the modem will simply refuse, so it is worth flagging early.
	const unsigned int MIN_FREQUENCY = 100000000U;
	const unsigned int MAX_FREQUENCY = 1300000000U;

	// The alphabet M17 callsigns are base-40 encoded from. Anything else is
	// silently turned into a space by CM17Utils::encodeCallsign(), so the
	// callsign that goes out over the air is not the one in the .ini file.
	const std::string M17_CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/.";

	void add(std::vector<CConfIssue>& issues, CONF_SEVERITY severity, const std::string& mode, const std::string& key, const std::string& message)
	{
		CConfIssue issue;
		issue.severity = severity;
		issue.mode     = mode;
		issue.key      = key;
		issue.message  = message;

		issues.push_back(issue);
	}

	bool isCallsignChar(char c)
	{
		return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '/';
	}

	bool hasDigit(const std::string& text)
	{
		return std::any_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; });
	}

	std::string trimmed(const std::string& text)
	{
		size_t first = text.find_first_not_of(' ');
		if (first == std::string::npos)
			return "";

		size_t last = text.find_last_not_of(' ');

		return text.substr(first, last - first + 1U);
	}

}

CConfValidator::CConfValidator(const CConf& conf) :
m_conf(conf)
{
}

CConfValidator::~CConfValidator()
{
}

std::vector<std::string> CConfValidator::modes()
{
	std::vector<std::string> modes;

#if defined(USE_DSTAR)
	modes.push_back(CONF_MODE_DSTAR);
#endif
#if defined(USE_DMR)
	modes.push_back(CONF_MODE_DMR);
#endif
#if defined(USE_YSF)
	modes.push_back(CONF_MODE_YSF);
#endif
#if defined(USE_P25)
	modes.push_back(CONF_MODE_P25);
#endif
#if defined(USE_NXDN)
	modes.push_back(CONF_MODE_NXDN);
#endif
#if defined(USE_M17)
	modes.push_back(CONF_MODE_M17);
#endif
#if defined(USE_POCSAG)
	modes.push_back(CONF_MODE_POCSAG);
#endif
#if defined(USE_FM)
	modes.push_back(CONF_MODE_FM);
#endif

	return modes;
}

std::vector<CConfIssue> CConfValidator::validate() const
{
	std::vector<CConfIssue> issues;

	validateGeneral(issues);
	validateModem(issues);
#if defined(USE_DSTAR)
	validateDStar(issues);
#endif
#if defined(USE_DMR)
	validateDMR(issues);
#endif
#if defined(USE_YSF)
	validateYSF(issues);
#endif
#if defined(USE_P25)
	validateP25(issues);
#endif
#if defined(USE_NXDN)
	validateNXDN(issues);
#endif
#if defined(USE_M17)
	validateM17(issues);
#endif
#if defined(USE_POCSAG)
	validatePOCSAG(issues);
#endif
#if defined(USE_FM)
	validateFM(issues);
#endif
	validatePorts(issues);

	return issues;
}

std::vector<CConfIssue> CConfValidator::validate(const std::string& mode) const
{
	std::vector<CConfIssue> all = validate();
	std::vector<CConfIssue> issues;

	for (const auto& issue : all) {
		if (issue.mode == mode)
			issues.push_back(issue);
	}

	return issues;
}

bool CConfValidator::isModeEnabled(const std::string& mode) const
{
#if defined(USE_DSTAR)
	if (mode == CONF_MODE_DSTAR)
		return m_conf.getDStarEnabled();
#endif
#if defined(USE_DMR)
	if (mode == CONF_MODE_DMR)
		return m_conf.getDMREnabled();
#endif
#if defined(USE_YSF)
	if (mode == CONF_MODE_YSF)
		return m_conf.getFusionEnabled();
#endif
#if defined(USE_P25)
	if (mode == CONF_MODE_P25)
		return m_conf.getP25Enabled();
#endif
#if defined(USE_NXDN)
	if (mode == CONF_MODE_NXDN)
		return m_conf.getNXDNEnabled();
#endif
#if defined(USE_M17)
	if (mode == CONF_MODE_M17)
		return m_conf.getM17Enabled();
#endif
#if defined(USE_POCSAG)
	if (mode == CONF_MODE_POCSAG)
		return m_conf.getPOCSAGEnabled();
#endif
#if defined(USE_FM)
	if (mode == CONF_MODE_FM)
		return m_conf.getFMEnabled();
#endif

	return false;
}

bool CConfValidator::isNetworkEnabled(const std::string& mode) const
{
#if defined(USE_DSTAR)
	if (mode == CONF_MODE_DSTAR)
		return m_conf.getDStarNetworkEnabled();
#endif
#if defined(USE_DMR)
	if (mode == CONF_MODE_DMR)
		return m_conf.getDMRNetworkEnabled();
#endif
#if defined(USE_YSF)
	if (mode == CONF_MODE_YSF)
		return m_conf.getFusionNetworkEnabled();
#endif
#if defined(USE_P25)
	if (mode == CONF_MODE_P25)
		return m_conf.getP25NetworkEnabled();
#endif
#if defined(USE_NXDN)
	if (mode == CONF_MODE_NXDN)
		return m_conf.getNXDNNetworkEnabled();
#endif
#if defined(USE_M17)
	if (mode == CONF_MODE_M17)
		return m_conf.getM17NetworkEnabled();
#endif
#if defined(USE_POCSAG)
	if (mode == CONF_MODE_POCSAG)
		return m_conf.getPOCSAGNetworkEnabled();
#endif
#if defined(USE_FM)
	if (mode == CONF_MODE_FM)
		return m_conf.getFMNetworkEnabled();
#endif

	return false;
}

unsigned int CConfValidator::count(const std::vector<CConfIssue>& issues, CONF_SEVERITY severity)
{
	unsigned int n = 0U;

	for (const auto& issue : issues) {
		if (issue.severity == severity)
			n++;
	}

	return n;
}

bool CConfValidator::has(const std::vector<CConfIssue>& issues, CONF_SEVERITY severity, const std::string& key)
{
	for (const auto& issue : issues) {
		if (issue.severity == severity && issue.key == key)
			return true;
	}

	return false;
}

std::string CConfValidator::format(const CConfIssue& issue)
{
	std::string text = issue.severity == CONF_SEVERITY::FAILURE ? "FAIL " : "WARN ";

	text += issue.mode;
	text += ": ";
	text += issue.key;
	text += " - ";
	text += issue.message;

	return text;
}

bool CConfValidator::anyRFModeEnabled() const
{
	for (const auto& mode : modes()) {
		if (isModeEnabled(mode))
			return true;
	}

	return false;
}

void CConfValidator::validateGeneral(std::vector<CConfIssue>& issues) const
{
	std::string callsign = trimmed(m_conf.getCallsign());

	if (callsign.empty()) {
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[General] Callsign", "no callsign has been set");
	} else {
		if (callsign.length() > 8U)
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[General] Callsign", "the callsign is longer than the eight characters the protocols allow");

		if (!std::all_of(callsign.begin(), callsign.end(), isCallsignChar))
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[General] Callsign", "the callsign contains characters other than A-Z, 0-9, '-' and '/'");
		else if (!hasDigit(callsign))
			add(issues, CONF_SEVERITY::WARNING, CONF_MODE_GENERAL, "[General] Callsign", "the callsign contains no digit, which is unusual for an amateur callsign");
	}

	unsigned int rxFrequency = m_conf.getModemRXFrequency();
	unsigned int txFrequency = m_conf.getModemTXFrequency();

	if (rxFrequency == 0U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[Modem] RXFrequency", "no receive frequency has been set");
	else if (rxFrequency < MIN_FREQUENCY || rxFrequency > MAX_FREQUENCY)
		add(issues, CONF_SEVERITY::WARNING, CONF_MODE_GENERAL, "[Modem] RXFrequency", "the receive frequency is outside the range an MMDVM modem will accept");

	if (txFrequency == 0U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[Modem] TXFrequency", "no transmit frequency has been set");
	else if (txFrequency < MIN_FREQUENCY || txFrequency > MAX_FREQUENCY)
		add(issues, CONF_SEVERITY::WARNING, CONF_MODE_GENERAL, "[Modem] TXFrequency", "the transmit frequency is outside the range an MMDVM modem will accept");

	if (m_conf.getDuplex() && rxFrequency != 0U && rxFrequency == txFrequency)
		add(issues, CONF_SEVERITY::WARNING, CONF_MODE_GENERAL, "[General] Duplex", "duplex is enabled but the receive and transmit frequencies are the same");

	if (m_conf.getTimeout() == 0U)
		add(issues, CONF_SEVERITY::WARNING, CONF_MODE_GENERAL, "[General] Timeout", "the transmission timeout is disabled");

	if (m_conf.getMQTTHost().empty())
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[MQTT] Host", "no MQTT broker address has been set");

	if (m_conf.getMQTTPort() == 0U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[MQTT] Port", "no MQTT broker port has been set");

	if (m_conf.getCWIdEnabled() && trimmed(m_conf.getCWIdCallsign()).empty())
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_GENERAL, "[CW Id] Callsign", "the CW identity is enabled but no callsign is available for it");
}

void CConfValidator::validateModem(std::vector<CConfIssue>& issues) const
{
	std::string protocol = m_conf.getModemProtocol();

	if (protocol == "uart") {
		if (m_conf.getModemUARTPort().empty())
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_MODEM, "[Modem] UARTPort", "the UART protocol is selected but no port has been set");

		if (m_conf.getModemUARTSpeed() == 0U)
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_MODEM, "[Modem] UARTSpeed", "the UART protocol is selected but no speed has been set");
	} else if (protocol == "i2c") {
		if (m_conf.getModemI2CPort().empty())
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_MODEM, "[Modem] I2CPort", "the I2C protocol is selected but no port has been set");
	} else if (protocol == "udp") {
		if (m_conf.getModemModemAddress().empty())
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_MODEM, "[Modem] ModemAddress", "the UDP protocol is selected but no modem address has been set");

		if (m_conf.getModemModemPort() == 0U)
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_MODEM, "[Modem] ModemPort", "the UDP protocol is selected but no modem port has been set");

		if (m_conf.getModemLocalPort() == 0U)
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_MODEM, "[Modem] LocalPort", "the UDP protocol is selected but no local port has been set");
	} else if (protocol == "null") {
		if (anyRFModeEnabled())
			add(issues, CONF_SEVERITY::WARNING, CONF_MODE_MODEM, "[Modem] Protocol", "the null modem is selected, so no mode will reach the air");
	} else {
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_MODEM, "[Modem] Protocol", "unknown protocol '" + protocol + "', expected one of null, uart, udp or i2c");
	}
}

void CConfValidator::validateNetwork(std::vector<CConfIssue>& issues, const std::string& mode, const std::string& section,
				     const std::string& gatewayAddress, unsigned short gatewayPort,
				     const std::string& localAddress, unsigned short localPort) const
{
	if (gatewayAddress.empty())
		add(issues, CONF_SEVERITY::FAILURE, mode, "[" + section + "] GatewayAddress", "the network is enabled but no gateway address has been set");

	if (gatewayPort == 0U)
		add(issues, CONF_SEVERITY::FAILURE, mode, "[" + section + "] GatewayPort", "the network is enabled but no gateway port has been set");

	if (localPort == 0U)
		add(issues, CONF_SEVERITY::FAILURE, mode, "[" + section + "] LocalPort", "the network is enabled but no local port has been set");

	// The same address and port at both ends means the host talks to itself.
	bool sameAddress = localAddress.empty() ? (gatewayAddress == "127.0.0.1" || gatewayAddress == "localhost") : (localAddress == gatewayAddress);
	if (sameAddress && localPort != 0U && localPort == gatewayPort)
		add(issues, CONF_SEVERITY::FAILURE, mode, "[" + section + "] LocalPort", "the local and gateway ports are the same on the same address, so the host would talk to itself");
}

#if defined(USE_DSTAR)
void CConfValidator::validateDStar(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getDStarEnabled())
		return;

	std::string module = trimmed(m_conf.getDStarModule());

	if (module.empty())
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_DSTAR, "[D-Star] Module", "no module has been set");
	else if (module.length() != 1U || module[0] < 'A' || module[0] > 'D')
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_DSTAR, "[D-Star] Module", "the module must be a single letter from A to D, not '" + module + "'");

	if (trimmed(m_conf.getCallsign()).empty())
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_DSTAR, "[General] Callsign", "D-Star is enabled but no callsign has been set to identify the repeater");

	if (m_conf.getDStarNetworkEnabled())
		validateNetwork(issues, CONF_MODE_DSTAR, "D-Star Network", m_conf.getDStarGatewayAddress(), m_conf.getDStarGatewayPort(),
				m_conf.getDStarLocalAddress(), m_conf.getDStarLocalPort());
}
#endif

#if defined(USE_DMR)
void CConfValidator::validateDMR(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getDMREnabled())
		return;

	unsigned int colorCode = m_conf.getDMRColorCode();
	if (colorCode > 15U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_DMR, "[DMR] ColorCode", "the colour code must be in the range 0 to 15");

	unsigned int id = m_conf.getDMRId();
	if (id == 0U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_DMR, "[General] Id", "DMR is enabled but no DMR id has been set");
	else if (id > 16777215U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_DMR, "[General] Id", "the DMR id is larger than the 24 bits the protocol allows");

	if (m_conf.getDMRBeacons() == DMR_BEACONS::TIMED && m_conf.getDMRBeaconInterval() == 0U)
		add(issues, CONF_SEVERITY::WARNING, CONF_MODE_DMR, "[DMR] BeaconInterval", "timed beacons are enabled but the interval is zero");

	if (m_conf.getDMRNetworkEnabled()) {
		validateNetwork(issues, CONF_MODE_DMR, "DMR Network", m_conf.getDMRNetworkGatewayAddress(), m_conf.getDMRNetworkGatewayPort(),
				m_conf.getDMRNetworkLocalAddress(), m_conf.getDMRNetworkLocalPort());

		// CDMRNetwork asserts on this, so an id of 1000 or less aborts the host.
		if (id != 0U && id <= 1000U)
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_DMR, "[General] Id", "the DMR id must be greater than 1000 for the network to be used");

		if (!m_conf.getDMRNetworkSlot1() && !m_conf.getDMRNetworkSlot2())
			add(issues, CONF_SEVERITY::WARNING, CONF_MODE_DMR, "[DMR Network] Slot1", "both network slots are disabled, so no traffic will pass");
	}
}
#endif

#if defined(USE_YSF)
void CConfValidator::validateYSF(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getFusionEnabled())
		return;

	// The callsign goes into every network poll, and a blank one leaves the
	// gateway unable to tell this repeater from any other.
	if (trimmed(m_conf.getCallsign()).empty())
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_YSF, "[General] Callsign", "System Fusion is enabled but no callsign has been set for the network polls");

	if (m_conf.getFusionNetworkEnabled())
		validateNetwork(issues, CONF_MODE_YSF, "System Fusion Network", m_conf.getFusionNetworkGatewayAddress(), m_conf.getFusionNetworkGatewayPort(),
				m_conf.getFusionNetworkLocalAddress(), m_conf.getFusionNetworkLocalPort());
}
#endif

#if defined(USE_P25)
void CConfValidator::validateP25(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getP25Enabled())
		return;

	if (m_conf.getP25NAC() > 0xFFFU)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_P25, "[P25] NAC", "the NAC must be in the range 000 to FFF and is written in hexadecimal");

	if (m_conf.getP25Id() == 0U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_P25, "[General] Id", "P25 is enabled but no id has been set");

	if (m_conf.getP25NetworkEnabled())
		validateNetwork(issues, CONF_MODE_P25, "P25 Network", m_conf.getP25GatewayAddress(), m_conf.getP25GatewayPort(),
				m_conf.getP25LocalAddress(), m_conf.getP25LocalPort());
}
#endif

#if defined(USE_NXDN)
void CConfValidator::validateNXDN(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getNXDNEnabled())
		return;

	if (m_conf.getNXDNRAN() > 63U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_NXDN, "[NXDN] RAN", "the RAN must be in the range 0 to 63");

	if (m_conf.getNXDNId() == 0U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_NXDN, "[NXDN] Id", "NXDN is enabled but no NXDN id has been set");

	if (m_conf.getNXDNNetworkEnabled()) {
		std::string protocol = m_conf.getNXDNNetworkProtocol();
		if (protocol != "Icom" && protocol != "Kenwood")
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_NXDN, "[NXDN Network] Protocol", "unknown protocol '" + protocol + "', expected Icom or Kenwood");

		validateNetwork(issues, CONF_MODE_NXDN, "NXDN Network", m_conf.getNXDNGatewayAddress(), m_conf.getNXDNGatewayPort(),
				m_conf.getNXDNLocalAddress(), m_conf.getNXDNLocalPort());
	}
}
#endif

#if defined(USE_M17)
void CConfValidator::validateM17(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getM17Enabled())
		return;

	if (m_conf.getM17CAN() > 15U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_M17, "[M17] CAN", "the CAN must be in the range 0 to 15");

	std::string callsign = trimmed(m_conf.getCallsign());

	if (callsign.empty()) {
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_M17, "[General] Callsign", "M17 is enabled but no callsign has been set");
	} else {
		if (callsign.length() > 9U)
			add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_M17, "[General] Callsign", "an M17 callsign cannot be longer than nine characters");

		for (char c : callsign) {
			if (M17_CHARS.find(c) == std::string::npos) {
				add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_M17, "[General] Callsign", "the callsign contains characters M17 cannot encode, and they would go out as spaces");
				break;
			}
		}
	}

	if (m_conf.getM17NetworkEnabled())
		validateNetwork(issues, CONF_MODE_M17, "M17 Network", m_conf.getM17GatewayAddress(), m_conf.getM17GatewayPort(),
				m_conf.getM17LocalAddress(), m_conf.getM17LocalPort());
}
#endif

#if defined(USE_POCSAG)
void CConfValidator::validatePOCSAG(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getPOCSAGEnabled())
		return;

	unsigned int frequency = m_conf.getPOCSAGFrequency();

	if (frequency == 0U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_POCSAG, "[POCSAG] Frequency", "POCSAG is enabled but no frequency has been set");
	else if (frequency < MIN_FREQUENCY || frequency > MAX_FREQUENCY)
		add(issues, CONF_SEVERITY::WARNING, CONF_MODE_POCSAG, "[POCSAG] Frequency", "the POCSAG frequency is outside the range an MMDVM modem will accept");

	if (m_conf.getPOCSAGNetworkEnabled())
		validateNetwork(issues, CONF_MODE_POCSAG, "POCSAG Network", m_conf.getPOCSAGGatewayAddress(), m_conf.getPOCSAGGatewayPort(),
				m_conf.getPOCSAGLocalAddress(), m_conf.getPOCSAGLocalPort());
}
#endif

#if defined(USE_FM)
void CConfValidator::validateFM(std::vector<CConfIssue>& issues) const
{
	if (!m_conf.getFMEnabled())
		return;

	std::string callsign = trimmed(m_conf.getFMCallsign());

	// CFMNetwork asserts on an empty callsign, so this aborts the host rather
	// than failing cleanly.
	if (callsign.empty() && m_conf.getFMNetworkEnabled())
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_FM, "[FM] Callsign", "the FM network is enabled but no callsign is available to identify with");

	if (callsign.empty() && (m_conf.getFMCallsignAtStart() || m_conf.getFMCallsignAtEnd()))
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_FM, "[FM] Callsign", "the callsign identification is enabled but no callsign is available to send");

	unsigned int accessMode = m_conf.getFMAccessMode();
	if (accessMode > 3U)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_FM, "[FM] AccessMode", "the access mode must be in the range 0 to 3");
	else if (accessMode > 0U && m_conf.getFMCTCSSFrequency() <= 0.0F)
		add(issues, CONF_SEVERITY::FAILURE, CONF_MODE_FM, "[FM] CTCSSFrequency", "the access mode needs CTCSS but no CTCSS frequency has been set");

	if (m_conf.getFMNetworkEnabled())
		validateNetwork(issues, CONF_MODE_FM, "FM Network", m_conf.getFMGatewayAddress(), m_conf.getFMGatewayPort(),
				m_conf.getFMLocalAddress(), m_conf.getFMLocalPort());
}
#endif

void CConfValidator::validatePorts(std::vector<CConfIssue>& issues) const
{
	// Every enabled network binds its own local port. Two of them on the same
	// port means the second one fails to open, so catch the clash here.
	std::vector<std::pair<std::string, unsigned short>> ports;

#if defined(USE_DSTAR)
	if (m_conf.getDStarEnabled() && m_conf.getDStarNetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_DSTAR, m_conf.getDStarLocalPort()));
#endif
#if defined(USE_DMR)
	if (m_conf.getDMREnabled() && m_conf.getDMRNetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_DMR, m_conf.getDMRNetworkLocalPort()));
#endif
#if defined(USE_YSF)
	if (m_conf.getFusionEnabled() && m_conf.getFusionNetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_YSF, m_conf.getFusionNetworkLocalPort()));
#endif
#if defined(USE_P25)
	if (m_conf.getP25Enabled() && m_conf.getP25NetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_P25, m_conf.getP25LocalPort()));
#endif
#if defined(USE_NXDN)
	if (m_conf.getNXDNEnabled() && m_conf.getNXDNNetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_NXDN, m_conf.getNXDNLocalPort()));
#endif
#if defined(USE_M17)
	if (m_conf.getM17Enabled() && m_conf.getM17NetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_M17, m_conf.getM17LocalPort()));
#endif
#if defined(USE_POCSAG)
	if (m_conf.getPOCSAGEnabled() && m_conf.getPOCSAGNetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_POCSAG, m_conf.getPOCSAGLocalPort()));
#endif
#if defined(USE_FM)
	if (m_conf.getFMEnabled() && m_conf.getFMNetworkEnabled())
		ports.push_back(std::make_pair(CONF_MODE_FM, m_conf.getFMLocalPort()));
#endif

	if (m_conf.getTransparentEnabled())
		ports.push_back(std::make_pair(CONF_MODE_GENERAL, m_conf.getTransparentLocalPort()));

	if (m_conf.getModemProtocol() == "udp")
		ports.push_back(std::make_pair(CONF_MODE_MODEM, m_conf.getModemLocalPort()));

	for (size_t i = 0U; i < ports.size(); i++) {
		if (ports[i].second == 0U)
			continue;

		for (size_t j = i + 1U; j < ports.size(); j++) {
			if (ports[i].second == ports[j].second)
				add(issues, CONF_SEVERITY::FAILURE, ports[j].first, "[" + ports[j].first + "] LocalPort",
				    "local port " + std::to_string(ports[j].second) + " is already used by " + ports[i].first);
		}
	}
}
