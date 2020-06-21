
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
	atexit(quit);
	return 0;
}

