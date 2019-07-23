#include "png.h"
#include "omp.h"
#include "mm_malloc.h"

size_t width, height;

png_byte **read;
png_byte **write;

void initHeightWidth(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
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
    free(info_ptr);
    free(png_ptr);
}
void readpng(const char *filepath, png_byte **row_pointers)
{
    FILE *fp = fopen(filepath, "rb");
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
    png_read_update_info(png_ptr, info_ptr);
    png_read_image(png_ptr, row_pointers);

    free(info_ptr);
    free(png_ptr);
    fclose(fp);
}
void writepng(const char *filename, png_byte **f)
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
png_byte **createinputMatrix(short h, short w)
{

    png_byte **rows = _mm_malloc(h * sizeof(png_bytepp), sizeof(png_bytepp));
    short i, j;
    png_byte sum = 0;
#pragma omp parallel for default(none) shared(rows, h, w) private(i, j) reduction(+ \
                                                                                  : sum) schedule(dynamic)

    for (i = 0; i < h; i++)
    {
        rows[i] = _mm_malloc(sizeof(png_byte *) * w, sizeof(png_bytepp));
        for (j = 0; j < w; j++)
            rows[i][j] = 255;
    }
    return rows;
}

double start = 0, elapsed = 0, end = 0, flops = 0;
size_t sum;

void test(const char *infile, const char *outfile)
{

    initHeightWidth(infile);
    write = createinputMatrix(height, width);
    read = createinputMatrix(height, width);
    readpng(infile, read);
    size_t h, w;
    int count = 10;
    double x, y;
    for (int i = 0; i < count; i++)
    {
        start = omp_get_wtime();
#pragma omp parallel for default(none) shared(read, write, height, width) private(h, w) reduction(+ \
                                                                                                  : sum) schedule(dynamic)
        for (h = 1; h < height - 1; h++)
            for (w = 1; w < width - 1; w++)
            {
                sum = (-1 * read[h - 1][w - 1]) + (-1 * read[h - 1][w]) + (-1 * read[h - 1][w + 1]) +
                      (-1 * read[h][w - 1]) + (8 * read[h][w]) + (-1 * read[h][w + 1]) +
                      (-1 * read[h + 1][w - 1]) + (-1 * read[h + 1][w]) + (-1 * read[h + 1][w + 1]);
                sum = sum < 0 ? 0 : sum;
                sum = sum > 255 ? 255 : sum;
                write[h][w] = sum;
            }

        end = omp_get_wtime();
        x = (end - start) * 1000;
        y = ((height - 2) * (width - 2) * 18) / x / 1000000;
        elapsed += x;
        flops += y;
        printf("-> Computaton Time elapsed : %f s and %f Gflop/s \n", x, y);
    }
    printf("-------------------------------------------------------\n");
    printf("Averaged elapsed time for %d cycle(s) : %f ms\n", count, elapsed / count);
    printf("Averaged FLOPS for %d cycle(s) : %f GFLOP/s\n", count, (double)(flops / count));
    writepng(outfile, write);
    printf("-> finished!\n");
}

void main()
{
    test("test-image.png", "output.png");
}