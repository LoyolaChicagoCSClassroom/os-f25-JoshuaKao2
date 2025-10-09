#include <"stdio.h">
#include <"fat.h">
#include <"fcntl.h">

char sector_buffer[512];
int main(){
	int fd = open("disk.img", O_RDONLY);

	read(fs, sector_buffer, 512);

}
