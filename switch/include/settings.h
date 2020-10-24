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

#ifndef CHIAKI_SETTINGS_H
#define CHIAKI_SETTINGS_H

#include <regex>

#include "host.h"
#include <chiaki/log.h>

// mutual host and settings
class Host;

class Settings
{
	private:
		ChiakiLog * log;
		const char * filename = "chiaki.conf";

		std::map<std::string, Host> *hosts;
		// global_settings from psedo INI file
		ChiakiVideoResolutionPreset global_video_resolution = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
		ChiakiVideoFPSPreset global_video_fps = CHIAKI_VIDEO_FPS_PRESET_60;
		std::string global_psn_online_id = "";
		std::string global_psn_account_id = "";

		typedef enum configurationitem
		{
			UNKNOWN,
			HOST_NAME,
			HOST_IP,
			PSN_ONLINE_ID,
			PSN_ACCOUNT_ID,
			RP_KEY,
			RP_KEY_TYPE,
			RP_REGIST_KEY,
			VIDEO_RESOLUTION,
			VIDEO_FPS,
		} ConfigurationItem;

		// dummy parser implementation
		// the aim is not to have bulletproof parser
		// the goal is to read/write inernal flat configuration file
		const std::map<Settings::ConfigurationItem, std::regex> re_map = {
			{ HOST_NAME, std::regex("^\\[\\s*(.+)\\s*\\]") },
			{ HOST_IP, std::regex("^\\s*host_ip\\s*=\\s*\"?(\\d+\\.\\d+\\.\\d+\\.\\d+)\"?") },
			{ PSN_ONLINE_ID, std::regex("^\\s*psn_online_id\\s*=\\s*\"?(\\w+)\"?") },
			{ PSN_ACCOUNT_ID, std::regex("^\\s*psn_account_id\\s*=\\s*\"?([\\w/=+]+)\"?") },
			{ RP_KEY, std::regex("^\\s*rp_key\\s*=\\s*\"?([\\w/=+]+)\"?") },
			{ RP_KEY_TYPE, std::regex("^\\s*rp_key_type\\s*=\\s*\"?(\\d)\"?") },
			{ RP_REGIST_KEY, std::regex("^\\s*rp_regist_key\\s*=\\s*\"?([\\w/=+]+)\"?") },
			{ VIDEO_RESOLUTION, std::regex("^\\s*video_resolution\\s*=\\s*\"?(1080p|720p|540p|360p)\"?") },
			{ VIDEO_FPS, std::regex("^\\s*video_fps\\s*=\\s*\"?(60|30)\"?") },
		};

		ConfigurationItem ParseLine(std::string * line, std::string * value);
		size_t GetB64encodeSize(size_t);
	public:
		Settings(ChiakiLog * log, std::map<std::string, Host> * hosts): log(log), hosts(hosts){};
		Host * GetOrCreateHost(std::string * host_name);
		ChiakiLog* GetLogger();
		std::string GetPSNOnlineID(Host * host);
		std::string GetPSNAccountID(Host * host);
		void SetPSNOnlineID(Host * host, std::string psn_online_id);
		void SetPSNAccountID(Host * host, std::string psn_account_id);
		ChiakiVideoResolutionPreset GetVideoResolution(Host * host);
		ChiakiVideoFPSPreset GetVideoFPS(Host * host);
		std::string ResolutionPresetToString(ChiakiVideoResolutionPreset resolution);
		std::string FPSPresetToString(ChiakiVideoFPSPreset fps);
		ChiakiVideoResolutionPreset StringToResolutionPreset(std::string value);
		ChiakiVideoFPSPreset StringToFPSPreset(std::string value);
		int ResolutionPresetToInt(ChiakiVideoResolutionPreset resolution);
		int FPSPresetToInt(ChiakiVideoFPSPreset fps);
		void SetVideoResolution(Host * host, ChiakiVideoResolutionPreset value);
		void SetVideoFPS(Host * host, ChiakiVideoFPSPreset value);
		void SetVideoResolution(Host * host, std::string value);
		void SetVideoFPS(Host * host, std::string value);
		std::string GetHostIPAddr(Host * host);
		std::string GetHostName(Host * host);
		bool SetHostRPKeyType(Host * host, std::string value);
		int GetHostRPKeyType(Host * host);
		std::string GetHostRPKey(Host * host);
		std::string GetHostRPRegistKey(Host * host);
		bool SetHostRPKey(Host * host, std::string rp_key_b64);
		bool SetHostRPRegistKey(Host * host, std::string rp_regist_key_b64);
		void ParseFile();
		int WriteFile();
};

#endif // CHIAKI_SETTINGS_H
