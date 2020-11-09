// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_EXCEPTION_H
#define CHIAKI_EXCEPTION_H

#include <exception>

#include <string>

class Exception : public std::exception
{
	private:
		const char *msg;

	public:
		explicit Exception(const char *msg) : msg(msg) {}
		const char *what() const noexcept override { return msg; }
};

#endif // CHIAKI_EXCEPTION_H
