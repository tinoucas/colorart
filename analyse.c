#include <stdio.h>
#include <math.h>
#include "analyse.h"
#include "colorset.h"

#define fequalzero(a) (fabs(a) < FLT_EPSILON)
#define colorThresholdMinimumPercentage 0.01

int colorIsBlackOrWhite (const struct NormalColor* color)
{
	if (color->r > .91 && color->g > .91 && color->b > .91 )
		return 1; // white

	if (color->r < .09 && color->g < .09 && color->b < .09 )
		return 1; // black

	return 0;
}

int colorIsDark (const struct NormalColor* color)
{
	double lum = 0.2126 * color->r + 0.7152 * color->g + 0.0722 * color->b;

	return lum < .5;
}

int colorIsContrastingWith (const struct NormalColor* background, const struct NormalColor* foreground)
{
	double bLum = 0.2126 * background->r + 0.7152 * background->g + 0.0722 * background->b;
	double fLum = 0.2126 * foreground->r + 0.7152 * foreground->g + 0.0722 * foreground->b;
	double contrast = 0.;

	if (bLum > fLum)
		contrast = (bLum + 0.05) / (fLum + 0.05);
	else
		contrast = (fLum + 0.05) / (bLum + 0.05);

	//return contrast > 3.0; //3-4.5 W3C recommends 3:1 ratio, but that filters too many colors
	return contrast > 1.6;
}

int colorIsDistinctWith (const struct NormalColor* color, const struct NormalColor* compareColor)
{
	double threshold = .25; //.15

	if (fabs(color->r - compareColor->r) > threshold ||
		fabs(color->g - compareColor->g) > threshold ||
		fabs(color->b - compareColor->b) > threshold)
	{
		// check for grays, prevent multiple gray colors

		if (fabs(color->r - color->g) < .03 && fabs(color->r - color->b) < .03)
		{
			if (fabs(compareColor->r - compareColor->g) < .03 && fabs(compareColor->r - compareColor->b) < .03)
				return 0;
		}

		return 1;
	}
	
	return 0;
}

void findEdgeColor (struct ImageData* data, struct NormalColor* edgeColor)
{
	int searchColumnX = 0;
	struct ColorSet* leftEdgeColors = createColorSet();
	
	for (int x = 0; x < data->width; ++x)
	{
		for (int y = 0; y < data->height; ++y)
		{
			struct NormalColor color = *getColorAt(data, x, y);
			
			if (x == searchColumnX)
			{
				//make sure it's a meaningful color
				/*if ( color.alphaComponent > .5 )*/
				appendColor(leftEdgeColors, &color);
			}
		}
		
		// background is clear, keep looking in next column for background color
		if (leftEdgeColors->size == 0 )
			searchColumnX += 1;
	}

	struct NormalColor *curColor = NULL;
	struct ColorSet* sortedColors = createColorSet();

	int randomColorsThreshold = (int)((double)data->height * colorThresholdMinimumPercentage);

	for (int i = 0; i < leftEdgeColors->size; ++i)
	{
		struct NormalColor* curColor = &leftEdgeColors->colors[i];
		int colorCount = countColorsMatching(leftEdgeColors, curColor);

		if (colorCount <= randomColorsThreshold) // prevent using random colors, threshold based on input image height
			continue;

		appendWeightedColor(sortedColors, curColor, colorCount);
	}

	sortColorsetByWeight(sortedColors);

	struct NormalColor* proposedEdgeColor = NULL;

	if (sortedColors->size > 0)
	{
		proposedEdgeColor = &sortedColors->colors[0];

		if (colorIsBlackOrWhite(proposedEdgeColor)) // want to choose color over black/white so we keep looking
		{
			for (int i = 1; i < sortedColors->size; ++i)
			{
				struct NormalColor *nextProposedColor = &sortedColors->colors[i];

				if (((double)nextProposedColor->weight / (double)proposedEdgeColor->weight) > .3 ) // make sure the second choice color is 30% as common as the first choice
				{
					if (!colorIsBlackOrWhite(nextProposedColor))
					{
						proposedEdgeColor = nextProposedColor;
						break;
					}
				}
				else
				{
					// reached color threshold less than 40% of the original proposed edge color so bail
					break;
				}
			}
		}
	}

	if (proposedEdgeColor != NULL)
		*edgeColor = *proposedEdgeColor;

	freeColorSet(leftEdgeColors);
	freeColorSet(sortedColors);
}

int countColorsMatchingData (const struct ImageData* data, const struct NormalColor* color)
{
	int hash = MAKEINT(color);
	int hashSize = data->width * data->height;
	int count = 0;

	for (int i = 0; i < hashSize; ++i)
	{
		if (data->pixelHash[i] == hash)
			++count;
		else if (count > 0)
			break;
	}

	return count;
}

void findTextColors (struct ImageData* data, struct NormalColor* primaryColor, struct NormalColor* secondaryColor, struct NormalColor* detailColor, struct NormalColor* backgroundColor)
{
	int havePrimaryColor = 0;
	int haveSecondaryColor = 0;
	int haveDetailColor = 0;

	struct NormalColor curColor;
	struct ColorSet* sortedColors = createColorSet();
	int findDarkTextColor = !colorIsDark(backgroundColor);

	int i = 0;
	int hashSize = data->width * data->height;
	while (i < hashSize)
	{
		int hash = data->pixelHash[i]; 
		struct NormalColor color = makeColorFromHash(hash);

		if (colorIsDark(&color) == findDarkTextColor)
		{
			int count = 1;

			while (data->pixelHash[++i] == hash)
				++count;

			/*if (count <= 2) // prevent using random colors, threshold should be based on input image size*/
			/*    continue;*/

			appendWeightedColor(sortedColors, &color, count);
		}
		else
		{
			++i;
		}
	}

	sortColorsetByWeight(sortedColors);

	for (int i = 0; i < sortedColors->size; ++i)
	{
		curColor = sortedColors->colors[i];

		if (!havePrimaryColor)
		{
			if (colorIsContrastingWith(&curColor, backgroundColor))
			{
				*primaryColor = curColor;
				havePrimaryColor = 1;
			}
		}
		else if (!havePrimaryColor)
		{
			if (!colorIsDistinctWith(primaryColor, &curColor) || !colorIsContrastingWith(&curColor, backgroundColor))
				continue;
			*secondaryColor = curColor;
			haveSecondaryColor = 1;
		}
		else if (!haveDetailColor)
		{
			if (!colorIsDistinctWith(secondaryColor, &curColor) || !colorIsDistinctWith(primaryColor, &curColor) || !colorIsContrastingWith(&curColor, backgroundColor))
				continue;

			*detailColor = curColor;
			haveDetailColor = 1;
			break;
		}
	}
}

void analyseimage (struct ImageData* data)
{
	/*NSCountedSet *imageColors = nil;*/
	struct NormalColor backgroundColor;
	
	findEdgeColor(data, &backgroundColor);

	struct NormalColor primaryColor;
	struct NormalColor secondaryColor;
	struct NormalColor detailColor;

	int darkBackground = colorIsBlackOrWhite(&backgroundColor);

	if ( darkBackground )
	{
		struct NormalColor whiteColor;

		whiteColor.r = 1.;
		whiteColor.g = 1.;
		whiteColor.b = 1.;

		primaryColor = whiteColor;
		secondaryColor = whiteColor;
		detailColor = whiteColor;
	}
	else
	{
		struct NormalColor blackColor;

		blackColor.r = 1.;
		blackColor.g = 1.;
		blackColor.b = 1.;

		primaryColor = blackColor;
		secondaryColor = blackColor;
		detailColor = blackColor;
	}

	findTextColors(data, &primaryColor, &secondaryColor, &detailColor, &backgroundColor);

	data->backgroundColor = backgroundColor;
	data->primaryColor = primaryColor;
	data->secondaryColor = secondaryColor;
	data->detailColor = detailColor;
}
