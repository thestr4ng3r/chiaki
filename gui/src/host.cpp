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