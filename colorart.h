#pragma once
#include <stdlib.h>

struct NormalColor
{
	double r, g, b;
};

struct _MagickWand;
struct ImageData
{
	struct NormalColor** pixels;
	size_t width;
	size_t height;
	const char* filepath;
	struct _MagickWand *wand;
};

const struct NormalColor* getColorAt (const struct ImageData* data, int x, int y);
