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

#include <host.h>

#include <QSettings>

RegisteredHost::RegisteredHost()
{
	memset(rp_regist_key, 0, sizeof(rp_regist_key));
	memset(rp_key, 0, sizeof(rp_key));
}

RegisteredHost::RegisteredHost(const ChiakiRegisteredHost &chiaki_host)
	: ps4_mac(chiaki_host.ps4_mac)
{
	ap_ssid = chiaki_host.ap_ssid;
	ap_bssid = chiaki_host.ap_bssid;
	ap_key = chiaki_host.ap_key;
	ap_name = chiaki_host.ap_name;
	ps4_nickname = chiaki_host.ps4_nickname;
	memcpy(rp_regist_key, chiaki_host.rp_regist_key, sizeof(rp_regist_key));
	rp_key_type = chiaki_host.rp_key_type;
	memcpy(rp_key, chiaki_host.rp_key, sizeof(rp_key));
}

void RegisteredHost::SaveToSettings(QSettings *settings)
{
	settings->setValue("ap_ssid", ap_ssid);
	settings->setValue("ap_bssid", ap_bssid);
	settings->setValue("ap_key", ap_key);
	settings->setValue("ap_name", ap_name);
	settings->setValue("ps4_nickname", ps4_nickname);
	settings->setValue("rp_regist_key", QByteArray(rp_regist_key, sizeof(rp_regist_key)));
	settings->setValue("rp_key_type", rp_key_type);
	settings->setValue("rp_key", QByteArray((const char *)rp_key, sizeof(rp_key)));
}

RegisteredHost RegisteredHost::LoadFromSettings(QSettings *settings)
{
	RegisteredHost r;
	r.ap_ssid = settings->value("ap_ssid").toString();
	r.ap_bssid = settings->value("ap_bssid").toString();
	r.ap_key = settings->value("ap_key").toString();
	r.ap_name = settings->value("ap_name").toString();
	r.ps4_nickname = settings->value("ps4_nickname").toString();
	auto rp_regist_key = settings->value("rp_regist_key").toByteArray();
	if(rp_regist_key.size() == sizeof(r.rp_regist_key))
		memcpy(r.rp_regist_key, rp_regist_key.constData(), sizeof(r.rp_regist_key));
	r.rp_key_type = settings->value("rp_key_type").toUInt();
	auto rp_key = settings->value("rp_key").toByteArray();
	if(rp_key.size() == sizeof(r.rp_key))
		memcpy(r.rp_key, rp_key.constData(), sizeof(r.rp_key));
	return r;
}
