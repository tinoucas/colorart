#include <stdio.h>
#include <stdlib.h>
#include <MagickWand/MagickWand.h>

#define LIMIT(n, m, v) ((v) > m ? m : ((v) < n ? n : (v)))
#define CHARCOL(c) ((unsigned char)((c) * 255. / QuantumRange))

struct MagickData
{
	const char* filepath;
	MagickWand *wand;
	size_t width;
	size_t height;
};

void analyseimage (struct MagickData* data)
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
			printf("#%02x%02x%02x ", CHARCOL(pixel.red), CHARCOL(pixel.green), CHARCOL(pixel.blue));
		}
		printf("\n");
	}

	DestroyPixelIterator(it);
}

void printresult (const struct MagickData* data, int printfilename)
{
}

int readimage (struct MagickData* data)
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
	}
	return status != MagickFalse;
}

int main (int argc, char** argv)
{
	struct MagickData data;

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
	}

	MagickWandTerminus();
	return 0;
}
