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

class QSettings;

class HostMAC
{
	private:
		uint8_t mac[6];

	public:
		HostMAC()								{ memset(mac, 0, sizeof(mac)); }
		HostMAC(const HostMAC &o)				{ memcpy(mac, o.GetMAC(), sizeof(mac)); }
		explicit HostMAC(const uint8_t mac[6])	{ memcpy(this->mac, mac, sizeof(this->mac)); }
		const uint8_t *GetMAC() const			{ return mac; }
		QString ToString() const				{ return QByteArray((const char *)mac, sizeof(mac)).toHex(); }
		uint64_t GetValue() const
		{
			return ((uint64_t)mac[0] << 0x28)
				| ((uint64_t)mac[1] << 0x20)
				| ((uint64_t)mac[2] << 0x18)
				| ((uint64_t)mac[3] << 0x10)
				| ((uint64_t)mac[4] << 0x8)
				| mac[5];
		}
};

static bool operator==(const HostMAC &a, const HostMAC &b)	{ return memcmp(a.GetMAC(), b.GetMAC(), 6) == 0; }
static bool operator<(const HostMAC &a, const HostMAC &b)	{ return a.GetValue() < b.GetValue(); }

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
		RegisteredHost(const RegisteredHost &o);

		RegisteredHost(const ChiakiRegisteredHost &chiaki_host);

		const HostMAC &GetPS4MAC() const 		{ return ps4_mac; }
		const QString &GetPS4Nickname() const	{ return ps4_nickname; }
		const QByteArray GetRPRegistKey() const	{ return QByteArray(rp_regist_key, sizeof(rp_regist_key)); }
		const QByteArray GetRPKey() const		{ return QByteArray((const char *)rp_key, sizeof(rp_key)); }

		void SaveToSettings(QSettings *settings) const;
		static RegisteredHost LoadFromSettings(QSettings *settings);
};

Q_DECLARE_METATYPE(RegisteredHost)
Q_DECLARE_METATYPE(HostMAC)

#endif //CHIAKI_HOST_H
