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

#include <setsu.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

Setsu *setsu;

#define WIDTH 1920
#define HEIGHT 942
#define TOUCHES_MAX 8
#define SCALE 16

struct {
	bool down;
	unsigned int tracking_id;
	unsigned int x, y;
} touches[TOUCHES_MAX];

bool dirty = false;
bool log_mode;

#define LOG(...) do { if(log_mode) fprintf(stderr, __VA_ARGS__); } while(0)

void quit()
{
	setsu_free(setsu);
}

void print_state()
{
	char buf[(1 + WIDTH/SCALE)*(HEIGHT/SCALE) + 1];
	size_t i = 0;
	for(size_t y=0; y<HEIGHT/SCALE; y++)
	{
		for(size_t x=0; x<WIDTH/SCALE; x++)
		{
			for(size_t t=0; t<TOUCHES_MAX; t++)
			{
				if(touches[t].down && touches[t].x / SCALE == x && touches[t].y / SCALE == y)
				{
					buf[i++] = 'X';
					goto beach;
				}
			}
			buf[i++] = '.';
beach:
			continue;
		}
		buf[i++] = '\n';
	}
	buf[i++] = '\0';
	assert(i == sizeof(buf));
	printf("\033[2J%s", buf);

	fflush(stdout);
}

void event(SetsuEvent *event, void *user)
{
	dirty = true;
	switch(event->type)
	{
		case SETSU_EVENT_DEVICE_ADDED: {
			SetsuDevice *dev = setsu_connect(setsu, event->path);
			LOG("Device added: %s, connect %s\n", event->path, dev ? "succeeded" : "FAILED!");
			break;
		}
		case SETSU_EVENT_DEVICE_REMOVED:
			LOG("Device removed: %s\n", event->path);
			break;
		case SETSU_EVENT_DOWN:
			LOG("Down for %s, tracking id %d\n", setsu_device_get_path(event->dev), event->tracking_id);
			for(size_t i=0; i<TOUCHES_MAX; i++)
			{
				if(!touches[i].down)
				{
					touches[i].down = true;
					touches[i].tracking_id = event->tracking_id;
					break;
				}
			}
			break;
		case SETSU_EVENT_POSITION:
		case SETSU_EVENT_UP:
			if(event->type == SETSU_EVENT_UP)
				LOG("Up for %s, tracking id %d\n", setsu_device_get_path(event->dev), event->tracking_id);
			else
				LOG("Position for %s, tracking id %d: %u, %u\n", setsu_device_get_path(event->dev),
						event->tracking_id, (unsigned int)event->x, (unsigned int)event->y);
			for(size_t i=0; i<TOUCHES_MAX; i++)
			{
				if(touches[i].down && touches[i].tracking_id == event->tracking_id)
				{
					switch(event->type)
					{
						case SETSU_EVENT_POSITION:
							touches[i].x = event->x;
							touches[i].y = event->y;
							break;
						case SETSU_EVENT_UP:
							touches[i].down = false;
							break;
						default:
							break;
					}
				}
			}
			break;
	}
}

void usage(const char *prog)
{
	printf("usage: %s [-l]\n  -l log mode\n", prog);
	exit(1);
}

int main(int argc, const char *argv[])
{
	log_mode = false;
	if(argc == 2)
	{
		if(!strcmp(argv[1], "-l"))
			log_mode = true;
		else
			usage(argv[0]);
	}
	else if(argc != 1)
		usage(argv[0]);

	memset(touches, 0, sizeof(touches));
	setsu = setsu_new();
	if(!setsu)
	{
		printf("Failed to init setsu\n");
		return 1;
	}
	dirty = true;
	while(1)
	{
		if(dirty && !log_mode)
			print_state();
		dirty = false;
		setsu_poll(setsu, event, NULL);
	}
	atexit(quit);
	return 0;
}

