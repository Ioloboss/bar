#include <wayland-client.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "xdg-shell-client-protocol.h"

#define BAR_IMAGE_SHEET_LOCATION "/home/harrison/documents/programs/bar/result/bin/barImageSheet.rgb"

#define TRANSPARENT_COLOR_R (char) 0xf2
#define TRANSPARENT_COLOR_G (char) 0x00
#define TRANSPARENT_COLOR_B (char) 0xff

#define BATTERY_FOREGROUND_R (char) 0xa1
#define BATTERY_FOREGROUND_G (char) 0x81
#define BATTERY_FOREGROUND_B (char) 0x58

//#define debug

struct wl_compositor* comp;
struct wl_surface* srfc;
struct wl_buffer* bfr;
struct wl_shm* shm;
struct xdg_wm_base* sh;
struct xdg_toplevel* top;

uint8_t* pixl;
uint16_t w = 2880 - 16;
uint16_t h = 24;
int BIW = 240;
int BIH = 96;
char* barImages;
void copyImage (int sheetX, int sheetY, int barX);
void writeNumber (int number, int barX);
void copyImageTransparent (int sheetX, int sheetY, int barX);
char* time;


int32_t alc_shm(uint64_t sz) {
	int8_t  name[8];
	name[0] = '/';
	name[7] = 0;
	for (uint8_t i = 1; i < 6; i++) {
		name[i] = (rand() & 23) + 97;
	}

	int32_t fd = shm_open((const char*) name, O_RDWR | O_CREAT | O_EXCL, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
	shm_unlink((const char*) name);
	ftruncate(fd, sz);

	return fd;
}

void resz() {
	int32_t fd = alc_shm(w * h * 4);

	pixl = (uint8_t*) mmap(0, w * h * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool* pool = wl_shm_create_pool(shm, fd, w * h * 4);
	bfr = wl_shm_pool_create_buffer(pool, 0, w, h, w * 4, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
}

int getActiveWorkspace() {
	char bashCommand[256] = "hyprctl activeworkspace | head -n 1 | awk '{print $3}'";
	char bashOutputBuffer[1000];
	FILE *pipe;
	int len;

	pipe = popen(bashCommand, "r");

	if (!pipe) {
		perror("pipe");
		exit(1);
	}

	fgets(bashOutputBuffer, sizeof(bashOutputBuffer), pipe);

	len = strlen(bashOutputBuffer);
	bashOutputBuffer[len-1] = '\0';
	pclose(pipe);
	int value = (int) (bashOutputBuffer[0] - '0');
	return value;
}

int getBatteryLevel() {
	char bashCommand[256] = "cat /sys/class/power_supply/BAT0/capacity";
	char bashOutputBuffer[1000];
	FILE *pipe;
	int len;

	pipe = popen(bashCommand, "r");

	if (!pipe) {
		perror("pipe");
		exit(1);
	}

	fgets(bashOutputBuffer, sizeof(bashOutputBuffer), pipe);

	len = strlen(bashOutputBuffer);
	bashOutputBuffer[len-1] = '\0';
	pclose(pipe);
	int value = atoi(bashOutputBuffer);
	return value;
}

char getBatteryStatus() {
	char bashCommand[256] = "cat /sys/class/power_supply/BAT0/status";
	char bashOutputBuffer[1000];
	FILE *pipe;
	int len;

	pipe = popen(bashCommand, "r");

	if (!pipe) {
		perror("pipe");
		exit(1);
	}

	fgets(bashOutputBuffer, sizeof(bashOutputBuffer), pipe);

	len = strlen(bashOutputBuffer);
	bashOutputBuffer[len-1] = '\0';
	pclose(pipe);
	int value = bashOutputBuffer[0];
	return value;
}

char getNetworkStatus() {
	char bashCommand[256] = "cat /sys/class/net/wlp1s0/operstate";
	char bashOutputBuffer[1000];
	FILE *pipe;
	int len;

	pipe = popen(bashCommand, "r");

	if (!pipe) {
		perror("pipe");
		exit(1);
	}

	fgets(bashOutputBuffer, sizeof(bashOutputBuffer), pipe);

	len = strlen(bashOutputBuffer);
	bashOutputBuffer[len-1] = '\0';
	pclose(pipe);
	int value = bashOutputBuffer[0];
	return value;
}

void updateTime(char* time) {
	char bashCommand[256] = "date '+%H%M'";
	char bashOutputBuffer[1000];
	FILE *pipe;
	int len;

	pipe = popen(bashCommand, "r");

	if (!pipe) {
		perror("pipe");
		exit(1);
	}

	fgets(bashOutputBuffer, sizeof(bashOutputBuffer), pipe);

	len = strlen(bashOutputBuffer);
	bashOutputBuffer[len-1] = '\0';
	pclose(pipe);
	time[0] = bashOutputBuffer[0];
	time[1] = bashOutputBuffer[1];
	time[2] = bashOutputBuffer[2];
	time[3] = bashOutputBuffer[3];
	return;
}

void drawBatteryForeground(int batteryLevel, int beginningX) {
	int adjustedBatteryLevel = (batteryLevel * 16) / 100;
	int beginningY = 20 - adjustedBatteryLevel;
	for (int x = 0; x < 24; x++) {
		for (int y = beginningY; y < 20; y++) {
			pixl[(((y * w) + (beginningX + x)) * 4) + 0] = BATTERY_FOREGROUND_B;
			pixl[(((y * w) + (beginningX + x)) * 4) + 1] = BATTERY_FOREGROUND_G;
			pixl[(((y * w) + (beginningX + x)) * 4) + 2] = BATTERY_FOREGROUND_R;
			pixl[(((y * w) + (beginningX + x)) * 4) + 3] = 0xff;
		}
	}
}

void drawGradient(int width) {
	for (int x = 0; x < width; x++) {
		float r = (((float) (49 - 26) * (float) (width - x)) / width) + 26; // 161
		float g = (((float) (49 - 26) * (float) (width - x)) / width) + 26; // 129
		float b = (((float) (80  - 26) * (float) (width - x)) / width) + 26; // 88
		for (int y = 0; y < 24; y++) {
			pixl[(((y * w) + (x)) * 4) + 0] = b;
			pixl[(((y * w) + (x)) * 4) + 1] = g;
			pixl[(((y * w) + (x)) * 4) + 2] = r;
			pixl[(((y * w) + (x)) * 4) + 3] = 0xff;
		}
	}
}

void blockColour(int beginningX, int width, unsigned char r, unsigned char g, unsigned char b) {
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < 24; y++) {
			pixl[(((y * w) + (beginningX + x)) * 4) + 0] = b;
			pixl[(((y * w) + (beginningX + x)) * 4) + 1] = g;
			pixl[(((y * w) + (beginningX + x)) * 4) + 2] = r;
			pixl[(((y * w) + (beginningX + x)) * 4) + 3] = 0xff;
		}
	}
}

void draw() {
	#ifdef debug
	printf("Draw Called\n");
	#endif

	//drawGradient(2640);
	blockColour(0, w, 26, 26, 26);

	/*
	for (int i = 0; i < 59; i++) {
    		for (int tile = 0; tile < 2; tile++) {
			copyImageTransparent(tile, 0, (tile * 24) + (48 * i));
    		}
	}
	*/

	//blockColour(2640, 224, 26, 26, 26);
	
	int activeworkspace = getActiveWorkspace();
	#ifdef debug
	printf("Active Workspace is %d\n", activeworkspace);
	#endif

	updateTime(time);

	writeNumber(time[3] - '0', w - ((10 * 1) + (12 * 1)));
	writeNumber(time[2] - '0', w - ((10 * 1) + (12 * 2)));
	writeNumber(-2,  w - ((10 * 1) + (12 * 3)));
	writeNumber(time[1] - '0', w - ((10 * 1) + (12 * 4)));
	writeNumber(time[0] - '0', w - ((10 * 1) + (12 * 5)));
	int batteryLevel = getBatteryLevel();
	char batteryStatus = getBatteryStatus();

	drawBatteryForeground(batteryLevel, w - ((10 * 2) + (12 * 7)));

	if (batteryStatus == 'C')
    		copyImageTransparent(1, 3, w - ((10 * 2) + (12 * 7)));
	else if (batteryLevel > 30)
		copyImageTransparent(2, 3, w - ((10 * 2) + (12 * 7)));
	else if (batteryLevel > 10)
		copyImageTransparent(3, 3, w - ((10 * 2) + (12 * 7)));
	else if (batteryLevel > 5)
		copyImageTransparent(4, 3, w - ((10 * 2) + (12 * 7)));
	else
		copyImageTransparent(5, 3, w - ((10 * 2) + (12 * 7)));

	char networkStatus = getNetworkStatus();

	if (networkStatus == 'u')
    		copyImageTransparent(6, 2, w - ((10 * 3) + (12 * 9)));
	else
    		copyImageTransparent(7, 2, w - ((10 * 3) + (12 * 9)));

	copyImageTransparent(activeworkspace % 10, 1, w - ((10 * 4) + (12 * 11)));

	
	wl_surface_attach(srfc, bfr, 0, 0);
	wl_surface_damage_buffer(srfc, 0, 0, w, h);
	wl_surface_commit(srfc);

}

struct wl_callback_listener cb_list;

void frame_new(void* data, struct wl_callback* cb, uint32_t a) {
	wl_callback_destroy(cb);
	cb = wl_surface_frame(srfc);
	wl_callback_add_listener(cb, &cb_list, 0);
	
	draw();
}

void xrfc_conf(void* data, struct xdg_surface* xrfc, uint32_t ser) {
	xdg_surface_ack_configure(xrfc, ser);
	if (!pixl) {
		resz();
	}

	draw();
}

struct wl_callback_listener cb_list = {
	.done = frame_new
};

struct xdg_surface_listener xrfc_list = {
	.configure = xrfc_conf
};

void top_conf(void* data, struct xdg_toplevel* top, int32_t w, int32_t h, struct wl_array* stat) {
	
}

static void top_cls(void* data, struct xdg_toplevel* top) {
	
}

struct xdg_toplevel_listener top_list = {
	.configure = top_conf,
	.close = top_cls
};

void sh_ping(void* data, struct xdg_wm_base* sh, uint32_t ser) {
	xdg_wm_base_pong(sh, ser);
}

struct xdg_wm_base_listener sh_list = {
	.ping = sh_ping
};

void reg_glob(void* data, struct wl_registry* reg, uint32_t name, const char* intf, uint32_t v) {
	if (!strcmp(intf, wl_compositor_interface.name)) {
		comp = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
	}
	else if (!strcmp(intf, wl_shm_interface.name)) {
		shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
	}
	else if (!strcmp(intf, xdg_wm_base_interface.name)) {
		sh = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(sh, &sh_list, 0);
	}
}

void reg_glob_rem(void* data, struct wl_registry* reg, uint32_t name) {

}

struct wl_registry_listener reg_list = {
	.global = reg_glob,
	.global_remove = reg_glob_rem
};

void readGraphics() {
	FILE *fp;
	long lSize;
	fp = fopen(BAR_IMAGE_SHEET_LOCATION, "rb");
	if (!fp) perror(BAR_IMAGE_SHEET_LOCATION),exit(1);

	fseek(fp, 0L, SEEK_END);
	lSize = ftell(fp);
	rewind(fp);

	barImages = malloc(lSize);
	if (!barImages) fclose(fp), fputs("memory alloc fails",stderr),exit(1);

	if (1!=fread(barImages, lSize, 1, fp))
    		fclose(fp),free(barImages),fputs("entire read fails",stderr),exit(1);
	fclose(fp);
}

void copyImage (int sheetX, int sheetY, int barX) {
	int adjustedSheetX = sheetX * 24;
	int adjustedSheetY = sheetY * 24;
	for (int x = 0; x < 24; x++) {
		for (int y = 0; y < 24; y++) {
    			int imagesPos = (((adjustedSheetY + y) * BIW) + (adjustedSheetX + x)) * 3;
			pixl[(((y * w) + (barX + x)) * 4) + 0] = barImages[imagesPos + 2];
			pixl[(((y * w) + (barX + x)) * 4) + 1] = barImages[imagesPos + 1];
			pixl[(((y * w) + (barX + x)) * 4) + 2] = barImages[imagesPos + 0];
			pixl[(((y * w) + (barX + x)) * 4) + 3] = 0xff;
		}
	}
}

void writeNumber (int number, int barX) {
	int adjustedSheetX = (number + 2) * 12;
	int adjustedSheetY = 2 * 24;
	for (int x = 0; x < 12; x++) {
		for (int y = 0; y < 24; y++) {
    			int imagesPos = (((adjustedSheetY + y) * BIW) + (adjustedSheetX + x)) * 3;
    			char inputR = barImages[imagesPos + 0];
    			char inputG = barImages[imagesPos + 1];
    			char inputB = barImages[imagesPos + 2];
    			if (!((inputR == TRANSPARENT_COLOR_R) && (inputG == TRANSPARENT_COLOR_G) && (inputB == TRANSPARENT_COLOR_B))) {
				pixl[(((y * w) + (barX + x)) * 4) + 0] = inputB;
				pixl[(((y * w) + (barX + x)) * 4) + 1] = inputG;
				pixl[(((y * w) + (barX + x)) * 4) + 2] = inputR;
				pixl[(((y * w) + (barX + x)) * 4) + 3] = 0xff;
    			}
		}
	}
}

void copyImageTransparent (int sheetX, int sheetY, int barX) {
	int adjustedSheetX = sheetX * 24;
	int adjustedSheetY = sheetY * 24;
	for (int x = 0; x < 24; x++) {
		for (int y = 0; y < 24; y++) {
    			int imagesPos = (((adjustedSheetY + y) * BIW) + (adjustedSheetX + x)) * 3;
    			char inputR = barImages[imagesPos + 0];
    			char inputG = barImages[imagesPos + 1];
    			char inputB = barImages[imagesPos + 2];
    			if (!((inputR == TRANSPARENT_COLOR_R) && (inputG == TRANSPARENT_COLOR_G) && (inputB == TRANSPARENT_COLOR_B))) {
				pixl[(((y * w) + (barX + x)) * 4) + 0] = inputB;
				pixl[(((y * w) + (barX + x)) * 4) + 1] = inputG;
				pixl[(((y * w) + (barX + x)) * 4) + 2] = inputR;
				pixl[(((y * w) + (barX + x)) * 4) + 3] = 0xff;
    			}
		}
	}
}

int main() {
	readGraphics();
	time = malloc(4);
    	struct wl_display* disp = wl_display_connect(0);
    	if (!disp)
        	return -1;
    	struct wl_registry* reg = wl_display_get_registry(disp);
	wl_registry_add_listener(reg, &reg_list, 0);
	wl_display_roundtrip(disp);

	srfc = wl_compositor_create_surface(comp);
	struct wl_callback* cb = wl_surface_frame(srfc);
	wl_callback_add_listener(cb, &cb_list, 0);

	struct xdg_surface* xrfc = xdg_wm_base_get_xdg_surface(sh, srfc);
	xdg_surface_add_listener(xrfc, &xrfc_list, 0);
	top = xdg_surface_get_toplevel(xrfc);
	xdg_toplevel_add_listener(top, &top_list, 0);
	xdg_toplevel_set_title(top, "status bar");
	xdg_toplevel_set_app_id(top, "status_bar");
	wl_surface_commit(srfc);

	while (wl_display_dispatch(disp)) {
    		usleep(100000);
	}

	if (bfr) {
		wl_buffer_destroy(bfr);
	}
	xdg_toplevel_destroy(top);
	xdg_surface_destroy(xrfc);
	wl_surface_destroy(srfc);    	
	wl_display_disconnect(disp);
	free(barImages);
	free(time);
	return 0;
}
