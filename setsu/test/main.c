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

struct {
	bool down;
	unsigned int tracking_id;
	unsigned int x, y;
} touches[TOUCHES_MAX];

bool dirty = false;

void quit()
{
	setsu_free(setsu);
}

void print_state()
{
	for(size_t i=0; i<TOUCHES_MAX; i++)
	{
		if(touches[i].down)
		{
//			printf("%8u, %8u\n", touches[i].x, touches[i].y);
		}
	}

#define SCALE 16
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
//	printf("%zu, %zu\n", i, sizeof(buf));
	assert(i == sizeof(buf));
	printf("\033[2J%s", buf);

	fflush(stdout);
}

void event(SetsuEvent *event, void *user)
{
	dirty = true;
	switch(event->type)
	{
		case SETSU_EVENT_DEVICE_ADDED:
			setsu_connect(setsu, event->path);
			break;
		case SETSU_EVENT_DEVICE_REMOVED:
			break;
		case SETSU_EVENT_DOWN:
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

int main()
{
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
		if(dirty)
			print_state();
		dirty = false;
		setsu_poll(setsu, event, NULL);
	}
	atexit(quit);
	return 0;
}

