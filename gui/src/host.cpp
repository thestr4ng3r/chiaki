// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#include <host.h>

#include <QSettings>

RegisteredHost::RegisteredHost()
{
	memset(rp_regist_key, 0, sizeof(rp_regist_key));
	memset(rp_key, 0, sizeof(rp_key));
}

RegisteredHost::RegisteredHost(const RegisteredHost &o)
	: ap_ssid(o.ap_ssid),
	ap_bssid(o.ap_bssid),
	ap_key(o.ap_key),
	ap_name(o.ap_name),
	ps4_mac(o.ps4_mac),
	ps4_nickname(o.ps4_nickname),
	rp_key_type(o.rp_key_type)
{
	memcpy(rp_regist_key, o.rp_regist_key, sizeof(rp_regist_key));
	memcpy(rp_key, o.rp_key, sizeof(rp_key));
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

void RegisteredHost::SaveToSettings(QSettings *settings) const
{
	settings->setValue("ap_ssid", ap_ssid);
	settings->setValue("ap_bssid", ap_bssid);
	settings->setValue("ap_key", ap_key);
	settings->setValue("ap_name", ap_name);
	settings->setValue("ps4_nickname", ps4_nickname);
	settings->setValue("ps4_mac", QByteArray((const char *)ps4_mac.GetMAC(), 6));
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
	auto ps4_mac = settings->value("ps4_mac").toByteArray();
	if(ps4_mac.size() == 6)
		r.ps4_mac = HostMAC((const uint8_t *)ps4_mac.constData());
	auto rp_regist_key = settings->value("rp_regist_key").toByteArray();
	if(rp_regist_key.size() == sizeof(r.rp_regist_key))
		memcpy(r.rp_regist_key, rp_regist_key.constData(), sizeof(r.rp_regist_key));
	r.rp_key_type = settings->value("rp_key_type").toUInt();
	auto rp_key = settings->value("rp_key").toByteArray();
	if(rp_key.size() == sizeof(r.rp_key))
		memcpy(r.rp_key, rp_key.constData(), sizeof(r.rp_key));
	return r;
}

ManualHost::ManualHost()
{
	id = -1;
	registered = false;
}

ManualHost::ManualHost(int id, const QString &host, bool registered, const HostMAC &registered_mac)
	: id(id),
	host(host),
	registered(registered),
	registered_mac(registered_mac)
{
}

ManualHost::ManualHost(int id, const ManualHost &o)
	: id(id),
	host(o.host),
	registered(o.registered),
	registered_mac(o.registered_mac)
{
}

void ManualHost::SaveToSettings(QSettings *settings) const
{
	settings->setValue("id", id);
	settings->setValue("host", host);
	settings->setValue("registered", registered);
	settings->setValue("registered_mac", QByteArray((const char *)registered_mac.GetMAC(), 6));
}

ManualHost ManualHost::LoadFromSettings(QSettings *settings)
{
	ManualHost r;
	r.id = settings->value("id", -1).toInt();
	r.host = settings->value("host").toString();
	r.registered = settings->value("registered").toBool();
	auto registered_mac = settings->value("registered_mac").toByteArray();
	if(registered_mac.size() == 6)
		r.registered_mac = HostMAC((const uint8_t *)registered_mac.constData());
	return r;
}
