// Desabilita (no MSVC++) warnings de funes no seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include "vc.h"


// Alocar memoria para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL) return NULL;
	if ((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar memria de uma imagem
IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)));
		if (c != '#') break;
		do c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF) break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

#ifdef VC_DEBUG
			printf("\nREAD IMAGE channels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca memria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL) return NULL;

#ifdef VC_DEBUG
			printf("\nREAD IMAGE channels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}


int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL) return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}


int vc_gray_negative(IVC* srcdst) {
	if (srcdst == NULL || srcdst->data == NULL) return 0;
	if (srcdst->width <= 0 || srcdst->height <= 0 || srcdst->channels != 1) return 0;

	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}

int vc_rgb_negative(IVC* srcdst) {
	if (srcdst == NULL || srcdst->data == NULL) return 0;
	if (srcdst->width <= 0 || srcdst->height <= 0 || srcdst->channels != 3) return 0;

	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
			data[pos + 1] = 255 - data[pos + 1];
			data[pos + 2] = 255 - data[pos + 2];
		}
	}

	return 1;
}

int vc_rgb_get_red_gray(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			data[pos + 1] = data[pos];
			data[pos + 2] = data[pos];
		}
	}

	return 1;
}

int vc_rgb_get_green_gray(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos + 1];
			data[pos + 2] = data[pos + 1];
		}
	}

	return 1;
}

int vc_rgb_get_blue_gray(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos + 2];
			data[pos + 1] = data[pos + 2];
		}
	}

	return 1;
}

int vc_rgb_to_gray(IVC* src, IVC* dst) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != 3 || dst->channels != 1) return 0;

	unsigned char* data = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)data[pos_src];
			gf = (float)data[pos_src + 1];
			bf = (float)data[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}

	return 1;
}

int vc_gray_to_rgb(IVC* src, IVC* dst)
{
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL)
	{
		printf("problema data");
		return 0;
	}
	if (src->width != dst->width || src->height != dst->height || src->channels != 1 || dst->channels != 3)
	{
		printf("problema tamanho");
		return 0;
	}

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;
	int x, y;
	long int pos_src, pos_dst;
	float r[256], g[256], b[256];

	for (int i = 0; i < 256; i++)
	{
		if (i <= 64)
		{
			r[i] = 0;
			g[i] = i * 4;
			b[i] = 255;
		}
		else if (i <= 128)
		{
			r[i] = 0;
			g[i] = 255;
			b[i] = 255 - (i - 64) * 4;
		}
		else if (i <= 192)
		{
			r[i] = i * 4 - 128;
			g[i] = 255;
			b[i] = 0;
		}
		else
		{
			r[i] = 255;
			g[i] = 255 - (i - 192) * 4;
			b[i] = 0;
		}
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			int gray_value = datasrc[pos_src];
			datadst[pos_dst] = r[gray_value];     // R
			datadst[pos_dst + 1] = g[gray_value]; // G
			datadst[pos_dst + 2] = b[gray_value]; // B
		}
	}
	return 1; // Adicione o retorno
}

int vc_rgb_to_hsv(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	// Verifica��o de erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores m�ximo e m�nimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else /* rgb_max == b*/
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		data[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);
	}

	return 1;
}

void hsv_to_rgb(float h, float s, float v, unsigned char* r, unsigned char* g, unsigned char* b) {
	int i;
	float f, p, q, t;

	if (s == 0) {
		// Achromatic (grey)
		*r = *g = *b = (unsigned char)(v * 255);
		return;
	}

	h /= 60;            // sector 0 to 5
	i = (int)floor(h);
	f = h - i;          // factorial part of h
	p = v * (1 - s);
	q = v * (1 - s * f);
	t = v * (1 - s * (1 - f));

	switch (i) {
	case 0:
		*r = (unsigned char)(v * 255);
		*g = (unsigned char)(t * 255);
		*b = (unsigned char)(p * 255);
		break;
	case 1:
		*r = (unsigned char)(q * 255);
		*g = (unsigned char)(v * 255);
		*b = (unsigned char)(p * 255);
		break;
	case 2:
		*r = (unsigned char)(p * 255);
		*g = (unsigned char)(v * 255);
		*b = (unsigned char)(t * 255);
		break;
	case 3:
		*r = (unsigned char)(p * 255);
		*g = (unsigned char)(q * 255);
		*b = (unsigned char)(v * 255);
		break;
	case 4:
		*r = (unsigned char)(t * 255);
		*g = (unsigned char)(p * 255);
		*b = (unsigned char)(v * 255);
		break;
	default:        // case 5:
		*r = (unsigned char)(v * 255);
		*g = (unsigned char)(p * 255);
		*b = (unsigned char)(q * 255);
		break;
	}
}

int vc_hsv_to_rgb(IVC* src, IVC* dst) {
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	float h, s, v;
	unsigned char r, g, b;
	long int pos;

	// Check if source and destination image pointers are valid
	if (datasrc == NULL || datadst == NULL)
		return 0;

	// Convert each pixel from HSV to RGB
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			h = (float)datasrc[pos];        // Hue
			s = (float)datasrc[pos + 1] / 255.0f; // Saturation
			v = (float)datasrc[pos + 2] / 255.0f; // Value

			// Convert HSV to RGB
			hsv_to_rgb(h, s, v, &r, &g, &b);

			// Write RGB values to destination image
			datadst[pos] = r;
			datadst[pos + 1] = g;
			datadst[pos + 2] = b;
		}
	}
	return 1;
}

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != 3 || dst->channels != 1) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;
	int h, s, v;

	// Normalizar os valores de h, s e v
	printf("hmin=%d hmax=%d smin=%d smax=%d vmin=%d vmax=%d\n", hmin, hmax, smin, smax, vmin, vmax);

	int size = width * height * channels_src;

	for (int i = 0; i < size; i = i + channels_src) {
		h = (int)((float)datasrc[i] / 255.0f * 360.0f);
		s = (int)((float)datasrc[i + 1] / 255.0f * 100.0f);
		v = (int)((float)datasrc[i + 2] / 255.0f * 100.0f);

		if ((h >= hmin && h <= hmax) && (s >= smin && s <= smax) && (v >= vmin && v <= vmax)) {
			datadst[i / channels_src] = 255;
		}
		else {
			datadst[i / channels_src] = 0;
		}
	}

	return 1;
}

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != 1 || dst->channels != 3) return 0;

	unsigned char* data = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;
	int x, y;
	long int pos_src, pos_dst;
	float r[256], g[256], b[256];
	r[0] = 0;
	g[0] = 0;
	b[0] = 0;

	for (y = 0; y < 256; y++) {
		if (y < 64) {
			r[y] = 0;
			g[y] = g[y - 1] + 4;
			b[y] = 255;
		}
		else if (y < 128) {
			r[y] = 0;
			g[y] = 255;
			b[y] = b[y - 1] - 4;
		}
		else if (y < 192) {
			r[y] = r[y - 1] + 4;
			g[y] = 255;
			b[y] = 0;
		}
		else {
			r[y] = 255;
			g[y] = g[y - 1] - 4;
			b[y] = 0;
		}
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			int aux = (float)data[pos_src];

			datadst[pos_dst] = r[aux];
			datadst[pos_dst + 1] = g[aux];
			datadst[pos_dst + 2] = b[aux];
		}
	}
	return 1;
}

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;

	unsigned char* data = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;
	int x, y;
	long int pos_src, pos_dst;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (data[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else {
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_global_mean(IVC* srcdst) {
	if (srcdst == NULL || srcdst->data == NULL) return 0;

	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int channels_src = srcdst->channels;
	int bytesperline_src = width * channels_src;
	int x, y, sum = 0;
	long int pos, threshold;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline_src + x * channels_src;

			sum += data[pos];
		}
	}

	threshold = sum / (width * height);

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline_src + x * channels_src;

			if (data[pos] > threshold) data[pos] = 255;
			else data[pos] = 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int x, y;
	long int pos_src, pos_dst;

	int min = 255;
	int max = 0;
	unsigned char threshold = 0;

	int offset = (kernel - 1) / 2;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			min = 255;
			max = 0;

			for (int i = -offset; i <= offset; i++) {
				for (int j = -offset; j <= offset; j++) {
					if ((y + i >= 0) && (y + i < height) && (x + j >= 0) && (x + j < width)) {
						pos_src = (y + i) * bytesperline_src + (x + j) * channels_src;

						if (datasrc[pos_src] < min) min = datasrc[pos_src];
						if (datasrc[pos_src] > max) max = datasrc[pos_src];
					}
				}
			}

			threshold = (min + max) / 2;

			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_dst] > threshold) datadst[pos_dst] = 255;
			else datadst[pos_dst] = 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int x, y;
	long int pos_src, pos_dst;

	int min = 255;
	int max = 0;
	int cmin;
	unsigned char threshold = 0;

	int offset = (kernel - 1) / 2;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			min = 255;
			max = 0;

			for (int i = -offset; i <= offset; i++) {
				for (int j = -offset; j <= offset; j++) {
					if ((y + i >= 0) && (y + i < height) && (x + j >= 0) && (x + j < width)) {
						pos_src = (y + i) * bytesperline_src + (x + j) * channels_src;

						if (datasrc[pos_src] < min) min = datasrc[pos_src];
						if (datasrc[pos_src] > max) max = datasrc[pos_src];
					}
				}
			}

			threshold = (min + max) / 2;

			// if ((max - min) < cmin ) threshold = 
			// else threshold = 0.5 * (max + min);

			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_dst] > threshold) datadst[pos_dst] = 255;
			else datadst[pos_dst] = 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int bytesperline_src = src->width * src->channels;
	int x, y, counter = 0;
	long int pos, pos_k;
	float sdeviation, mean;

	unsigned char threshold = 0;

	int offset = (kernel - 1) / 2;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			mean = 0.0f;
			pos = y * bytesperline_src + x * channels_src;
			counter = 0;

			for (int i = -offset; i <= offset; i++) {
				for (int j = -offset; j <= offset; j++) {
					if ((y + i) >= 0 && (y + i) < height && (x + j) >= 0 && (x + j) < width) {
						pos_k = (y + i) * bytesperline_src + (x + j) * channels_src;

						mean += (float)datasrc[pos_k];

						counter++;
					}
				}
			}
			mean /= counter;
			sdeviation = 0.0f;

			for (int i = -offset; i <= offset; i++) {
				for (int j = -offset; j <= offset; j++) {
					if ((y + i >= 0) && (y + i < height) && (x + j >= 0) && (x + j < width)) {
						pos_k = (y + i) * bytesperline_src + (x + j) * channels_src;

						sdeviation += powf((float)datasrc[pos_k] - mean, 2);

						counter++;
					}
				}
			}

			sdeviation = sqrtf(sdeviation / counter);

			threshold = mean + k * sdeviation;

			if (datasrc[pos] > threshold) datadst[pos] = 255;
			else datadst[pos] = 0;
		}
	}

	return 1;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int x, y;
	long int pos_src, pos_k;

	int offset = (kernel - 1) / 2;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;

			if (datasrc[pos_src] == 255) {
				for (int i = -offset; i <= offset; i++) {
					for (int j = -offset; j <= offset; j++) {
						if ((y + i >= 0) && (y + i < height) && (x + j >= 0) && (x + j < width)) {
							pos_k = (y + i) * bytesperline_src + (x + j) * channels_src;

							datadst[pos_k] = 255;
						}
					}
				}
			}
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int size)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	int xk, yk;
	int i, j;
	long int pos, posk;
	int s1, s2;
	unsigned char pixel;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	s2 = (size - 1) / 2;
	s1 = -(s2);

	memcpy(datadst, datasrc, bytesperline * height);

	// Cálculo da erosão
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = datasrc[pos];

			for (yk = s1; yk <= s2; yk++)
			{
				j = y + yk;

				if ((j < 0) || (j >= height)) continue;

				for (xk = s1; xk <= s2; xk++)
				{
					i = x + xk;

					if ((i < 0) || (i >= width)) continue;

					posk = j * bytesperline + i * channels;

					pixel &= datasrc[posk];
				}
			}

			// Se um qualquer pixel da vizinhança, na imagem de origem, for de plano de fundo, então o pixel central
			// na imagem de destino é também definido como plano de fundo.
			if (pixel == 0) datadst[pos] = 0;
		}
	}

	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel1, int kernel2)
{
	// Exemplo em vc_binary_open:
	IVC* aux = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (aux == NULL) {
		printf("Erro ao alocar imagem auxiliar!\n");
		return 0;
	}
	// Verificações de entrada
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL ||
		src->width <= 0 || src->height <= 0 || dst->width != src->width || dst->height != src->height ||
		src->channels != 1 || dst->channels != 1 || kernel1 <= 0 || kernel2 <= 0)
	{
		return 0; // Retorna 0 em caso de erro nos parâmetros de entrada
	}

	// Cria uma imagem auxiliar para armazenar o resultado da erosão
	IVC* erosion_result = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (erosion_result == NULL)
	{
		return 0; // Retorna 0 se houve falha na alocação de memória
	}

	// Aplica a erosão na imagem de entrada
	if (!vc_binary_erode(src, erosion_result, kernel1))
	{
		vc_image_free(erosion_result);
		return 0; // Retorna 0 em caso de erro na erosão
	}

	// Aplica a dilatação na imagem resultante da erosão
	if (!vc_binary_dilate(erosion_result, dst, kernel2))
	{
		vc_image_free(erosion_result);
		return 0; // Retorna 0 em caso de erro na dilatação
	}

	// Libera a memória alocada para a imagem auxiliar
	vc_image_free(erosion_result);

	return 1; // Retorna 1 em caso de sucesso
}


int vc_binary_close(IVC* src, IVC* dst, int kernel1, int kernel2)
{
	// Exemplo em vc_binary_open:
	IVC* aux = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (aux == NULL) {
		printf("Erro ao alocar imagem auxiliar!\n");
		return 0;
	}
	// Verificações de entrada
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL ||
		src->width <= 0 || src->height <= 0 || dst->width != src->width || dst->height != src->height ||
		src->channels != 1 || dst->channels != 1 || kernel1 <= 0 || kernel2 <= 0)
	{
		return 0; // Retorna 0 em caso de erro nos parâmetros de entrada
	}

	// Cria uma imagem auxiliar para armazenar o resultado da dilatação
	IVC* dilation_result = vc_image_new(src->width, src->height, src->channels, src->levels);
	if (dilation_result == NULL)
	{
		return 0; // Retorna 0 se houve falha na alocação de memória
	}

	// Aplica a dilatação na imagem de entrada
	if (!vc_binary_dilate(src, dilation_result, kernel1))
	{
		vc_image_free(dilation_result);
		return 0; // Retorna 0 em caso de erro na dilatação
	}

	// Aplica a erosão na imagem resultante da dilatação
	if (!vc_binary_erode(dilation_result, dst, kernel2))
	{
		vc_image_free(dilation_result);
		return 0; // Retorna 0 em caso de erro na erosão
	}

	// Libera a memória alocada para a imagem auxiliar
	vc_image_free(dilation_result);

	return 1; // Retorna 1 em caso de sucesso
}


int vc_gray_dilate(IVC* src, IVC* dst, int kernel) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int x, y;
	long int pos_src, pos_k;

	int offset = (kernel - 1) / 2;
	unsigned char max_val;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			max_val = 0;

			for (int i = -offset; i <= offset; i++) {
				for (int j = -offset; j <= offset; j++) {
					if ((y + i >= 0) && (y + i < height) && (x + j >= 0) && (x + j < width)) {
						pos_k = (y + i) * bytesperline_src + (x + j) * channels_src;

						if (datasrc[pos_k] > max_val) {
							max_val = datasrc[pos_k];
						}
					}
				}
			}

			datadst[pos_src] = max_val;
		}
	}

	return 1;
}

int vc_gray_erode(IVC* src, IVC* dst, int kernel) {
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int x, y;
	long int pos_src, pos_k;

	int offset = (kernel - 1) / 2;
	unsigned char min_val;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_src = y * bytesperline_src + x * channels_src;
			min_val = 255;

			for (int i = -offset; i <= offset; i++) {
				for (int j = -offset; j <= offset; j++) {
					if ((y + i >= 0) && (y + i < height) && (x + j >= 0) && (x + j < width)) {
						pos_k = (y + i) * bytesperline_src + (x + j) * channels_src;

						if (datasrc[pos_k] < min_val) {
							min_val = datasrc[pos_k];
						}
					}
				}
			}

			datadst[pos_src] = min_val;
		}
	}

	return 1;
}

int vc_gray_open(IVC* src, IVC* dst, int kernel) {
	IVC* aux = vc_image_new(src->width, src->height, src->channels, src->levels);

	vc_gray_erode(src, aux, kernel);
	vc_gray_dilate(aux, dst, kernel);

	vc_image_free(aux);

	return 1;
}

int vc_gray_close(IVC* src, IVC* dst, int kernel) {
	IVC* aux = vc_image_new(src->width, src->height, src->channels, src->levels);

	vc_gray_dilate(src, aux, kernel);
	vc_gray_erode(aux, dst, kernel);

	vc_image_free(aux);

	return 1;
}

OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels) {
	// Verificao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (src->channels != 1) return NULL;

	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que ser retornado desta funo.


	// Copia dados da imagem binria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixis de plano de fundo devem obrigatriamente ter valor 0
	// Todos os pixis de primeiro plano devem obrigatriamente ter valor 255
	// Sero atribudas etiquetas no intervalo [1,254]
	// Este algoritmo est assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++) {
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binria
	for (y = 0; y < height; y++) {
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}

	for (x = 0; x < width; x++) {
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++) {
		for (x = 1; x < width - 1; x++) {
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0) {
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0)) {
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else {
					num = 255;

					// Se A est marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B est marcado, e  menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C est marcado, e  menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D est marcado, e  menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0) {
						if (labeltable[datadst[posA]] != num) {
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++) {
								if (labeltable[a] == tmplabel) {
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0) {
						if (labeltable[datadst[posB]] != num) {
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++) {
								if (labeltable[a] == tmplabel) {
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0) {
						if (labeltable[datadst[posC]] != num) {
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++) {
								if (labeltable[a] == tmplabel) {
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0) {
						if (labeltable[datadst[posD]] != num) {
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++) {
								if (labeltable[a] == tmplabel) {
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++) {
		for (x = 1; x < width - 1; x++) {
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0) {
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do nmero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++) {
		for (b = a + 1; b < label; b++) {
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que no hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++) {
		if (labeltable[a] != 0) {
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se no h blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL) {
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs) {

	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta rea de cada blob
	for (i = 0; i < nblobs; i++) {
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++) {
			for (x = 1; x < width - 1; x++) {
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label) {
					// rea
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Permetro
					// Se pelo menos um dos quatro vizinhos no pertence ao mesmo label, ento  um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label)) {
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

OVC* vc_blob_gray_coloring(IVC* src, IVC* dst, OVC* blobs, int nblobs) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;

	// Verificacao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binaria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Pinta os blobs
	for (i = 0; i < nblobs; i++) {
		unsigned char color = 120 + (i * 80) % 255;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				pos = y * bytesperline + x * channels;
				if (datadst[pos] == blobs[i].label) {
					datadst[pos] = color;
				}
			}
		}
	}
	return blobs;
}

 OVC* vc_blob_rgb_coloring2(IVC *src, IVC *dst, OVC *blobs, int nblobs) {
     // Verificacao de erros
     if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
     if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;

 	 unsigned char *datasrc = (unsigned char *)src->data;
     unsigned char *datadst = (unsigned char *)dst->data;
     int width = src->width;
     int height = src->height;
     int bytesperline = src->bytesperline;
     int channels = src->channels;
     int x, y, i;
     long int pos;

     // Copia dados da imagem binaria para imagem grayscale
     memcpy(datadst, datasrc, bytesperline * height);

	 for (i = 0; i < nblobs; i++) {
		 unsigned char r = rand() % 255;
		 unsigned char g = rand() % 255;
		 unsigned char b = rand() % 255;
		 for (y = 0; y < height; y++) {
			 for (x = 0; x < width; x++) {
				 pos = y * bytesperline + x * channels;
				 if (datadst[pos] == blobs[i].label) {
					 datadst[pos] = r;
					 datadst[pos + 1] = g;
					 datadst[pos + 2] = b;
				 }
			 }
		 }
	 }
	 return blobs;
 }

int vc_draw_boundingbox(IVC* srcdst, OVC* blob) {
	int c, x, y;

	for (y = blob->y; y < blob->y + blob->height; y++) {
		for (c = 0; c < srcdst->channels; c++) {
			srcdst->data[y * srcdst->bytesperline + blob->x * srcdst->channels] = 255;
			srcdst->data[y * srcdst->bytesperline + (blob->x + blob->width - 1) * srcdst->channels] = 255;
		}
	}

	for (x = blob->x; x < blob->x + blob->width; x++) {
		for (c = 0; c < srcdst->channels; c++) {
			srcdst->data[blob->y * srcdst->bytesperline + x * srcdst->channels] = 255;
			srcdst->data[(blob->y + blob->height - 1) * srcdst->bytesperline + x * srcdst->channels] = 255;
		}
	}

	return 1;
}

int vc_draw_centerofgravity(IVC* srcdst, OVC* blob) {
	int c, x, y;

	// Verificar se o centroide está dentro dos limites da imagem
	if (blob->xc < 2 || blob->xc >= (srcdst->width - 2) || blob->yc < 2 || blob->yc >= (srcdst->height - 2)) {
		return 0; // Retornar 0 se estiver fora dos limites
	}

	// Draw vertical line
	for (y = blob->yc - 2; y <= blob->yc + 2; y++) {
		for (c = 0; c < srcdst->channels; c++) {
			srcdst->data[y * srcdst->bytesperline + blob->xc * srcdst->channels + c] = 0;
		}
	}

	// Draw horizontal line
	for (x = blob->xc - 2; x <= blob->xc + 2; x++) {
		for (c = 0; c < srcdst->channels; c++) {
			srcdst->data[blob->yc * srcdst->bytesperline + x * srcdst->channels + c] = 0;
		}
	}

	return 1;
}


int vc_gray_histogram_show(IVC* src, IVC* dst) {
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int bytesperline_dst = dst->bytesperline;
	int channels_dst = dst->channels;
	int x, y;
	long int pos;

	int histogram[256];
	for (x = 0; x < 256; x++) {
		histogram[x] = 0;
	}

	// Histogram data
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			histogram[datasrc[pos]]++;
		}
	}

	for (int i = 0; i < 256; i++) {
		printf("histogram %d = %d\n", i, histogram[i]);
	}

	// Max value of pixels in array
	int max = 0;
	for (x = 0; x < 256; x++) {
		if (histogram[x] > max) {
			max = histogram[x];
		}
	}

	int offset = (dst->width - 256) / 2;

	for (int i = 0; i < 256; i++) {
		int value = (histogram[i] * dst->height) / max;
		for (int j = 0; j < dst->height; j++) {
			if (j < value)
				datadst[(dst->height - 1 - j) * bytesperline_dst + (offset + i) * channels_dst] = 255;
			else
				datadst[(dst->height - 1 - j) * bytesperline_dst + (offset + i) * channels_dst] = 0;
		}
	}

	return 1;
}

int vc_gray_histogram_equalized(IVC* src, IVC* dst) {
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int histogram[256];
	int max = 0;

	// Aloca��o de mem�ria para a imagem de destino
	if (datadst == NULL)
		return 0;

	// Inicializa��o da imagem de sa�da a zeros
	memset(datadst, 0, bytesperline * height);

	// Criar um array para contar o n�mero de pixels para cada valor de intensidade
	memset(histogram, 0, sizeof(int) * 256);
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			histogram[datasrc[pos]]++;
		}
	}

	// calcular o cdf
	int cdf[256];
	cdf[0] = histogram[0];
	for (i = 1; i < 256; i++) {
		cdf[i] = cdf[i - 1] + histogram[i];
	}

	// min do cdf
	int cdf_min = 0;
	for (i = 0; i < 256; i++) {
		if (cdf[i] != 0) {
			cdf_min = cdf[i];
			break;
		}
	}

	// Normalizar o histograma com a fun�ao dst(x,y)= (cdf(src(x,y)) - cdf_min / 1 - cdfmin) / (L*1)
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
			datadst[pos] = (unsigned char)(((cdf[datasrc[pos]] - cdf_min) / (float)(cdf[255] - cdf_min)) * 255);
		}
	}

	return 1;
}

int vc_hsv_histogram_equalized(IVC* src, IVC* dst) {
	// histogram para a satura��o e o valor da imagem
	unsigned char* datasrc = src->data;
	unsigned char* datadst = dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int histogramsaturation[256] = { 0 }; // Initialize histogram arrays
	int histogramvalue[256] = { 0 };

	// Check if destination image pointer is valid
	if (datadst == NULL)
		return 0;

	// Initialize destination image to zeros
	memset(datadst, 0, bytesperline * height);

	// Calculate histograms for saturation and value
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			histogramsaturation[datasrc[pos + 1]]++;
			histogramvalue[datasrc[pos + 2]]++;
		}
	}

	// Calculate cumulative distribution functions (CDF) for saturation and value
	int cdfsaturation[256];
	int cdfvalue[256];
	cdfsaturation[0] = histogramsaturation[0];
	cdfvalue[0] = histogramvalue[0];

	for (i = 1; i < 256; i++) {
		cdfsaturation[i] = cdfsaturation[i - 1] + histogramsaturation[i];
		cdfvalue[i] = cdfvalue[i - 1] + histogramvalue[i];
	}

	// Find the minimum non-zero CDF value for saturation and value
	int cdf_min_saturation = 0;
	int cdf_min_value = 0;
	for (i = 0; i < 256; i++) {
		if (cdfsaturation[i] != 0) {
			cdf_min_saturation = cdfsaturation[i];
			break;
		}
	}

	for (i = 0; i < 256; i++) {
		if (cdfvalue[i] != 0) {
			cdf_min_value = cdfvalue[i];
			break;
		}
	}

	// Normalize the histogram
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			// Ensure cdf_min is not zero to avoid division by zero
			datadst[pos] = datasrc[pos];
			if (cdfsaturation[255] - cdf_min_saturation != 0) {
				datadst[pos + 1] = (unsigned char)(((cdfsaturation[datasrc[pos + 1]] - cdf_min_saturation) / (float)(cdfsaturation[255] - cdf_min_saturation)) * 255);
			}
			if (cdfvalue[255] - cdf_min_value != 0) {
				datadst[pos + 2] = (unsigned char)(((cdfvalue[datasrc[pos + 2]] - cdf_min_value) / (float)(cdfvalue[255] - cdf_min_value)) * 255);
			}
		}
	}

	return 1;
}

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th) {
    int x, y, i, j;
    int channels;
    unsigned char* data_src;
    unsigned char* data_dst;
    int width, height;
    int size;
    float* buffer_fx, * buffer_fy;
    float* buffer_magnitude;
    float* buffer_direction;
    float grad_fx, grad_fy, magnitude, direction;

    // Par�metros da imagem de entrada
    channels = src->channels;
    width = src->width;
    height = src->height;
    size = width * height;
    data_src = (unsigned char*)src->data;
    data_dst = (unsigned char*)dst->data;

    // Aloca mem�ria para os buffers
    buffer_fx = (float*)malloc(size * sizeof(float));
    buffer_fy = (float*)malloc(size * sizeof(float));
    buffer_magnitude = (float*)malloc(size * sizeof(float));
    buffer_direction = (float*)malloc(size * sizeof(float));

    if (buffer_fx == NULL || buffer_fy == NULL || buffer_magnitude == NULL || buffer_direction == NULL) {
        printf("vc_gray_edge_prewitt(): failed to allocate memory for buffers.\n");
        return 0;
    }

    // Passo 1: Aplicar o operador de Prewitt em x (derivada em x)
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            grad_fx = 0.0;

            for (j = -1; j <= 1; j++) {
                for (i = -1; i <= 1; i++) {
                    if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) {
                        grad_fx += data_src[(y + j) * width + (x + i) * channels] * ((i == -1 || i == 1) ? -1.0 : 1.0);
                    }
                }
            }

            buffer_fx[y * width + x] = grad_fx;
        }
    }

    // Passo 2: Aplicar o operador de Prewitt em y (derivada em y)
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            grad_fy = 0.0;

            for (j = -1; j <= 1; j++) {
                for (i = -1; i <= 1; i++) {
                    if (x + i >= 0 && x + i < width && y + j >= 0 && y + j < height) {
                        grad_fy += data_src[(y + j) * width + (x + i) * channels] * ((j == -1 || j == 1) ? -1.0 : 1.0);
                    }
                }
            }

            buffer_fy[y * width + x] = grad_fy;
        }
    }

    // Passo 3: Calcular a magnitude do vetor gradiente
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            magnitude = sqrt(pow(buffer_fx[y * width + x], 2) + pow(buffer_fy[y * width + x], 2));
            buffer_magnitude[y * width + x] = magnitude;
        }
    }

    // Passo 4: Calcular a dire��o do vetor gradiente
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            direction = atan2(buffer_fy[y * width + x], buffer_fx[y * width + x]);
            buffer_direction[y * width + x] = direction;
        }
    }

    // Passo 5: Aplicar o threshold e determinar os pixels de contorno
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (buffer_magnitude[y * width + x] > th) {
                // Pixel de contorno
                data_dst[y * width + x] = 255;
            }
            else {
                // N�o � pixel de contorno
                data_dst[y * width + x] = 0;
            }
        }
    }

    // Libertar mem�ria
    free(buffer_fx);
    free(buffer_fy);
    free(buffer_magnitude);
    free(buffer_direction);

    return 1;
}

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize) {
    if (src == NULL || src->data == NULL || dst == NULL || dst->data == NULL) {
        fprintf(stderr, "Erro: imagens de entrada ou sa�da n�o alocadas corretamente.\n");
        return 0;
    }

    int x, y, i, j;
    int offset = kernelsize / 2; // Offset para o centro do kernel
    int sum, count;

    // Percorre a imagem de entrada
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            sum = 0;
            count = 0;
            // Percorre a vizinhan�a do pixel atual
            for (j = -offset; j <= offset; j++) {
                for (i = -offset; i <= offset; i++) {
                    // Verifica os limites da imagem
                    int px = x + i;
                    int py = y + j;
                    if (px >= 0 && px < src->width && py >= 0 && py < src->height) {
                        // Acumula o valor do pixel na vizinhan�a
                        sum += src->data[(py * src->width) + px];
                        count++;
                    }
                }
            }
            // Calcula a m�dia e atribui ao pixel na imagem de sa�da
            dst->data[(y * src->width) + x] = sum / count;
        }
    }

    return 1;
}

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize) {
    if (src == NULL || src->data == NULL || dst == NULL || dst->data == NULL) {
        fprintf(stderr, "Erro: imagens de entrada ou sa�da n�o alocadas corretamente.\n");
        return 0;
    }

    int x, y, i, j;
    int offset = kernelsize / 2; // Offset para o centro do kernel
    unsigned char* values = (unsigned char*)malloc(kernelsize * kernelsize * sizeof(unsigned char));

    if (values == NULL) {
        fprintf(stderr, "Erro: n�o foi poss�vel alocar mem�ria para valores do kernel.\n");
        return 0;
    }

    // Percorre a imagem de entrada
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            int count = 0;
            // Percorre a vizinhan�a do pixel atual
            for (j = -offset; j <= offset; j++) {
                for (i = -offset; i <= offset; i++) {
                    // Verifica os limites da imagem
                    int px = x + i;
                    int py = y + j;
                    if (px >= 0 && px < src->width && py >= 0 && py < src->height) {
                        // Armazena o valor do pixel na vizinhan�a
                        values[count++] = src->data[(py * src->width) + px];
                    }
                }
            }

            // Ordena os valores
            for (i = 0; i < count - 1; i++) {
                for (j = i + 1; j < count; j++) {
                    if (values[i] > values[j]) {
                        unsigned char temp = values[i];
                        values[i] = values[j];
                        values[j] = temp;
                    }
                }
            }

            // Calcula a mediana e atribui ao pixel na imagem de sa�da
            if (count % 2 == 1) {
                dst->data[(y * src->width) + x] = values[count / 2];
            }
            else {
                dst->data[(y * src->width) + x] = (values[count / 2 - 1] + values[count / 2]) / 2;
            }
        }
    }

    free(values);

    return 1;
}

int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst) {
	// Verificação de erros
	if (src == NULL || dst == NULL || src->data == NULL || dst->data == NULL) {
		printf("ERROR: vc_gray_lowpass_gaussian_filter() - Invalid images.\n");
		return 0;
	}
	if (src->width != dst->width || src->height != dst->height || src->channels != 1 || dst->channels != 1) {
		printf("ERROR: vc_gray_lowpass_gaussian_filter() - Image dimensions/channels mismatch.\n");
		return 0;
	}

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	// Kernel Gaussiano 3x3 (normalizado)
	float gaussian_kernel[3][3] = {
		{1.0f / 16, 2.0f / 16, 1.0f / 16},
		{2.0f / 16, 4.0f / 16, 2.0f / 16},
		{1.0f / 16, 2.0f / 16, 1.0f / 16}
	};

	// Aplicar convolução
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			float sum = 0.0f;

			// Percorrer o kernel
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int pos = (y + ky) * bytesperline + (x + kx);
					sum += datasrc[pos] * gaussian_kernel[ky + 1][kx + 1];
				}
			}

			// Garantir valores entre [0,255]
			sum = (sum < 0) ? 0 : (sum > 255) ? 255 : sum;
			datadst[y * bytesperline + x] = (unsigned char)sum;
		}
	}

	// Preencher bordas (opcional)
	for (int y = 0; y < height; y++) {
		datadst[y * bytesperline] = datasrc[y * bytesperline];                   // Borda esquerda
		datadst[y * bytesperline + (width - 1)] = datasrc[y * bytesperline + (width - 1)]; // Borda direita
	}
	for (int x = 0; x < width; x++) {
		datadst[x] = datasrc[x];                                                 // Borda superior
		datadst[(height - 1) * bytesperline + x] = datasrc[(height - 1) * bytesperline + x]; // Borda inferior
	}

	return 1;
}

int vc_gray_binary_or(IVC* src1, IVC* src2) {
	if ((src1 == NULL) || (src2 == NULL)) return 0;
	if ((src1->width != src2->width) || (src1->height != src2->height) || (src1->channels != src2->channels)) return 0;

	unsigned char* data1 = (unsigned char*)src1->data;
	unsigned char* data2 = (unsigned char*)src2->data;
	int bytesperline = src1->bytesperline;
	int channels = src1->channels;
	int width = src1->width;
	int height = src1->height;
	int x, y;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			data1[y * bytesperline + x * channels] |= data2[y * bytesperline + x * channels];
		}
	}

	return 1;
}

int vc_bgr_to_hsv(IVC* src, IVC* dst)
{
	// Ponteiros para os dados das imagens de origem e destino
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	// Largura e altura das imagens
	int width = src->width;
	int height = src->height;
	// Número de canais nas imagens
	int channels = src->channels;
	// Tamanho dos dados das imagens
	int size = width * height * channels;
	// Variáveis para os componentes de cor B, G e R, e para o espaço de cores HSV
	float b, g, r, hue, saturation, value;
	float rgb_max, rgb_min;

	// Verifica se os parâmetros das imagens são válidos
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL) ||
		(src->width != dst->width) || (src->height != dst->height) ||
		(src->channels != 3) || (dst->channels != 3))
	{
		return 0; // Retorna 0 em caso de erro
	}

	// Itera sobre os pixels da imagem de origem
	for (int i = 0; i < size; i += channels)
	{
		// Obtém os valores dos componentes de cor B, G e R
		b = (float)datasrc[i];
		g = (float)datasrc[i + 1];
		r = (float)datasrc[i + 2];

		// Calcula os valores máximo e mínimo dos canais de cor B, G e R
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Calcula o valor do canal Value (brilho)
		value = rgb_max;

		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			// Calcula o valor do canal Saturation (saturação)
			saturation = ((rgb_max - rgb_min) / rgb_max);

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
				// Calcula o valor do canal Hue (matiz)
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else /* rgb_max == b*/
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		// Atribui os valores aos pixels da imagem de destino
		datadst[i] = (unsigned char)(hue / 360.0f * 255.0f);   // Canal Hue
		datadst[i + 1] = (unsigned char)(saturation * 255.0f); // Canal Saturation
		datadst[i + 2] = (unsigned char)value;                 // Canal Value
	}

	return 1; // Retorna 1 em caso de sucesso
}

int vc_double_hsv_segmentation(IVC* src, IVC* dst, int hmin1, int hmax1, int smin1, int smax1, int vmin1, int vmax1,
	int hmin2, int hmax2, int smin2, int smax2, int vmin2, int vmax2)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->bytesperline;
	int channels_src = src->channels;
	int bytesperline_dst = dst->bytesperline;
	int channels_dst = dst->channels;
	float h, s, v;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL) || dst->data == NULL)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;
	if (src->channels != 3 || dst->channels != 1)
		return 0;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			h = ((float)datasrc[pos_src]) / 255.0f * 360.0f;
			s = ((float)datasrc[pos_src + 1]) / 255.0f * 100.0f;
			v = ((float)datasrc[pos_src + 2]) / 255.0f * 100.0f;

			bool inRange1 = (h >= hmin1 && h <= hmax1) && (s >= smin1 && s <= smax1) && (v >= vmin1 && v <= vmax1);
			bool inRange2 = (h >= hmin2 && h <= hmax2) && (s >= smin2 && s <= smax2) && (v >= vmin2 && v <= vmax2);

			datadst[pos_dst] = (inRange1 || inRange2) ? (unsigned char)255 : (unsigned char)0;
		}
	}

	return 1;
}




