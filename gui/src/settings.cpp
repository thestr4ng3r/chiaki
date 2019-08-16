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

#include <settings.h>

#define SETTINGS_VERSION 1

Settings::Settings(QObject *parent) : QObject(parent)
{
	settings.setValue("version", SETTINGS_VERSION);
	LoadRegisteredHosts();
}

static const QMap<ChiakiVideoResolutionPreset, QString> resolutions = {
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_360p, "360p"},
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_540p, "540p"},
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_720p, "720p"},
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_1080p, "1080p"}
};

static const ChiakiVideoResolutionPreset resolution_default = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;

ChiakiVideoResolutionPreset Settings::GetResolution() const
{
	auto s = settings.value("settings/resolution", resolutions[resolution_default]).toString();
	return resolutions.key(s, resolution_default);
}

void Settings::SetResolution(ChiakiVideoResolutionPreset resolution)
{
	settings.setValue("settings/resolution", resolutions[resolution]);
}

static const QMap<ChiakiVideoFPSPreset, int> fps_values = {
	{ CHIAKI_VIDEO_FPS_PRESET_30, 30 },
	{ CHIAKI_VIDEO_FPS_PRESET_60, 60 }
};

static const ChiakiVideoFPSPreset fps_default = CHIAKI_VIDEO_FPS_PRESET_60;

ChiakiVideoFPSPreset Settings::GetFPS() const
{
	auto v = settings.value("settings/fps", fps_values[fps_default]).toInt();
	return fps_values.key(v, fps_default);
}

void Settings::SetFPS(ChiakiVideoFPSPreset fps)
{
	settings.setValue("settings/fps", fps_values[fps]);
}


void Settings::LoadRegisteredHosts()
{
	registered_hosts.clear();

	int count = settings.beginReadArray("registered_hosts");
	for(int i=0; i<count; i++)
	{
		settings.setArrayIndex(i);
		RegisteredHost host = RegisteredHost::LoadFromSettings(&settings);
		registered_hosts[host.GetPS4MAC()] = host;
	}
	settings.endArray();
}

void Settings::SaveRegisteredHosts()
{
	settings.beginWriteArray("registered_hosts");
	int i=0;
	for(const auto &host : registered_hosts)
	{
		settings.setArrayIndex(i);
		host.SaveToSettings(&settings);
		i++;
	}
	settings.endArray();
}

void Settings::AddRegisteredHost(const RegisteredHost &host)
{
	registered_hosts[host.GetPS4MAC()] = host;
	SaveRegisteredHosts();
	emit RegisteredHostsUpdated();
}

void Settings::RemoveRegisteredHost(const HostMAC &mac)
{
	if(!registered_hosts.contains(mac))
		return;
	registered_hosts.remove(mac);
	SaveRegisteredHosts();
	emit RegisteredHostsUpdated();
}
