/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CHIAKI_HOST_H
#define CHIAKI_HOST_H

#include <QMetaType>
#include <QString>

#include <chiaki/regist.h>

class HostMAC
{
	private:
		uint8_t mac[6];

	public:
		HostMAC()								{ memset(mac, 0, sizeof(mac)); }
		explicit HostMAC(const uint8_t mac[6])	{ memcpy(this->mac, mac, sizeof(this->mac)); }
		const uint8_t *GetMAC() const			{ return mac; }
		QString ToString() const				{ return QByteArray((const char *)mac, sizeof(mac)).toHex(); }
};

static bool operator==(const HostMAC &a, const HostMAC &b) { return memcmp(a.GetMAC(), b.GetMAC(), 6) == 0; }

class RegisteredHost
{
	private:
		QString ap_ssid;
		QString ap_bssid;
		QString ap_key;
		QString ap_name;
		HostMAC ps4_mac;
		QString ps4_nickname;
		char rp_regist_key[CHIAKI_SESSION_AUTH_SIZE];
		uint32_t rp_key_type;
		uint8_t rp_key[0x10];

	public:
		RegisteredHost();
		RegisteredHost(const ChiakiRegisteredHost &chiaki_host);

		const QString &GetPS4Nickname() const	{ return ps4_nickname; }
};

Q_DECLARE_METATYPE(RegisteredHost)

#endif //CHIAKI_HOST_H
