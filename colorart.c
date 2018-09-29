#include <stdio.h>
#include "colorart.h"
#include "analyse.h"
#include <MagickWand/MagickWand.h>

#define LIMIT(n, m, v) ((v) > m ? m : ((v) < n ? n : (v)))
#define NORMCOL(c) ((c) / QuantumRange)
#define CHARCOL(c) ((unsigned char)((c) * 255.))

void makeNormalColor (const PixelInfo* pixel, struct NormalColor* color)
{
	color->r = NORMCOL(pixel->red);
	color->g = NORMCOL(pixel->green);
	color->b = NORMCOL(pixel->blue);
}

const struct NormalColor* getColorAt (const struct ImageData* data, int x, int y)
{
	static struct NormalColor dummyColor;

	if (x < 0 || data->width <= x || y < 0 || data->height <= y)
		return &dummyColor;
	return &data->pixels[y][x];
}

void printresult (const struct ImageData* data, int printfilename)
{
	if (data->pixels != NULL)
	{
		for (int x = 0; x < data->width; ++x)
		{
			for (int y = 0; y < data->height; ++y)
				printf("#%02x%02x%02x ", CHARCOL(getColorAt(data, x, y)->r), CHARCOL(getColorAt(data, x, y)->g), CHARCOL(getColorAt(data, x, y)->b)); 
			printf("\n");
		}
	}
}

void allocPixels (struct ImageData* data)
{
	data->pixels = calloc(data->height, sizeof(struct NormalColor*));
	for (int y = 0; y < data->height; ++y)
		data->pixels[y] = calloc(data->width, sizeof(struct NormalColor));
}

void fillPixels (struct ImageData* data)
{
	PixelIterator* it = NewPixelIterator(data->wand);
	PixelWand** rowpixels;
	PixelInfo pixel;
	size_t width;

	for (int y = 0; y < data->height; ++y)
	{
		rowpixels = PixelGetNextIteratorRow(it, &width);

		if (width != data->width)
		{
			fprintf(stderr, "Width changed at row %d!\n", y);
			break;
		}
		for (int x = 0; x < data->width; ++x)
		{
			PixelGetMagickColor(rowpixels[x], &pixel);
			makeNormalColor(&pixel, &data->pixels[y][x]);
		}
	}

	DestroyPixelIterator(it);
}

int readimage (struct ImageData* data)
{
	MagickBooleanType status;

	data->wand = NewMagickWand();
	status = MagickReadImage(data->wand, data->filepath);
	if (status == MagickFalse)
	{
		char *description;
		ExceptionType severity;

		description = MagickGetException(data->wand, &severity);
		fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(), description);
		description = MagickRelinquishMemory(description);
	}
	else
	{
		data->width = MagickGetImageWidth(data->wand);
		data->height = MagickGetImageHeight(data->wand);
		allocPixels(data);
		fillPixels(data);
	}
	return status != MagickFalse;
}

void freePixels (struct ImageData* data)
{
	if (data->pixels)
	{
		for (int y = 0; y < data->height; ++y)
			free(data->pixels[y]);
		free(data->pixels);
	}
	data->pixels = NULL;
}

int main (int argc, char** argv)
{
	struct ImageData data;

	data.pixels = NULL;

	if (argc <= 1)
	{
		fprintf(stderr, "Usage: %s image\n", argv[0]);
		exit(3);
	}

	MagickWandGenesis();

	for (int i = 1; i < argc; ++i)
	{
		data.filepath = argv[i];
		if (readimage(&data))
		{
			analyseimage(&data);
			printresult(&data, argc > 2);
		}
		data.wand = DestroyMagickWand(data.wand);
		freePixels(&data);
	}

	MagickWandTerminus();
	return 0;
}
