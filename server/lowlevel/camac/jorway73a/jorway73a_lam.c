#include <sconf.h>
#include <errorcodes.h>
#include <debug.h>

int get_lam()
	{
	T(get_lam)
	printf("get_lam\n");
	return 0;
	}

errcode init_lam()
	{
	T(init_lam)
	printf("init_lam\n");
	return OK;
	}

errcode done_lam()
	{
	T(done_lam)
	printf("done_lam\n");
	return OK;
	}

int get_lam_mask()
	{
	T(get_lam_mask)
	printf("get_lam_mask\n");
	return 0;
	}

void enable_lam()
	{
	T(enable_lam)
	printf("enable_lam\n");
	}

void disable_lam()
	{
	T(disable_lam)
	printf("disable_lam\n");
	}
