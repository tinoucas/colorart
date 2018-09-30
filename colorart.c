#include <stdio.h>
#include "colorart.h"
#include "analyse.h"
#include "color.h"
#include <MagickWand/MagickWand.h>

#define LIMIT(n, m, v) ((v) > m ? m : ((v) < n ? n : (v)))
#define NORMCOL(c) ((c) / QuantumRange)

int colorsEqual (const struct NormalColor* left, const struct NormalColor* right)
{
	return left->r == right->r
		&& left->g == right->g
		&& left->b == right->b;
}

int colorsCompare (const struct NormalColor* left, const struct NormalColor* right)
{
	int diff = MAKEINT(right) - MAKEINT(left);
	
	return diff;
}

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

void printColor(const struct NormalColor* color)
{
	printf("#%02x%02x%02x", CHARCOL(color->r), CHARCOL(color->g), CHARCOL(color->b)); 
}

void printresult (const struct ImageData* data, int printfilename)
{
	if (data->pixels != NULL)
	{
		printf("normbg \"");
		printColor(&data->backgroundColor); // "#221d19"
		printf("\", normfg \"");
		printColor(&data->primaryColor); // "#b3c176"
		printf("\", selbg \"");
		printColor(&data->primaryColor); // "#b3c176"
		printf("\", selfg \"");
		printColor(&data->backgroundColor); // "#221d19"
		printf("\"\n");
	}
		/*printColor(&data->detailColor); // "#8d816b"*/
		/*printColor(&data->secondaryColor); // "#ffffff"*/
}

void allocPixels (struct ImageData* data)
{
	data->pixels = calloc(data->height, sizeof(struct NormalColor*));
	for (int y = 0; y < data->height; ++y)
		data->pixels[y] = calloc(data->width, sizeof(struct NormalColor));
	data->pixelHash = calloc(data->width * data->height, sizeof(int));
}

int intcomp (const void* left, const void* right)
{
	return *(int*)left < *(int*)right;
}

void sortPixelHash (struct ImageData* data)
{
	int hashSize = data->width * data->height;

	qsort(data->pixelHash, hashSize, sizeof(int), &intcomp);
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
			data->pixelHash[y * data->height + x] = MAKEINT(&data->pixels[y][x]);
		}
	}

	sortPixelHash(data);

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
		free(data->pixelHash);
	}
	data->pixels = NULL;
}

static double maxsaturation = 0.628;

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
			ensuresaturation(&data, maxsaturation);
			printresult(&data, argc > 2);
		}
		data.wand = DestroyMagickWand(data.wand);
		freePixels(&data);
	}

	MagickWandTerminus();
	return 0;
}

#define NORMALCHAR(c, b) (double)(((c >> b) & 0xff) / 255.)

struct NormalColor makeColorFromHash (int hash)
{
	struct NormalColor color;

	color.r = NORMALCHAR(hash, 0);
	color.g = NORMALCHAR(hash, 8);
	color.b = NORMALCHAR(hash, 16);

	return color;
}
