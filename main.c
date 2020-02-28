#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#define _CRT_SECURE_NO_WARNINGS

#define VERSION "0.31"

#define COLOR_BLACK			0x00 // RGB(0x00, 0x00, 0x00)
#define COLOR_WHITE			0x01 // RGB(0xff, 0xff, 0xff)
#define COLOR_RED			0x02 // RGB(0x88, 0x00, 0x00)
#define COLOR_CYAN			0x03 // RGB(0xaa, 0xff, 0xee)
#define COLOR_MAGENTA		0x04 // RGB(0xcc, 0x44, 0xcc)
#define COLOR_GREEN			0x05 // RGB(0x00, 0xcc, 0x55)
#define COLOR_BLUE			0x06 // RGB(0x00, 0x00, 0xaa)
#define COLOR_YELLOW		0x07 // RGB(0xee, 0xee, 0x77)
#define COLOR_LIGHTBROWN	0x08 // RGB(0xdd, 0x88, 0x55)
#define COLOR_BROWN			0x09 // RGB(0x66, 0x44, 0x00)
#define COLOR_PINK			0x0a // RGB(0xff, 0x77, 0x77)
#define COLOR_DARKGRAY		0x0b // RGB(0x33, 0x33, 0x33)
#define COLOR_GRAY			0x0c // RGB(0x77, 0x77, 0x77)
#define COLOR_LIME			0x0d // RGB(0xaa, 0xff, 0x66)
#define COLOR_SKYBLUE		0x0e // RGB(0x00, 0x88, 0xff)
#define COLOR_LIGHTGRAY		0x0f // RGB(0xbb, 0xbb, 0xbb)

#define COLOR_BACKGROUND	COLOR_BLACK

bool verboseMode = false, asmMode = false;

void programInfo(void)
{
	puts("ImageMapper for Easy6502 ver. " VERSION);
	puts("(C)2020 catnip1917");
}

void usage(const char* arg0)
{
	printf("usage: %s <input_file> <output_file> [-options]\n", arg0);
	printf("options: v(verbose), a(output assembly code)");
}

ALLEGRO_COLOR getPixelColor(unsigned int color) {
	unsigned int red, green, blue;
	color &= 0x00ffffff; // mask alpha value
	red = color >> 16;
	color &= 0x0000ffff;
	green = color >> 8;
	blue = color & 0xff;
	return al_map_rgb(red, green, blue);
}

int getMappedColor(unsigned int color) {
	unsigned int red, green, blue;
	color &= 0x00ffffff; // mask alpha value
	red = color >> 16;
	color &= 0x0000ffff;
	green = color >> 8;
	blue = color & 0xff;

	if (red == 0x00 && green == 0x00 && blue == 0x00)
		return COLOR_BLACK;
	else if (red == 0xff && green == 0xff && blue == 0xff)
		return COLOR_WHITE;
	else if (red == 0x88 && green == 0x00 && blue == 0x00)
		return COLOR_RED;
	else if (red == 0xaa && green == 0xff && blue == 0xee)
		return COLOR_CYAN;
	else if (red == 0xcc && green == 0x44 && blue == 0xcc)
		return COLOR_MAGENTA;
	else if (red == 0x00 && green == 0xcc && blue == 0x55)
		return COLOR_GREEN;
	else if (red == 0x00 && green == 0x00 && blue == 0xaa)
		return COLOR_BLUE;
	else if (red == 0xee && green == 0xee && blue == 0x77)
		return COLOR_YELLOW;
	else if (red == 0xdd && green == 0x88 && blue == 0x55)
		return COLOR_LIGHTBROWN;
	else if (red == 0x66 && green == 0x44 && blue == 0x00)
		return COLOR_BROWN;
	else if (red == 0xff && green == 0x77 && blue == 0x77)
		return COLOR_PINK;
	else if (red == 0x33 && green == 0x33 && blue == 0x33)
		return COLOR_DARKGRAY;
	else if (red == 0x77 && green == 0x77 && blue == 0x77)
		return COLOR_GRAY;
	else if (red == 0xaa && green == 0xff && blue == 0x66)
		return COLOR_LIME;
	else if (red == 0x00 && green == 0x88 && blue == 0xff)
		return COLOR_SKYBLUE;
	else if (red == 0xbb && green == 0xbb && blue == 0xbb)
		return COLOR_LIGHTGRAY;
	else {
		fprintf(stderr, "unknown color: (%d, %d, %d)\n", red, green, blue);
		return COLOR_BACKGROUND;
	}
}

void writeStr(FILE *fp, const char* str)
{
	fprintf(fp, str);
	if (verboseMode)
		fprintf(stdout, str);
}

int main(int argc, char* argv[])
{
	ALLEGRO_BITMAP* bitmap = NULL;
	ALLEGRO_LOCKED_REGION* mem = NULL;
	FILE* outfile = NULL;
	int bmpWidth, bmpHeight;
	int bmpFormat;

	char buf[128];

	programInfo();
	if (argc < 3 || argc > 4) {
		usage(argv[0]);
		return -1;
	}

	if (argc == 4) {
		if (strchr(argv[3], 'v'))
			verboseMode = true;
		if (strchr(argv[3], 'a'))
			asmMode = true;
	}

	al_init();
	al_init_image_addon();
	
	bitmap = al_load_bitmap(argv[1]);
	if (!bitmap) {
		fprintf(stderr, "cannot open file: %s\n", argv[1]);
		return -1;
	}

	bmpWidth = al_get_bitmap_width(bitmap);
	bmpHeight = al_get_bitmap_height(bitmap);
	bmpFormat = al_get_bitmap_format(bitmap);
	
	if (bmpWidth > 32 || bmpHeight > 32) {
		fprintf(stderr, "warning: maximum image size is 32x32, but this image file is %dx%d\n", bmpWidth, bmpHeight);
	}

	outfile = fopen(argv[2], "wt");
	if (!outfile) {
		fprintf(stderr, "file open error: %s\n", argv[2]);
		return -1;
	}
	
	mem = al_lock_bitmap(bitmap, bmpFormat, ALLEGRO_LOCK_READONLY);

	if (verboseMode) {
		printf("image file: %s\n", argv[1]);
		printf("output file: %s\n", argv[2]);
		printf("bitmap width: %d, bitmap height: %d\n", bmpWidth, bmpHeight);
		printf("pitch: %d, pixel size: %d\n", mem->pitch, mem->pixel_size);
	}

	int i, j;
	unsigned int* ptr = (unsigned int*)mem->data;
	int mappedColor;

	int baseAddr = 0x200;
	int idx = 0;

	for (i = 0; i < bmpHeight; i++) {
		for (j = 0; j < bmpWidth; j++) {
			mappedColor = getMappedColor(*ptr++);
			if (asmMode) {
				// LDA #mappedColor
				// STA baseAddr + idx
				sprintf(buf, "LDA #$%x\n", mappedColor);
				writeStr(outfile, buf);
				sprintf(buf, "STA $%x\n", baseAddr + idx);
				writeStr(outfile, buf);
				idx++;
			}
			else {
				sprintf(buf, "$%02x", mappedColor);
				writeStr(outfile, buf);
				if (j < bmpWidth - 1) {
					writeStr(outfile, ", ");
				}
			}
		}
		writeStr(outfile, "\n");
	}

	al_unlock_bitmap(bitmap);
	fclose(outfile);
	al_destroy_bitmap(bitmap);
	return 0;
}