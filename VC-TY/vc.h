#define VC_DEBUG
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char* data;
	int width, height;
	int channels;			// Binario/Cinzentos=1; RGB=3
	int levels;				// Binario=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;

typedef struct {
	int x, y, width, height;
	int area;
	int xc, yc;
	int perimeter;
	int label;

	unsigned char* mask;
	unsigned char* data;

	int channels;
	int levels;
} OVC;


// ALOCAR E LIBERTAR UMA IMAGEM
IVC* vc_image_new(int width, int height, int channels, int levels);
IVC* vc_image_free(IVC* image);

// LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC* vc_read_image(char* filename);
int vc_write_image(char* filename, IVC* image);

// GERAR NEGATIVO DA IMAGEM GRAY
int vc_gray_negative(IVC* srcdst);

// GERAR NEGATIVO DA IMAGEM RGB
int vc_rgb_negative(IVC* srcdst);

// EXTRAIR COMPONENTES R/G/B DE UMA IMAGEM RGB PARA IMAGENS EM TONS DE CINZENTO
int vc_rgb_get_red_gray(IVC* srcdst);
int vc_rgb_get_green_gray(IVC* srcdst);
int vc_rgb_get_blue_gray(IVC* srcdst);

// CONVERTER UMA IMAGEM NO ESPAÇO RGB PARA UMA IMAGEM EM TONS DE CINZENTO
int vc_rgb_to_gray(IVC* src, IVC* dst);

// CONVERTER UMA IMAGEM EM TONS DE CINZENTO PARA UMA IMAGEM NO ESPAÇO RGB
int vc_gray_to_rgb(IVC* src, IVC* dst);

// CONVERTER UMA IMAGEM RGB -> HSV  
int vc_rgb_to_hsv(IVC* srcdst);
// CONVERTER UMA IMAGEM HSV -> RGB  
int vc_hsv_to_rgb(IVC* src, IVC* dst);

// RECEBER IMAGEM HSV E RETORNA IMAGEM COM 1 CANAL 
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

// CONVERTER UMA IMAGEM COM ESCALA DE INTENSIDADES EM CINZENTO NUMA IMAGEM COM UMA ESCALA EM CORES DO ESPAÇO RGB
int vc_scale_gray_to_color_palette(IVC* src, IVC* dst);

// CONVERTER IMAGEM GRAY EM BINARIO C/ THRESHOLD
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);

// CONVERTER IMAGEM GRAY EM BINARIO S/ THRESHOLD E PELA MEDIA GLOBAL
int vc_gray_to_binary_global_mean(IVC* srcdst);

// CONVERTER IMAGEM GRAY EM BINARIO C/ MIDPOINT (PIXEL CENTRAL)
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel);
// ^ METODO BERNSEN -- DOESNT WORK
int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel);
// ^ METODO NIBLACK
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k);


// IMAGEM BINARIA
// DILATACAO
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
// EROSAO 
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
// ABERTURA - EROSAO DEPOIS DILATACAO
int vc_binary_open(IVC* src, IVC* dst, int kernel1, int kernel2);
// FECHO - DILATACAO DEPOIS EROSAO
int vc_binary_close(IVC* src, IVC* dst, int kernel1, int kernel2);


// IMAGEM GRAY
// DILATACAO
int vc_gray_dilate(IVC* src, IVC* dst, int kernel);
// EROSAO 
int vc_gray_erode(IVC* src, IVC* dst, int kernel);
// ABERTURA - EROSAO DEPOIS DILATACAO
int vc_gray_open(IVC* src, IVC* dst, int kernel);
// FECHO - DILATACAO DEPOIS EROSAO
int vc_gray_close(IVC* src, IVC* dst, int kernel);


// BLOBS
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

OVC* vc_blob_gray_coloring(IVC* src, IVC* dst, OVC* blobs, int nblobs);

OVC* vc_blob_rgb_coloring2(IVC *src, IVC *dst, OVC *blobs, int nblobs); // 

int vc_draw_boundingbox(IVC* srcdst, OVC* blob);

int vc_draw_centerofgravity(IVC* srcdst, OVC* blob);

// HISTOGRAMAS
// GRAY
int vc_gray_histogram_show(IVC* src, IVC* dst);

int vc_gray_histogram_equalized(IVC* src, IVC* dst);

// HSV
int vc_hsv_histogram_equalized(IVC* src, IVC* dst);


// Contornos de imagem
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th);

//Filtros no dominio espacial
int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize);

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize);

int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst);

//PROJETO

int vc_gray_binary_or(IVC* src1, IVC* src2);

int vc_double_hsv_segmentation(IVC* src, IVC* dst, int hmin1, int hmax1, int smin1, int smax1, int vmin1, int vmax1,
	int hmin2, int hmax2, int smin2, int smax2, int vmin2, int vmax2);

int vc_bgr_to_hsv(IVC* src, IVC* dst);
