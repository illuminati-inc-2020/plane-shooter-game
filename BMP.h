/* Subject	: 24bit Windows Bitmap Image with 54 bit header handling (Part of Shooting game)
 * Author	: Rakesh Malik
 * Date		: 20/04/2011
 */

#include <stdlib.h>
#include <stdio.h>

typedef struct BMP
{
	/* Bitmap file header (14byte) */
	unsigned short 	signature;					// 0x4d42
	unsigned long 	size;
	unsigned short 	reserved1;					// 0
	unsigned short 	reserved2;					// 0
	unsigned long 	offset;
	/* Bitmap info header (40byte) */
	unsigned long 	bitmap_info_header_size;	// 40
	unsigned long 	width;
	unsigned long 	height;
	unsigned short 	planes;						// 1
	unsigned short 	bits_per_pixel;				// 1,4,9,24
	unsigned long 	compression_type;			//0=none,1=RLE8,2=RLE4
	unsigned long	image_data_size;
	unsigned long 	horizontal_resolution_per_meter;
	unsigned long 	vertical_resolution_per_meter;
	unsigned long 	number_of_colors;
	unsigned long 	number_of_important_colors;
	/* Bitmap data (24bit rgb format) */
	unsigned char *data;
}BMP;

void load_BMP(struct BMP *image,const char *fname);// Load a BMP file
void free_BMP(struct BMP *image);// Frees all allocated memory for a loaded BMP file
void print_BMP_info(struct BMP image);

/* Function Definitions */
void load_BMP(struct BMP *image,const char *fname)
{
	long i,j;
	FILE *fp;
	if((fp=fopen(fname,"rb"))==NULL)
	{
		char s[300];
		sprintf(s,"BMP file \"%s\" read error",fname);
		MessageBox(NULL,s,"Error",MB_ICONWARNING);
		exit(0);
	}
		
	fread(&image->signature,						sizeof(unsigned short),	1,fp);
	if(image->signature!=0x4d42)
	{
		char s[300];
		sprintf(s,"\"%s\" is not a BMP image file with 54 byte header and 24 bit rgb color format",fname);
		MessageBox(NULL,s,"Error",MB_ICONWARNING);
		exit(0);
	}
	fread(&image->size,								sizeof(unsigned long),	1,fp);
	fread(&image->reserved1,						sizeof(unsigned short),	1,fp);
	fread(&image->reserved2,						sizeof(unsigned short),	1,fp);
	fread(&image->offset,							sizeof(unsigned long),	1,fp);
	fread(&image->bitmap_info_header_size,			sizeof(unsigned long),	1,fp);
	fread(&image->width,							sizeof(unsigned long),	1,fp);
	fread(&image->height,							sizeof(unsigned long),	1,fp);
	fread(&image->planes,							sizeof(unsigned short),	1,fp);
	fread(&image->bits_per_pixel,					sizeof(unsigned short),	1,fp);
	fread(&image->compression_type,					sizeof(unsigned long),	1,fp);
	fread(&image->image_data_size,					sizeof(unsigned long),	1,fp);
	fread(&image->horizontal_resolution_per_meter,	sizeof(unsigned long),	1,fp);
	fread(&image->vertical_resolution_per_meter,	sizeof(unsigned long),	1,fp);
	fread(&image->number_of_colors,					sizeof(unsigned long),	1,fp);
	fread(&image->number_of_important_colors,		sizeof(unsigned long),	1,fp);
	
	image->data=(unsigned char*)calloc(sizeof(unsigned char),image->height*image->width*3);
				
	for(i=image->height-1;i>=0;i--)
	{
		for(j=0;j<image->width;j++)
		{			
			fread(&image->data[3*(i*image->width+j)+2],sizeof(unsigned char),1,fp);
			fread(&image->data[3*(i*image->width+j)+1],sizeof(unsigned char),1,fp);
			fread(&image->data[3*(i*image->width+j)+0],sizeof(unsigned char),1,fp);			
		}
	}
	
	fclose(fp);
}

void free_BMP(struct BMP *image)
{
	free(image->data);
}

void print_BMP_info(struct BMP image)
{
	printf("\nBITMAP HEADER FILE :\n");
	printf("Signature : %x\n",image.signature);
	printf("File size : %lu\n",image.size);
	printf("Reserved : %hu\n",image.reserved1);
	printf("Reserved : %hu\n",image.reserved2);
	printf("Offset : %lu\n",image.offset);
	printf("\nBITMAP HEADER INFO :\n");
	printf("Header info size : %lu\n",image.bitmap_info_header_size);
	printf("Resolution in pixel : %lux%lu\n",image.width,image.height);
	printf("Plane : %hu\n",image.planes);
	printf("Bits per pixel : %hu\n",image.bits_per_pixel);
	printf("Compression type : %hu\n",image.compression_type);
	printf("Image data size : %lu\n",image.image_data_size);
	printf("Resolution in meter : %lux%lu\n",image.horizontal_resolution_per_meter,image.vertical_resolution_per_meter);
	printf("Number of colors : %lu\n",image.number_of_colors);
	printf("Number of important colors : %lu\n",image.number_of_important_colors);
}