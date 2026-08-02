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

#if !defined(ConfValidator_H)
#define	ConfValidator_H

#include "Conf.h"

#include <string>
#include <vector>

// The mode names used to tag and to select issues. These are also the names
// accepted by the test harness on the command line.
const char* const CONF_MODE_GENERAL = "General";
const char* const CONF_MODE_MODEM   = "Modem";
const char* const CONF_MODE_DSTAR   = "D-Star";
const char* const CONF_MODE_DMR     = "DMR";
const char* const CONF_MODE_YSF     = "YSF";
const char* const CONF_MODE_P25     = "P25";
const char* const CONF_MODE_NXDN    = "NXDN";
const char* const CONF_MODE_M17     = "M17";
const char* const CONF_MODE_POCSAG  = "POCSAG";
const char* const CONF_MODE_FM      = "FM";

enum class CONF_SEVERITY {
	WARNING,		// the host will start, but something is likely to misbehave
	FAILURE			// the host will refuse to start, abort, or be unusable
};

struct CConfIssue {
	CONF_SEVERITY severity;
	std::string   mode;		// one of the CONF_MODE_* names above
	std::string   key;		// the offending item, e.g. "[Modem] RXFrequency"
	std::string   message;
};

// Static checks over a parsed configuration. Nothing here touches the network
// or the modem, so it is safe to run anywhere, including on the bench with a
// live host running.
class CConfValidator {
public:
	explicit CConfValidator(const CConf& conf);
	~CConfValidator();

	// Everything, in mode order.
	std::vector<CConfIssue> validate() const;

	// Just the named mode. The mode's own section and its network section are
	// both tagged with the mode name, so this covers both. "General" and
	// "Modem" are valid mode names here too.
	std::vector<CConfIssue> validate(const std::string& mode) const;

	// The modes compiled into this build, in the order validate() reports them.
	// Excludes "General" and "Modem".
	static std::vector<std::string> modes();

	// Is the named mode turned on in this configuration?
	bool isModeEnabled(const std::string& mode) const;

	// Is the named mode's network section turned on in this configuration?
	bool isNetworkEnabled(const std::string& mode) const;

	static unsigned int count(const std::vector<CConfIssue>& issues, CONF_SEVERITY severity);
	static bool         has(const std::vector<CConfIssue>& issues, CONF_SEVERITY severity, const std::string& key);
	static std::string  format(const CConfIssue& issue);

private:
	const CConf& m_conf;

	void validateGeneral(std::vector<CConfIssue>& issues) const;
	void validateModem(std::vector<CConfIssue>& issues) const;
	void validateDStar(std::vector<CConfIssue>& issues) const;
	void validateDMR(std::vector<CConfIssue>& issues) const;
	void validateYSF(std::vector<CConfIssue>& issues) const;
	void validateP25(std::vector<CConfIssue>& issues) const;
	void validateNXDN(std::vector<CConfIssue>& issues) const;
	void validateM17(std::vector<CConfIssue>& issues) const;
	void validatePOCSAG(std::vector<CConfIssue>& issues) const;
	void validateFM(std::vector<CConfIssue>& issues) const;
	void validatePorts(std::vector<CConfIssue>& issues) const;

	// Shared checks for a mode's [<mode> Network] section.
	void validateNetwork(std::vector<CConfIssue>& issues, const std::string& mode, const std::string& section,
			     const std::string& gatewayAddress, unsigned short gatewayPort,
			     const std::string& localAddress, unsigned short localPort) const;

	bool anyRFModeEnabled() const;
};

#endif
