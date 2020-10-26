// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_EXCEPTION_H
#define CHIAKI_EXCEPTION_H

#include <exception>

#include <QString>

class Exception : public std::exception
{
	private:
		QByteArray msg;

	public:
		explicit Exception(const QString &msg) : msg(msg.toLocal8Bit()) {}
		const char *what() const noexcept override { return msg.constData(); }
};

#endif // CHIAKI_EXCEPTION_H
