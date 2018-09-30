#include "colorset.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

static const int chunkSize = 1024;

struct ColorSet* createColorSet ()
{
	struct ColorSet* colorset = calloc(1, sizeof(struct ColorSet));

	return colorset;
}

struct ColorSet* createColorSetWithData (const struct ImageData* data)
{
	struct ColorSet* colorset = createColorSet();

	for (int x = 0; x < data->width; ++x)
		for (int y = 0; y < data->height; ++y)
			appendColor(colorset, getColorAt(data, x, y));

	return colorset;
}

void freeColorSet (struct ColorSet* colorset)
{
	if (colorset->capacity > 0)
		free(colorset->colors);
	free(colorset);
}

void appendWeightedColor (struct ColorSet* colorset, const struct NormalColor* color, int weight)
{
	if (colorset->size + 1 > colorset->capacity)
	{
		colorset->capacity += chunkSize;
		colorset->colors = realloc(colorset->colors, colorset->capacity * sizeof(struct NormalColor));
		colorset->pixelHash = realloc(colorset->pixelHash, colorset->capacity * sizeof(int));
	}
	colorset->colors[colorset->size] = *color;
	colorset->pixelHash[colorset->size] = MAKEINT(color);
	colorset->hashSorted = 0;
	++colorset->size;
}

void appendColor (struct ColorSet* colorset, const struct NormalColor* color)
{
	appendWeightedColor(colorset, color, 1);
}

static int weightcomp (const void* left, const void* right)
{
	return ((struct NormalColor*)left)->weight > ((struct NormalColor*)right)->weight;
}

void sortColorsetByWeight (struct ColorSet* colorset)
{
	qsort(colorset->colors, colorset->size, sizeof(struct NormalColor), &weightcomp);
}

int intcomp (const void* left, const void* right);

void sortSetPixelHash (struct ColorSet* colorset)
{
	if (!colorset->hashSorted)
	{
		qsort(colorset->pixelHash, colorset->size, sizeof(int), &intcomp);
		colorset->hashSorted = 1;
	}
}

int containsColorRecurs(int* pixelHash, int low, int high, int hash) 
{ 
   if (high < low) 
       return -1; 

   int mid = (low + high) / 2;
   int diff = hash - pixelHash[mid];

   if (diff == 0) 
       return mid; 
   if (diff < 0) 
       return containsColorRecurs(pixelHash, (mid + 1), high, hash); 
   return containsColorRecurs(pixelHash, low, (mid - 1), hash); 
}

int containsColor (struct ColorSet* colorset, const struct NormalColor* color)
{
	if (colorset->size == 0)
		return 0;

	int hash = MAKEINT(color);

	sortSetPixelHash(colorset);
	return containsColorRecurs(colorset->pixelHash, 0, colorset->size, hash) >= 0;
}

int countColorsMatching (struct ColorSet* colorset, const struct NormalColor* color)
{
	if (colorset->size == 0)
		return 0;

	int hash = MAKEINT(color);

	sortSetPixelHash(colorset);

	int matchingIndex = containsColorRecurs(colorset->pixelHash, 0, colorset->size, hash);
	int count = 0;

	if (matchingIndex >= 0)
	{
		count = 1;
		for (int i = matchingIndex + 1; i < colorset->size && colorset->pixelHash[i] == hash; ++i)
			++count;
		for (int i = matchingIndex - 1; i >= 0 && colorset->pixelHash[i] == hash; --i)
			++count;
	}

	return count;
}

