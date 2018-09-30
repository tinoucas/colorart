#pragma once
#include <stdlib.h>

struct NormalColor
{
	double r, g, b;
	double h, s, v;
	double a;
	int weight;
};

int colorsEqual (const struct NormalColor* left, const struct NormalColor* right);
int colorsCompare (const struct NormalColor* left, const struct NormalColor* right);

struct _MagickWand;
struct ImageData
{
	struct NormalColor** pixels;
	int* pixelHash;

	size_t width;
	size_t height;
	const char* filepath;

	struct NormalColor backgroundColor;
	struct NormalColor primaryColor;
	struct NormalColor secondaryColor;
	struct NormalColor detailColor;

	struct _MagickWand *wand;
};

const struct NormalColor* getColorAt (const struct ImageData* data, int x, int y);

void printColor (const struct NormalColor* color);

#define CHARCOL(c) ((unsigned char)((c) * 255.))
#define MAKEINT(c) ((int)CHARCOL((c)->r) | ((int)CHARCOL((c)->g) << 8) | ((int)CHARCOL((c)->b) << 16) | ((int)CHARCOL((c)->a) << 24))

struct NormalColor makeColorFromHash (int hash);
