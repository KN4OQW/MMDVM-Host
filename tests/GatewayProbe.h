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

#if !defined(GatewayProbe_H)
#define	GatewayProbe_H

#include <string>

enum class PROBE_RESULT {
	REPLIED,		// the gateway answered, so it is there and it is talking
	NO_REPLY,		// the datagram was accepted but nothing came back
	UNREACHABLE,		// ICMP said nothing is listening on that port
	RESOLVE_FAILED,		// the gateway address does not resolve
	SEND_FAILED,
	BIND_FAILED		// the local address or port could not be bound
};

struct CProbeReport {
	PROBE_RESULT result;
	std::string  detail;
	unsigned int replyLength;
};

// Sends one datagram to a gateway and waits for an answer.
//
// The socket is connected to the gateway, which is what makes an ICMP port
// unreachable visible here - that is the difference between "the gateway is
// not running" and "the gateway is running but has nothing to say".
//
// Pass a local port of zero to let the system pick one. That is what the
// gateway checks do, so that probing never takes a port away from a host that
// is already running.
extern CProbeReport ProbeGateway(const std::string& gatewayAddress, unsigned short gatewayPort,
				 const std::string& localAddress, unsigned short localPort,
				 const unsigned char* poll, unsigned int pollLength,
				 unsigned int timeoutMs);

// Tries to bind the local address and port a mode's network would use. Used to
// tell "this port is free" from "something else already has it".
enum class BIND_RESULT {
	FREE,
	IN_USE,
	FAILED
};

struct CBindReport {
	BIND_RESULT result;
	std::string detail;
};

extern CBindReport ProbeLocalPort(const std::string& localAddress, unsigned short localPort);

extern std::string ProbeResultName(PROBE_RESULT result);

#endif
