#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "colorart.h"
#include "analyse.h"
#include "color.h"
#include <MagickWand/MagickWand.h>

#define LIMIT(n, m, v) ((v) > m ? m : ((v) < n ? n : (v)))
#define NORMCOL(c) ((c) / QuantumRange)
#define DIM(a) (sizeof(a)/sizeof((a)[0]))

int colorsEqual (const struct NormalColor* left, const struct NormalColor* right)
{
	return left->r == right->r
		&& left->g == right->g
		&& left->b == right->b
		&& left->a == right->a;
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
	color->a = NORMCOL(pixel->alpha);
}

const struct NormalColor* getColorAt (const struct ImageData* data, int x, int y)
{
	static struct NormalColor dummyColor;

	if (x < 0 || data->width <= x || y < 0 || data->height <= y)
		return &dummyColor;
	return &data->pixels[y][x];
}

#define COLORSTRFMT "#%02x%02x%02x"
#define COLORSTRLEN 7

void sprintColor(char* str, const struct NormalColor* color)
{
	sprintf(str, COLORSTRFMT, CHARCOL(color->r), CHARCOL(color->g), CHARCOL(color->b));
}

void fprintColor(FILE* fd, const struct NormalColor* color)
{
	fprintf(fd, COLORSTRFMT, CHARCOL(color->r), CHARCOL(color->g), CHARCOL(color->b)); 
}

void printColor(const struct NormalColor* color)
{
	fprintColor(stdout, color);
}

void debugresult (FILE* fd, const struct ImageData* data)
{
	if (fd != NULL)
	{
		fprintf(fd, "Image file '%s':\n", data->filepath);
		fprintf(fd, "background: ");
		fprintColor(fd, &data->backgroundColor);
		fprintf(fd, "\nprimary: ");
		fprintColor(fd, &data->primaryColor);
		fprintf(fd, "\ndetail: ");
		fprintColor(fd, &data->detailColor);
		fprintf(fd, "\nsecondary: ");
		fprintColor(fd, &data->secondaryColor);
		fprintf(fd, "\n\n");
	}
}

char* ensureresultlength (char** presult, int neededlen)
{
	static const int blocksize = 1024;
	char* result = *presult;
	int resultlen = result == NULL ? 0 : strlen(result);
	int alloclen = result == NULL ? 0 : (resultlen / blocksize + 1) * blocksize + 1;

	if (result == NULL || alloclen <= resultlen + neededlen)
		result = realloc(result, (alloclen + blocksize) * sizeof(char));

	*presult = result;
	result += resultlen;
	*result = 0;
	return result;
}

void appendResultStr (char** presult, const char* str, int nchars)
{
	char* result = ensureresultlength(presult, nchars);

	if (result != NULL)
	{
		strncpy(result, str, nchars);
		*(result + nchars) = 0;
	}
}

void appendResultColor (char** presult, const struct NormalColor* color)
{
	char* result = ensureresultlength(presult, COLORSTRLEN);

	if (result != NULL)
		sprintColor(result, color);
}

void printresult (const struct ImageData* data, int printfilename, const char* format)
{
	if (printfilename)
		printf("%s: ", data->filepath);

	if (data->pixels != NULL)
	{
		char* result = NULL;

		if (format != NULL && *format != 0)
		{
			const struct
			{
				const char wildcard;
				const struct NormalColor *color;
			} validTokens[] =
			{
				{ 'b', &data->backgroundColor },
				{ 'p', &data->primaryColor },
				{ 's', &data->secondaryColor },
				{ 'd', &data->detailColor },
			};

			int formatlen = strlen(format);
			const char* formatleft = format;

			while (*formatleft != 0)
			{
				const char* nextToken = strstr(formatleft, "%");
				int copynchars = 0;

				if (nextToken != NULL)
					copynchars = nextToken - formatleft;
				else
					copynchars = strlen(formatleft);

				if (copynchars > 0)
				{
					appendResultStr(&result, formatleft, copynchars);
					formatleft += copynchars;
				}
				if (nextToken != NULL)
				{
					int tokenFound = 0;

					++formatleft;
					++nextToken;

					if (*nextToken == '%')
						appendResultStr(&result, "%", 1);
					else
						for (int i = 0; i < DIM(validTokens); ++i)
						{
							if (*nextToken == validTokens[i].wildcard)
							{
								appendResultColor(&result, validTokens[i].color);
								++formatleft;
								tokenFound = 1;
								break;
							}
						}
				}
			}
		}
		if (result != NULL)
		{
			printf("%s", result);
			free(result);
		}
	}
	else
	{
		printf("no image found.");
	}
	printf("\n");
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

struct Options
{
	double maxsaturation; // 0.628;
	const char* format;
	int printfilename;
	int quiet;
};

void usage (const char* procName)
{
	fprintf(stderr, "Usage: %s [-fq] [-s maxsat] [-F formatstr] image [image...]\n"
			"-f: print file path\n"
			"-q: quiet\n"
			"-s maxsat: limit output color saturation (0..1)\n"
			"-F formatstr: format output:\n"
			"	'%b': background color\n"
			"	'%p': primary color\n"
			"	'%s': secondary color\n"
			"	'%d': detail color\n"
			, procName);
	exit(1);
}

void initoptions (struct Options* options)
{
	options->maxsaturation = 1.;
	options->format = NULL;
	options->printfilename = 0;
	options->quiet = 0;
}

void readoptions (struct Options* options, int argc, char** argv)
{
	int error = 0;
	int c;
	opterr = 0;

	while ((c = getopt (argc, argv, "s:fF:q")) != -1)
		switch (c)
		{
		case 's':
			{
				double maxsaturation = atof(optarg);

				if (0. <= maxsaturation && maxsaturation <= 1.)
					options->maxsaturation = maxsaturation;
				else
				{
					fprintf(stderr, "saturation needs to be in the range 0..1");
					error = 1;
				}
			}
			break;
		case 'f':
			options->printfilename = 1;
			break;
		case 'F':
			options->format = optarg;
			break;
		case 'q':
			options->quiet = 1;
			break;
		default:
			error = 1;
			break;
		}

	if (error)
	{
		usage(argv[0]);
	}

}

int main (int argc, char** argv)
{
	struct ImageData data;
	struct Options options;

	data.pixels = NULL;

	initoptions(&options);
	readoptions(&options, argc, argv);

	if (argc - optind < 1)
	{
		usage(argv[0]);
	}

	MagickWandGenesis();

	for (int i = optind; i < argc; ++i)
	{
		data.filepath = argv[i];
		if (readimage(&data))
		{
			analyseimage(&data);
			ensuresaturation(&data, options.maxsaturation);
			printresult(&data, options.printfilename, options.format);
			if (!options.quiet && (options.format == NULL || *options.format != 0))
				debugresult(stderr, &data);
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
