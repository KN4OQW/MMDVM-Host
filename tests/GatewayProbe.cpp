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

#include "GatewayProbe.h"

#include <cerrno>
#include <cstring>

#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace {

	const unsigned int BUFFER_LENGTH = 500U;

	std::string errorText(int err)
	{
		return std::string(::strerror(err));
	}

	// Resolves an address and port the same way CUDPSocket does.
	bool resolve(const std::string& address, unsigned short port, sockaddr_storage& addr, socklen_t& addrLen, int flags, std::string& detail)
	{
		struct addrinfo hints;
		::memset(&hints, 0, sizeof(hints));
		hints.ai_flags |= AI_NUMERICSERV | flags;

		std::string service = std::to_string(port);

		struct addrinfo* res = nullptr;
		int err = ::getaddrinfo(address.empty() ? nullptr : address.c_str(), service.c_str(), &hints, &res);
		if (err != 0) {
			detail = std::string(::gai_strerror(err));
			return false;
		}

		addrLen = socklen_t(res->ai_addrlen);
		::memcpy(&addr, res->ai_addr, res->ai_addrlen);

		::freeaddrinfo(res);

		return true;
	}

}

std::string ProbeResultName(PROBE_RESULT result)
{
	switch (result) {
	case PROBE_RESULT::REPLIED:
		return "replied";
	case PROBE_RESULT::NO_REPLY:
		return "no reply";
	case PROBE_RESULT::UNREACHABLE:
		return "unreachable";
	case PROBE_RESULT::RESOLVE_FAILED:
		return "address will not resolve";
	case PROBE_RESULT::SEND_FAILED:
		return "send failed";
	case PROBE_RESULT::BIND_FAILED:
		return "local bind failed";
	default:
		return "unknown";
	}
}

CProbeReport ProbeGateway(const std::string& gatewayAddress, unsigned short gatewayPort,
			  const std::string& localAddress, unsigned short localPort,
			  const unsigned char* poll, unsigned int pollLength,
			  unsigned int timeoutMs)
{
	CProbeReport report;
	report.result      = PROBE_RESULT::NO_REPLY;
	report.replyLength = 0U;

	sockaddr_storage gateway;
	socklen_t gatewayLen = 0;
	std::string detail;

	if (!resolve(gatewayAddress, gatewayPort, gateway, gatewayLen, 0, detail)) {
		report.result = PROBE_RESULT::RESOLVE_FAILED;
		report.detail = gatewayAddress + " - " + detail;
		return report;
	}

	int fd = ::socket(gateway.ss_family, SOCK_DGRAM, 0);
	if (fd < 0) {
		report.result = PROBE_RESULT::BIND_FAILED;
		report.detail = "cannot create a socket - " + errorText(errno);
		return report;
	}

	if (!localAddress.empty() || localPort > 0U) {
		sockaddr_storage local;
		socklen_t localLen = 0;

		if (!resolve(localAddress, localPort, local, localLen, AI_PASSIVE, detail)) {
			::close(fd);
			report.result = PROBE_RESULT::BIND_FAILED;
			report.detail = "the local address will not resolve - " + detail;
			return report;
		}

		if (::bind(fd, (sockaddr*)&local, localLen) < 0) {
			int err = errno;
			::close(fd);
			report.result = PROBE_RESULT::BIND_FAILED;
			report.detail = "cannot bind " + (localAddress.empty() ? std::string("*") : localAddress) + ":" + std::to_string(localPort) + " - " + errorText(err);
			return report;
		}
	}

	// Connecting a datagram socket is what makes an ICMP port unreachable
	// visible to us, and it also keeps out replies from anywhere else.
	if (::connect(fd, (sockaddr*)&gateway, gatewayLen) < 0) {
		int err = errno;
		::close(fd);
		report.result = PROBE_RESULT::SEND_FAILED;
		report.detail = "cannot connect to the gateway - " + errorText(err);
		return report;
	}

	ssize_t n = ::send(fd, poll, pollLength, 0);
	if (n < 0) {
		int err = errno;
		::close(fd);
		report.result = err == ECONNREFUSED ? PROBE_RESULT::UNREACHABLE : PROBE_RESULT::SEND_FAILED;
		report.detail = errorText(err);
		return report;
	}

	struct pollfd pfd;
	pfd.fd      = fd;
	pfd.events  = POLLIN;
	pfd.revents = 0;

	int ready = ::poll(&pfd, 1U, int(timeoutMs));
	if (ready < 0) {
		int err = errno;
		::close(fd);
		report.result = PROBE_RESULT::SEND_FAILED;
		report.detail = "poll failed - " + errorText(err);
		return report;
	}

	if (ready == 0) {
		::close(fd);
		report.result = PROBE_RESULT::NO_REPLY;
		report.detail = "nothing came back within " + std::to_string(timeoutMs) + "ms";
		return report;
	}

	unsigned char buffer[BUFFER_LENGTH];

	n = ::recv(fd, buffer, BUFFER_LENGTH, 0);
	if (n < 0) {
		int err = errno;
		::close(fd);

		if (err == ECONNREFUSED) {
			report.result = PROBE_RESULT::UNREACHABLE;
			report.detail = "nothing is listening on " + gatewayAddress + ":" + std::to_string(gatewayPort);
		} else {
			report.result = PROBE_RESULT::SEND_FAILED;
			report.detail = errorText(err);
		}

		return report;
	}

	::close(fd);

	report.result      = PROBE_RESULT::REPLIED;
	report.replyLength = (unsigned int)n;
	report.detail      = std::to_string(n) + " bytes";

	return report;
}

CBindReport ProbeLocalPort(const std::string& localAddress, unsigned short localPort)
{
	CBindReport report;
	report.result = BIND_RESULT::FAILED;

	sockaddr_storage local;
	socklen_t localLen = 0;
	std::string detail;

	if (!resolve(localAddress, localPort, local, localLen, AI_PASSIVE, detail)) {
		report.detail = "the local address will not resolve - " + detail;
		return report;
	}

	int fd = ::socket(local.ss_family, SOCK_DGRAM, 0);
	if (fd < 0) {
		report.detail = "cannot create a socket - " + errorText(errno);
		return report;
	}

	// Deliberately without SO_REUSEADDR, so that a port a running host already
	// holds shows up as being in use rather than quietly binding alongside it.
	if (::bind(fd, (sockaddr*)&local, localLen) < 0) {
		int err = errno;
		::close(fd);

		if (err == EADDRINUSE) {
			report.result = BIND_RESULT::IN_USE;
			report.detail = "port " + std::to_string(localPort) + " is already in use";
		} else {
			report.detail = "cannot bind " + (localAddress.empty() ? std::string("*") : localAddress) + ":" + std::to_string(localPort) + " - " + errorText(err);
		}

		return report;
	}

	::close(fd);

	report.result = BIND_RESULT::FREE;
	report.detail = "port " + std::to_string(localPort) + " is free";

	return report;
}
