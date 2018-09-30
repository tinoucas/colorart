#include <stdio.h>
#include "color.h"

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

void
makeHSVComp (struct NormalColor* color)
{
	double min = MIN(MIN(color->r, color->g), color->b);
	double max = MAX(MAX(color->r, color->g), color->b);
	double v = max;
	double delta = max - min;

	double h = 0.;
	double s = 0.;

	if (max != 0.)
		s = delta / max;        // s
	else
	{
		// r = g = b = 0        // s = 0, v is undefined
		h = -1.;
		return ;
	}
	if (delta != 0.)
	{
		if (color->r == max)
			h = (color->g - color->b) / delta;	  // between yellow & magenta
		else if (color->g == max)
			h = 2. + (color->b - color->r) / delta;  // between cyan & yellow
		else
			h = 4. + (color->r - color->g) / delta;  // between magenta & cyan
		h *= 60.;                // degrees
		if(h < 0)
			h += 360.;
	}
	else
		h = 0.;

	color->h = h;
	color->s = s;
	color->v = v;
}

#define APPLYRGB(c, R, G, B) (c)->r = (R); (c)->g = (G); (c)->b = (B)

void
makeRGBComp (struct NormalColor* color)
{
	int i;
	double h, s, v, r, g, b;
	double f;

	h = color->h;
	s = color->s;
	v = color->v;

	if (s == 0)
	{
		// achromatic (grey)
		r = g = b = v;
	}
	else
	{
		h /= 60;			// sector 0 to 5
		i = (int)h;
		f = h - i;		  // factorial part of h

		double p = v * ( 1 - s );
		double q = v * ( 1 - s * f );
		double t = v * ( 1 - s * ( 1 - f ) );

		switch (i)
		{
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:		// case 5:
			r = v;
			g = p;
			b = q;
			break;
		}
	}
	APPLYRGB(color, r, g, b);
}

void ensuresaturationofcolor (struct NormalColor* color, double maxsat)
{
	makeHSVComp(color);
	if (color->s > maxsat)
	{
		color->s = maxsat;
		makeRGBComp(color);
	}
}

void ensuresaturation (struct ImageData* data, double maxsat)
{
	ensuresaturationofcolor(&data->backgroundColor, maxsat);
	ensuresaturationofcolor(&data->primaryColor, maxsat);
	ensuresaturationofcolor(&data->secondaryColor, maxsat);
	ensuresaturationofcolor(&data->detailColor, maxsat);
}

