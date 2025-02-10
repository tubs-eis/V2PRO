/*
 *  * Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
 *                    Technische Universitaet Braunschweig, Germany
 *                    www.tu-braunschweig.de/en/eis
 * 
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 * 
 */
// ################################################################
// # opencv_helper.h - Image transfer/conversion helper functions #
// # Make sure to have the correct opencv module loaded!          #
// #                                                              #
// # Stephan Nolting, Uni Hannover, 2017                          #
// ################################################################

#ifndef opencv_helper_h
#define opencv_helper_h

// opencv libraries
#include <opencv2/opencv.hpp>

using namespace cv;

// std libraries
#include <stdio.h>
#include <stdlib.h>

// prototypes
void opencv_image2file(char const* input_file, unsigned int width, unsigned int height, char const* output_file);
void opencv_image2array(char const* input_file, unsigned int width, unsigned int height, uint8_t* array_pnt);
void opencv_file2image(char const* input_file, unsigned int width, unsigned int height, char const* output_file);
void opencv_array2image(uint8_t* array_pnt, unsigned int width, unsigned int height, char const* output_file);

void array_to_file(uint8_t* array_pnt, int num_bytes, char const* file_name);
void file_to_array(void* array_pnt, int num_bytes, char const* file_name);

void opencv_memcpy32t8(uint8_t* src_pnt, uint8_t* dst_pnt, int num);
void opencv_memcpy8t32(uint8_t* src_pnt, uint8_t* dst_pnt, int num);

void opencv_memcpy16t8(uint8_t* src_pnt, uint8_t* dst_pnt, int num);
void opencv_memcpy8t16(uint8_t* src_pnt, uint8_t* dst_pnt, int num);


// ---------------------------------------------------------------------------------
// Convert image file to data file (1 byte/pixel)
// input_file:  File name (+path) of input image
// width:       X size of input image
// height:      Y size of input image
// output_file: File name (+path) of output data array
// ---------------------------------------------------------------------------------
void opencv_image2file(char const* input_file, unsigned int width, unsigned int height, char const* output_file)
{
	Mat image;
	image = imread( input_file, 1 );

	if (!image.data) {
		printf("Failed to read image '%s'!\n", input_file);
    return;
  }

	// resize image
	Size size(width, height);//the dst image size,e.g.100x100
	resize(image, image, size);//resize image

	// convert to grayscale
	Mat gray_image;
	cvtColor( image, gray_image, cv::COLOR_BGR2GRAY );

	FILE* out = fopen(output_file, "wb");
  if (out == NULL) {
    printf("Failed to read data array '%s'!\n", output_file);
    return;
  }

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			unsigned int tmp = 0;
			tmp = (unsigned int)gray_image.at<uchar>(y,x);
			fwrite(&tmp, 1, 1, out);
		}
	}

	fclose(out);
}

// ---------------------------------------------------------------------------------
// Convert image file to data array (1 byte/pixel)
// input_file:  File name (+path) of input image
// width:       X size of input image
// height:      Y size of input image
// array_pnt:   Pointer to output data array
// ---------------------------------------------------------------------------------
void opencv_image2array(char const* input_file, unsigned int width, unsigned int height, uint8_t* array_pnt)
{
	Mat image;
	image = imread( input_file, 1 );

	if (!image.data) {
		printf("Failed to read image '%s'!\n", input_file);
    return;
  }

	// resize image
	Size size(width, height);//the dst image size,e.g.100x100
	resize(image, image, size);//resize image

	// convert to grayscale
	Mat gray_image;
	cvtColor( image, gray_image, cv::COLOR_BGR2GRAY );

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			*array_pnt++ = (uint8_t)gray_image.at<uchar>(y,x);
		}
	}
}


// ---------------------------------------------------------------------------------
// Convert data file to image file (1 byte/pixel)
// input_file:  File name (+path) of input image
// width:       X size of input image
// height:      Y size of input image
// output_file: File name (+path) of output data array
// ---------------------------------------------------------------------------------
void opencv_file2image(char const* input_file, unsigned int width, unsigned int height, char const* output_file)
{
	Size size(width, height);
	Mat img(size, CV_8U);

	FILE* in = fopen(input_file, "rb");
  if (in == NULL) {
    printf("Failed to read data array '%s'!\n", input_file);
    return;
  }

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			int tmp;
			fread(&tmp, 1, 1, in);
			img.at<uchar>(y,x) = (unsigned char)tmp;
		}
	}

	fclose(in);

	imwrite(output_file, img);
}


// ---------------------------------------------------------------------------------
// Convert data file to image file (1 byte/pixel)
// array_pnt:   Pointer to input data array
// width:       X size of input image
// height:      Y size of input image
// output_file: File name (+path) of output data array
// ---------------------------------------------------------------------------------
void opencv_array2image(uint8_t* array_pnt, unsigned int width, unsigned int height, char const* output_file)
{
	Size size(width, height);
	Mat img(size, CV_8U);

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			img.at<uchar>(y,x) = (unsigned char)*array_pnt++;
		}
	}

	imwrite(output_file, img);
}


// ---------------------------------------------------------------------------------
// Dump data array to file
// array_pnt:   Pointer to input data array
// num_bytes:   Number of bytes
// file_name:   Output file name
// ---------------------------------------------------------------------------------
void array_to_file(uint8_t* array_pnt, int num_bytes, char const* file_name) {

  FILE *pfh = NULL;
  pfh = fopen(file_name, "wb");
  if (pfh == NULL) {
    printf("Unable to open file <%s>!\n", file_name);
    return;
  }

  fwrite(array_pnt, 1, num_bytes, pfh);

  fclose(pfh);
}


// ---------------------------------------------------------------------------------
// Copy file to array
// array_pnt:   Pointer to input data array
// num_bytes:   Number of bytes
// file_name:   Output file name
// ---------------------------------------------------------------------------------
void file_to_array(void* array_pnt, int num_bytes, char const* file_name) {

  FILE *pfh = NULL;
  pfh = fopen(file_name, "rb");
  if (pfh == NULL) {
    printf("Unable to open file <%s>!\n", file_name);
    return;
  }

  memset(array_pnt, 0, num_bytes);
  fread(array_pnt, 1, num_bytes, pfh);

  fclose(pfh);
}


// ---------------------------------------------------------------------------------
// Copy 32-bit pixel array to 8-bit pixel array
// src_pnt: Input array (32-bit)
// dst_pnt: Output array(8-bit)
// num:     Number of pixels
// ---------------------------------------------------------------------------------
void opencv_memcpy32t8(uint8_t* src_pnt, uint8_t* dst_pnt, int num)
{
  while(num--) {
    src_pnt+=3;
    *dst_pnt++ = *src_pnt++; // dismiss other bytes
  }
}


// ---------------------------------------------------------------------------------
// Copy 8-bit pixel array to 32-bit pixel array (fill with zero)
// src_pnt: Input array (8-bit)
// dst_pnt: Output array(32-bit)
// num:     Number of pixels
// ---------------------------------------------------------------------------------
void opencv_memcpy8t32(uint8_t* src_pnt, uint8_t* dst_pnt, int num)
{
  while(num--) {
    *dst_pnt++ = 0;
    *dst_pnt++ = 0;
    *dst_pnt++ = 0;
    *dst_pnt++ = *src_pnt++; // fill with zero
  }
}

// ---------------------------------------------------------------------------------
// Copy 26-bit pixel array to 8-bit pixel array
// src_pnt: Input array (26-bit)
// dst_pnt: Output array(8-bit)
// num:     Number of pixels
// ---------------------------------------------------------------------------------
void opencv_memcpy16t8(uint8_t* src_pnt, uint8_t* dst_pnt, int num)
{
  while(num--) {
    src_pnt++;
    *dst_pnt++ = *src_pnt++; // dismiss upper bytes
  }
}


// ---------------------------------------------------------------------------------
// Copy 8-bit pixel array to 26-bit pixel array (fill with zero)
// src_pnt: Input array (8-bit)
// dst_pnt: Output array(26-bit)
// num:     Number of pixels
// ---------------------------------------------------------------------------------
void opencv_memcpy8t16(uint8_t* src_pnt, uint8_t* dst_pnt, int num)
{
  while(num--) {
    *dst_pnt++ = 0;
    *dst_pnt++ = *src_pnt++; // fill with zero
  }
}


#endif // opencv_helper_h
