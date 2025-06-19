#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
 
#define OUTPUT_PPM "output.ppm"
#define OUTPUT_HISTOGRAM "output.txt"
 
int mask[3][3] = {
    {0, -1, 0},
    {-1, 5, -1},
    {0, -1, 0}};
 
typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Cell;
 
typedef struct
{
    int height;
    int width;
    Cell *data;
} Image;
 
Image *readPPM(const char *filename, int *maxVal);                                              // function for reading file
void writePPM(const char *filename, Image *image, int maxval);                                  // function to write ppm file
void writeHistogram(const char *filename, int *histogram);                                      // function to write histogram file
void pixelHistogram(int *histogram, Cell cell);                                                 // function to find histogram one pixel
Cell masking(Image *image, size_t curPixel);                                                    // function for mask
Image processImage(Image *image, int *histogram);                                               // function for image processing
void oneStringProcess(Cell *newCell, Cell *cell, int number, int *histogram);                   // function for processing one line (top and bottom of the picture)
void applyMask(Image *image, Image *newImage, int *histogram, int *pixHistogram, int y_height); // function to apply a mask
void processEdges(Image *image, Image *newImage, int *histogram, int y_height);                 // function for processing image edges
 
int main(int argc, char *argv[])
{
    // reading
    if (argc != 2)
    {
        fprintf(stderr, "Input error.\n");
        return EXIT_FAILURE;
    }
    char *filename = argv[1];
    int maxVal;
    Image *image = readPPM(filename, &maxVal);
    if (!image)
    {
        fprintf(stderr, "Reading error.\n");
        return EXIT_FAILURE;
    }
    int histogram[5] = {0};
    // convolution
    Image processedImage = processImage(image, histogram);
    // writing ppm
    writePPM(OUTPUT_PPM, &processedImage, maxVal);
    // writing histogram
    writeHistogram(OUTPUT_HISTOGRAM, histogram);
    // frees
    free(processedImage.data);
    free(image->data);
    free(image);
    return EXIT_SUCCESS;
}
 
Image *readPPM(const char *filename, int *maxVal)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "Error open file.\n");
        return NULL;
    }
    // read the file header
    char magic[3];
    if (fscanf(file, "%s", magic) != 1)
    {
        fprintf(stderr, "Error reading magic header.\n");
        fclose(file);
        return NULL;
    }
 
    if (magic[0] != 'P' || magic[1] != '6')
    {
        fprintf(stderr, "File format not supported: %s\n", magic);
        fclose(file);
        return NULL;
    }
    // skip comments
    char c;
    while ((c = fgetc(file)) == '#')
    {
        while (fgetc(file) != '\n')
            ;
    }
    ungetc(c, file);
 
    // read wigth, height, max value of cell
    int width, height;
    if (fscanf(file, "%d %d %d", &width, &height, maxVal) != 3)
    {
        fprintf(stderr, "Error reading image parameters.\n");
        fclose(file);
        return NULL;
    }
 
    fgetc(file); // skip new string symbol
 
    Image *image = (Image *)malloc(sizeof(Image));
    if (!image)
    {
        fprintf(stderr, "Memory error.\n");
        fclose(file);
        return NULL;
    }
    image->width = width;
    image->height = height;
    image->data = (Cell *)malloc(width * height * sizeof(Cell));
    if (!image->data)
    {
        fprintf(stderr, "Memory error.\n");
        fclose(file);
        free(image);
        return NULL;
    }
    // read cell components
    if (fread(image->data, 1, width * height * 3, file) != width * height * 3)
    {
        fprintf(stderr, "Error reading image data.\n");
        fclose(file);
        free(image);
        return NULL;
    }
 
    fclose(file);
    return image;
}
 
void writePPM(const char *filename, Image *image, int maxval)
{
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        fprintf(stderr, "Error open file.\n");
        return;
    }
 
    fprintf(file, "P6\n%d %d\n%d\n", image->width, image->height, maxval);
 
    fwrite((Cell *)image->data, 1, (int)image->width * image->height * 3, file);
    fclose(file);
}
 
void writeHistogram(const char *filename, int *histogram)
{
    if (!histogram)
    {
        fprintf(stderr, "Error output.\n");
        return;
    }
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        fprintf(stderr, "Error openinhg file.\n");
        return;
    }
    fprintf(file, "%d", histogram[0]);
    for (int i = 1; i < 5; i++)
    {
        fprintf(file, " %d", histogram[i]);
    }
    fclose(file);
}
 
/* void pixelHistogram(int *histogram, Cell cell)
{
    long Y = (2126L * cell.red + 7152L * cell.green + 722L * cell.blue);
    unsigned char brightness = (unsigned char)((Y + 5000) / 10000);
    ;
    switch (brightness / 51)
    {
    case 0:
        histogram[0]++;
        break;
    case 1:
        histogram[1]++;
 
        break;
    case 2:
        histogram[2]++;
 
        break;
    case 3:
        histogram[3]++;
 
        break;
    default:
        histogram[4]++;
 
        break;
    }
} */
 
Cell masking(Image *img, size_t curPixel)
{
    Cell newCell;
 
    int topIndex = curPixel - img->width;
    int bottomIndex = curPixel + img->width;
    int leftIndex = curPixel - 1;
    int rightIndex = curPixel + 1;
 
    int r = (5 * img->data[curPixel].red) - img->data[topIndex].red - img->data[bottomIndex].red - img->data[leftIndex].red - img->data[rightIndex].red;
    int g = (5 * img->data[curPixel].green) - img->data[topIndex].green - img->data[bottomIndex].green - img->data[leftIndex].green - img->data[rightIndex].green;
    int b = (5 * img->data[curPixel].blue) - img->data[topIndex].blue - img->data[bottomIndex].blue - img->data[leftIndex].blue - img->data[rightIndex].blue;
 
    /* Cell newCell;
    Cell *data = image->data;
    int width = image->width;
    int r = 0, g = 0, b = 0;
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            int neighborCell = ((cell - data) + (i * width) + j);
            r += mask[i + 1][j + 1] * data[neighborCell].red;
            g += mask[i + 1][j + 1] * data[neighborCell].green;
            b += mask[i + 1][j + 1] * data[neighborCell].blue;
        }
    } */
    // checking range
    /* newCell.red = (unsigned char)fmin(fmax(r, 0), 255);
    newCell.green = (unsigned char)fmin(fmax(g, 0), 255);
    newCell.blue = (unsigned char)fmin(fmax(b, 0), 255); */
 
    if (r < 0)
    {
        newCell.red = 0;
    }
    else if (r > 255)
    {
        newCell.red = 255;
    }
    else
    {
        newCell.red = (unsigned char)r;
    }
 
    if (g < 0)
    {
        newCell.green = 0;
    }
    else if (g > 255)
    {
        newCell.green = 255;
    }
    else
    {
        newCell.green = (unsigned char)g;
    }
 
    if (b < 0)
    {
        newCell.blue = 0;
    }
    else if (b > 255)
    {
        newCell.blue = 255;
    }
    else
    {
        newCell.blue = (unsigned char)b;
    }
 
    return newCell;
}
 
Image processImage(Image *image, int *histogram)
{
    Image newImage;
    newImage.width = image->width;
    newImage.height = image->height;
    newImage.data = (Cell *)malloc(sizeof(Cell) * image->width * image->height);
    // first line copy
    // oneStringProcess(newImage.data, image->data, image->width, histogram);
    memcpy(newImage.data, image->data, image->width * sizeof(Cell));
    for (int i = 0; i < image->width; i++)
    {
        // pixelHistogram(histogram, image->data[i]);
        long Y = (2126L * image->data[i].red + 7152L * image->data[i].green + 722L * image->data[i].blue);
        unsigned char brightness1 = (unsigned char)((Y + 5000) / 10000);
        ;
        switch (brightness1 / 51)
        {
        case 0:
            histogram[0]++;
            break;
        case 1:
            histogram[1]++;
 
            break;
        case 2:
            histogram[2]++;
 
            break;
        case 3:
            histogram[3]++;
 
            break;
        default:
            histogram[4]++;
 
            break;
        }
    }
    for (int y = 1; y < newImage.height - 1; y++)
    {
        /* processEdges(image, &newImage, histogram, y); */
        int left = y * image->width;
        int right = left + image->width - 1;
        newImage.data[left] = image->data[left];
        newImage.data[right] = image->data[right];
        // pixelHistogram(histogram, image->data[left]);
        long X = (2126L * image->data[left].red + 7152L * image->data[left].green + 722L * image->data[left].blue);
        unsigned char brightness2 = (unsigned char)((X + 5000) / 10000);
        ;
        switch (brightness2 / 51)
        {
        case 0:
            histogram[0]++;
            break;
        case 1:
            histogram[1]++;
 
            break;
        case 2:
            histogram[2]++;
 
            break;
        case 3:
            histogram[3]++;
 
            break;
        default:
            histogram[4]++;
 
            break;
        }
        // pixelHistogram(histogram, image->data[right]);
        long Z = (2126L * image->data[right].red + 7152L * image->data[right].green + 722L * image->data[right].blue);
        unsigned char brightness3 = (unsigned char)((Z + 5000) / 10000);
        ;
        switch (brightness3 / 51)
        {
        case 0:
            histogram[0]++;
            break;
        case 1:
            histogram[1]++;
 
            break;
        case 2:
            histogram[2]++;
 
            break;
        case 3:
            histogram[3]++;
 
            break;
        default:
            histogram[4]++;
 
            break;
        }
 
        /* applyMask(image, &newImage, histogram, pixHistogram, y); */
        for (int x = 1; x < newImage.width - 1; x++)
        {
            int currCell = newImage.width * y + x;
            newImage.data[currCell] = masking(image, currCell);
            // pixelHistogram(histogram, newImage.data[currCell]);
            long A = (2126L * newImage.data[currCell].red + 7152L * newImage.data[currCell].green + 722L * newImage.data[currCell].blue);
            unsigned char brightness4 = (unsigned char)((A + 5000) / 10000);
            ;
            switch (brightness4 / 51)
            {
            case 0:
                histogram[0]++;
                break;
            case 1:
                histogram[1]++;
 
                break;
            case 2:
                histogram[2]++;
 
                break;
            case 3:
                histogram[3]++;
 
                break;
            default:
                histogram[4]++;
 
                break;
            }
        }
    }
 
    // last line copy
    // oneStringProcess(newImage.data + (image->height - 1) * image->width, image->data + (image->height - 1) * image->width, image->width, histogram);
    memcpy(newImage.data + (image->height - 1) * image->width, image->data + (image->height - 1) * image->width, image->width * sizeof(Cell));
    for (int i = 0; i < image->width; i++)
    {
        Cell last_cells = (image->data + (image->height - 1) * image->width)[i];
       // pixelHistogram(histogram, (image->data + (image->height - 1) * image->width)[i]);
        long XYZ = (2126L * last_cells.red + 7152L * last_cells.green + 722L * last_cells.blue);
    unsigned char brightness = (unsigned char)((XYZ + 5000) / 10000);
    ;
    switch (brightness / 51)
    {
    case 0:
        histogram[0]++;
        break;
    case 1:
        histogram[1]++;
 
        break;
    case 2:
        histogram[2]++;
 
        break;
    case 3:
        histogram[3]++;
 
        break;
    default:
        histogram[4]++;
 
        break;
    }
         
    }
    return newImage;
}
