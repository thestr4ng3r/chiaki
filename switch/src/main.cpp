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

// chiaki modules
#include <chiaki/log.h>
#include <chiaki/discovery.h>

// discover and wakeup ps4 host
// from local network
#include "discoverymanager.h"
#include "settings.h"
#include "io.h"
#include "gui.h"

#ifdef __SWITCH__
#include <switch.h>
#else
bool appletMainLoop()
{
	return true;
}
#endif

#ifndef CHIAKI_SWITCH_ENABLE_LINUX
#define CHIAKI_ENABLE_SWITCH_NXLINK 1
#endif

#ifdef __SWITCH__
// use a custom nintendo switch socket config
// chiaki requiers many threads with udp/tcp sockets
static const SocketInitConfig g_chiakiSocketInitConfig = {
	.bsdsockets_version = 1,

	.tcp_tx_buf_size = 0x8000,
	.tcp_rx_buf_size = 0x10000,
	.tcp_tx_buf_max_size = 0x40000,
	.tcp_rx_buf_max_size = 0x40000,

	.udp_tx_buf_size = 0x40000,
	.udp_rx_buf_size = 0x40000,

	.sb_efficiency = 8,

	.num_bsd_sessions = 16,
	.bsd_service_type = BsdServiceType_User,
};
#endif // __SWITCH__

#ifdef CHIAKI_ENABLE_SWITCH_NXLINK
static int s_nxlinkSock = -1;

static void initNxLink()
{
	// use chiaki socket config initialization
	if (R_FAILED(socketInitialize(&g_chiakiSocketInitConfig)))
		return;

	s_nxlinkSock = nxlinkStdio();
	if (s_nxlinkSock >= 0)
		printf("initNxLink");
	else
		socketExit();
}

static void deinitNxLink()
{
	if (s_nxlinkSock >= 0)
	{
		close(s_nxlinkSock);
		s_nxlinkSock = -1;
	}
}
#endif // CHIAKI_ENABLE_SWITCH_NXLINK

#ifdef __SWITCH__
extern "C" void userAppInit()
{
#ifdef CHIAKI_ENABLE_SWITCH_NXLINK
	initNxLink();
#endif
	// to load gui resources
	romfsInit();
	plInitialize(PlServiceType_User);
	// load socket custom config
	socketInitialize(&g_chiakiSocketInitConfig);
	setsysInitialize();
}

extern "C" void userAppExit()
{
#ifdef CHIAKI_ENABLE_SWITCH_NXLINK
	deinitNxLink();
#endif // CHIAKI_ENABLE_SWITCH_NXLINK
	socketExit();
	/* Cleanup tesla required services. */
	hidsysExit();
	pmdmntExit();
	plExit();

	/* Cleanup default services. */
	fsExit();
	hidExit();
	appletExit();
	setsysExit();
	smExit();
}
#endif // __SWITCH__

int main(int argc, char* argv[])
{
	// init chiaki lib
	ChiakiLog log;
#if defined(CHIAKI_ENABLE_SWITCH_NXLINK) || defined(CHIAKI_SWITCH_ENABLE_LINUX)
#ifdef __SWITCH__
	chiaki_log_init(&log, CHIAKI_LOG_ALL ^ CHIAKI_LOG_VERBOSE, chiaki_log_cb_print, NULL);
#else
	chiaki_log_init(&log, CHIAKI_LOG_ALL, chiaki_log_cb_print, NULL);
#endif
#else
	// null log for switch version
	chiaki_log_init(&log, 0, chiaki_log_cb_print, NULL);
#endif

	// load chiaki lib
	CHIAKI_LOGI(&log, "Loading chaki lib");

	ChiakiErrorCode err = chiaki_lib_init();
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&log, "Chiaki lib init failed: %s\n", chiaki_error_string(err));
		return 1;
	}

	CHIAKI_LOGI(&log, "Loading SDL audio / joystick");
	if(SDL_Init( SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ))
	{
		CHIAKI_LOGE(&log, "SDL initialization failed: %s", SDL_GetError());
		return 1;
	}

	// build sdl OpenGl and AV decoders graphical interface
	IO io = IO(&log); // open Input Output class

	// manage ps4 setting discovery wakeup and registration
	std::map<std::string, Host> hosts;
	// create host objects form config file
	Settings settings = Settings(&log, &hosts);
	CHIAKI_LOGI(&log, "Read chiaki settings file");
	// FIXME use GUI for config
	settings.ParseFile();
	Host * host = nullptr;

	DiscoveryManager discoverymanager = DiscoveryManager(&settings);
	MainApplication app = MainApplication(&hosts, &settings, &discoverymanager, &io, &log);
	app.Load();

	CHIAKI_LOGI(&log, "Quit applet");
	SDL_Quit();
	return 0;
}

