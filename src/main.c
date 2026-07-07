#include <stdio.h>
#include <pthread.h>
#include "network.h"
#include "tui.h"

int main(int argc, char *argv[]){
	net_init();
	outline_window();
	printf("NetDet\n");
	return 0;
}
