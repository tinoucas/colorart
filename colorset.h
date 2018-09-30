#pragma once

#include "colorart.h"

struct ColorSet
{
	struct NormalColor* colors;
	int *pixelHash;
	int hashSorted;
	int size;
	int capacity;
};

struct ColorSet* createColorSet ();
struct ColorSet* createColorSetWithData (const struct ImageData* data);
void freeColorSet (struct ColorSet* colorset);

void appendColor (struct ColorSet* colorset, const struct NormalColor* color);
void appendWeightedColor (struct ColorSet* colorset, const struct NormalColor* color, int weight);
int countColorsMatching (struct ColorSet* colorset, const struct NormalColor* color);
void sortColorsetByWeight (struct ColorSet* colorset);
void sortSetPixelHash (struct ColorSet* colorset);
int containsColor (struct ColorSet* colorset, const struct NormalColor* color);
