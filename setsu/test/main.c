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

SetsuCtx *ctx;

void quit()
{
	setsu_ctx_free(ctx);
}

int main()
{
	ctx = setsu_ctx_new();
	if(!ctx)
	{
		printf("Failed to init setsu\n");
		return 1;
	}
	setsu_ctx_run(ctx);
	atexit(quit);
	return 0;
}

