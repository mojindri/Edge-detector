#include "png.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
struct measure
{
    double flops;
    float estimation;
};
//int width, height;
png_uint_32 width;
png_uint_32 height;

float **tofloatArray(png_byte **orig)
{
    float **dest = (float **)malloc(sizeof(float **) * height);
    for (int i = 0; i < height; i++)
    {
        dest[i] = (float *)malloc(sizeof(float *) * width);
        for (int j = 0; j < width; j++)
            dest[i][j] = orig[i][j]; //ok
    }
    return dest;
}
png_byte **topngArray(float **orig)
{
    png_byte **dest = (png_byte **)malloc(sizeof(png_byte **) * height);
    for (int i = 0; i < height; i++)
    {
        dest[i] = (png_byte *)malloc(sizeof(png_byte *) * width);
        for (int j = 0; j < width; j++)
            dest[i][j] = orig[i][j]; //ok
    }

    return dest;
}
/*
* read the png file 
* return: a pointing reference to png's Vector 
*/

png_byte **readpng(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");

    int bit_depth;
    int color_type;
    int interlace_method;
    int compression_method;
    int filter_method;
    png_structp png_ptr;
    png_infop info_ptr;

    fp = fopen(filepath, "rb");
    if (!fp)
    {
        abort();
    }
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        abort();
    info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr)
        abort();

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    png_read_update_info(png_ptr, info_ptr);
    png_byte **row_pointers = (png_byte **)malloc(sizeof(png_bytep **) * height);
    for (int y = 0; y < height; y++)
    {
        row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));
    }
    png_read_image(png_ptr, row_pointers);

    free(info_ptr);
    free(png_ptr);
    fclose(fp);
    return row_pointers;
}
/*
*release the occupied memory by pointer
*/
void cleanpngbytes(png_byte **rowsx)
{
    for (int y = 0; y < height; y++)
    {
        free(rowsx[y]);
    }
    free(rowsx);
}

/*
*release the occupied memory by pointer
*/
void cleanfloats(float **rowsx)
{
    for (int y = 0; y < height; y++)
    {
        free(rowsx[y]);
    }
    free(rowsx);
}

float **fillfloats()
{
    float **row_pointers = malloc(height * sizeof(float **));
    size_t w, h;
    for (h = 0; h < height; h++)
    {
        row_pointers[h] = malloc(sizeof(float *) * width);
        for (w = 0; w < width; w++)
        {
            row_pointers[h][w] = 0;
        }
    }
    return row_pointers;
}

float sum = 0;
struct measure stencilfilterapply(float **read_rows, float **write_rows)
{

    size_t w, h;
    long a = 1;
    float **row_pointers = write_rows;
    clock_t start = clock();
    for (h = 1; h < height - 1; h++)
    {
        for (w = 1; w < width - 1; w++)
        {
            sum =
                (-1 * read_rows[h - 1][w - 1]) + (-1 * read_rows[h - 1][w]) + (-1 * read_rows[h - 1][w + 1]) +
                (-1 * read_rows[h][w - 1]) + (8 * read_rows[h][w]) + (-1 * read_rows[h][w + 1]) +
                (-1 * read_rows[h + 1][w - 1]) + (-1 * read_rows[h + 1][w]) + (-1 * read_rows[h + 1][w + 1]);
            if (sum < 0)
                sum = 0;
            else if (sum > 255)
                sum = 255;
            write_rows[h][w] = sum;
        }
    }

    clock_t end = clock();
    double elapsed = ((double)(end - start) / CLOCKS_PER_SEC);
    struct measure curr_measure;
    curr_measure.flops = (((height - 2) * (width - 2) * 18) / (elapsed) / 1000000000);
    curr_measure.estimation = elapsed  * 1000;

    return curr_measure;
}

/*
* write png 
*/
void writepng(const char *filename, float **write_rows)
{
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    int depth = 8;
    fp = fopen(filename, "wb");
    if (!fp)
        abort();
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
        abort();

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
        abort();
    if (setjmp(png_jmpbuf(png_ptr)))
        abort();

    png_set_IHDR(png_ptr,
                 info_ptr,
                 width,
                 height,
                 depth,
                 PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_byte **f = topngArray(write_rows);

    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, f);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    //cleaning up
    for (int y = 0; y < height; y++)
        png_free(png_ptr, f[y]);
    png_free(png_ptr, f);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}
