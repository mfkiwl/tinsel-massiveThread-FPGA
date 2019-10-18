// SPDX-License-Identifier: BSD-2-Clause
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/time.h>
#include <HostLink.h>
#include "boxes.h"

// Magnification factor when displaying heat map
const int MAG = 4;

// 256 x RGB colours representing heat intensities
int heat[] = {
  0x00, 0x00, 0x76, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x83,
  0x00, 0x00, 0x88, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x91, 0x00, 0x00, 0x95,
  0x00, 0x00, 0x9a, 0x00, 0x00, 0x9e, 0x00, 0x00, 0xa3, 0x00, 0x00, 0xa3,
  0x00, 0x00, 0xa7, 0x00, 0x00, 0xac, 0x00, 0x00, 0xb0, 0x00, 0x00, 0xb5,
  0x00, 0x00, 0xb9, 0x00, 0x00, 0xbe, 0x00, 0x00, 0xc2, 0x00, 0x00, 0xc7,
  0x00, 0x00, 0xcb, 0x00, 0x00, 0xd0, 0x00, 0x00, 0xd4, 0x00, 0x00, 0xd9,
  0x00, 0x00, 0xde, 0x00, 0x00, 0xe2, 0x00, 0x00, 0xe7, 0x00, 0x00, 0xeb,
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf4, 0x00, 0x00, 0xf9, 0x00, 0x00, 0xfd,
  0x00, 0x03, 0xff, 0x00, 0x07, 0xff, 0x00, 0x0c, 0xff, 0x00, 0x10, 0xff,
  0x00, 0x15, 0xff, 0x00, 0x19, 0xff, 0x00, 0x1e, 0xff, 0x00, 0x22, 0xff,
  0x00, 0x27, 0xff, 0x00, 0x2b, 0xff, 0x00, 0x30, 0xff, 0x00, 0x34, 0xff,
  0x00, 0x39, 0xff, 0x00, 0x3d, 0xff, 0x00, 0x42, 0xff, 0x00, 0x47, 0xff,
  0x00, 0x4b, 0xff, 0x00, 0x50, 0xff, 0x00, 0x54, 0xff, 0x00, 0x59, 0xff,
  0x00, 0x5d, 0xff, 0x00, 0x62, 0xff, 0x00, 0x66, 0xff, 0x00, 0x6b, 0xff,
  0x00, 0x6f, 0xff, 0x00, 0x74, 0xff, 0x00, 0x78, 0xff, 0x00, 0x7d, 0xff,
  0x00, 0x81, 0xff, 0x00, 0x86, 0xff, 0x00, 0x8a, 0xff, 0x00, 0x8f, 0xff,
  0x00, 0x93, 0xff, 0x00, 0x98, 0xff, 0x00, 0x9c, 0xff, 0x00, 0xa1, 0xff,
  0x00, 0xa5, 0xff, 0x00, 0xaa, 0xff, 0x00, 0xaf, 0xff, 0x00, 0xb3, 0xff,
  0x00, 0xb8, 0xff, 0x00, 0xbc, 0xff, 0x00, 0xc1, 0xff, 0x00, 0xc5, 0xff,
  0x00, 0xca, 0xff, 0x00, 0xce, 0xff, 0x00, 0xd3, 0xff, 0x00, 0xd7, 0xff,
  0x00, 0xdc, 0xff, 0x00, 0xe0, 0xff, 0x00, 0xe5, 0xff, 0x00, 0xe9, 0xff,
  0x00, 0xee, 0xff, 0x00, 0xf2, 0xff, 0x00, 0xf7, 0xff, 0x00, 0xfb, 0xff,
  0x00, 0xff, 0xff, 0x00, 0xff, 0xfa, 0x00, 0xff, 0xf5, 0x00, 0xff, 0xf1,
  0x00, 0xff, 0xec, 0x00, 0xff, 0xe7, 0x00, 0xff, 0xe3, 0x00, 0xff, 0xde,
  0x00, 0xff, 0xda, 0x00, 0xff, 0xd5, 0x00, 0xff, 0xd1, 0x00, 0xff, 0xcc,
  0x00, 0xff, 0xc8, 0x00, 0xff, 0xc3, 0x00, 0xff, 0xbf, 0x00, 0xff, 0xba,
  0x00, 0xff, 0xb6, 0x00, 0xff, 0xb1, 0x00, 0xff, 0xad, 0x00, 0xff, 0xa8,
  0x00, 0xff, 0xa4, 0x00, 0xff, 0x9f, 0x00, 0xff, 0x9b, 0x00, 0xff, 0x96,
  0x00, 0xff, 0x92, 0x00, 0xff, 0x8d, 0x00, 0xff, 0x89, 0x00, 0xff, 0x84,
  0x00, 0xff, 0x80, 0x00, 0xff, 0x7b, 0x00, 0xff, 0x76, 0x00, 0xff, 0x72,
  0x00, 0xff, 0x6d, 0x00, 0xff, 0x69, 0x00, 0xff, 0x64, 0x00, 0xff, 0x60,
  0x00, 0xff, 0x5b, 0x00, 0xff, 0x57, 0x00, 0xff, 0x52, 0x00, 0xff, 0x4e,
  0x00, 0xff, 0x49, 0x00, 0xff, 0x45, 0x00, 0xff, 0x40, 0x00, 0xff, 0x3c,
  0x00, 0xff, 0x37, 0x00, 0xff, 0x33, 0x00, 0xff, 0x2e, 0x00, 0xff, 0x2a,
  0x00, 0xff, 0x25, 0x00, 0xff, 0x21, 0x00, 0xff, 0x1c, 0x00, 0xff, 0x18,
  0x00, 0xff, 0x13, 0x00, 0xff, 0x0e, 0x00, 0xff, 0x0a, 0x00, 0xff, 0x05,
  0x00, 0xff, 0x01, 0x04, 0xff, 0x00, 0x08, 0xff, 0x00, 0x0d, 0xff, 0x00,
  0x11, 0xff, 0x00, 0x16, 0xff, 0x00, 0x1a, 0xff, 0x00, 0x1f, 0xff, 0x00,
  0x23, 0xff, 0x00, 0x28, 0xff, 0x00, 0x2c, 0xff, 0x00, 0x31, 0xff, 0x00,
  0x35, 0xff, 0x00, 0x3a, 0xff, 0x00, 0x3e, 0xff, 0x00, 0x43, 0xff, 0x00,
  0x47, 0xff, 0x00, 0x4c, 0xff, 0x00, 0x50, 0xff, 0x00, 0x55, 0xff, 0x00,
  0x5a, 0xff, 0x00, 0x5e, 0xff, 0x00, 0x63, 0xff, 0x00, 0x67, 0xff, 0x00,
  0x6c, 0xff, 0x00, 0x70, 0xff, 0x00, 0x75, 0xff, 0x00, 0x79, 0xff, 0x00,
  0x7e, 0xff, 0x00, 0x82, 0xff, 0x00, 0x87, 0xff, 0x00, 0x8b, 0xff, 0x00,
  0x90, 0xff, 0x00, 0x94, 0xff, 0x00, 0x99, 0xff, 0x00, 0x9d, 0xff, 0x00,
  0xa2, 0xff, 0x00, 0xa6, 0xff, 0x00, 0xab, 0xff, 0x00, 0xaf, 0xff, 0x00,
  0xb4, 0xff, 0x00, 0xb8, 0xff, 0x00, 0xbd, 0xff, 0x00, 0xc2, 0xff, 0x00,
  0xc6, 0xff, 0x00, 0xcb, 0xff, 0x00, 0xcf, 0xff, 0x00, 0xd4, 0xff, 0x00,
  0xd8, 0xff, 0x00, 0xdd, 0xff, 0x00, 0xe1, 0xff, 0x00, 0xe6, 0xff, 0x00,
  0xea, 0xff, 0x00, 0xef, 0xff, 0x00, 0xf3, 0xff, 0x00, 0xf8, 0xff, 0x00,
  0xfc, 0xff, 0x00, 0xff, 0xfd, 0x00, 0xff, 0xf9, 0x00, 0xff, 0xf4, 0x00,
  0xff, 0xf0, 0x00, 0xff, 0xeb, 0x00, 0xff, 0xe7, 0x00, 0xff, 0xe2, 0x00,
  0xff, 0xde, 0x00, 0xff, 0xd9, 0x00, 0xff, 0xd5, 0x00, 0xff, 0xd0, 0x00,
  0xff, 0xcb, 0x00, 0xff, 0xc7, 0x00, 0xff, 0xc2, 0x00, 0xff, 0xbe, 0x00,
  0xff, 0xb9, 0x00, 0xff, 0xb5, 0x00, 0xff, 0xb0, 0x00, 0xff, 0xac, 0x00,
  0xff, 0xa7, 0x00, 0xff, 0xa3, 0x00, 0xff, 0x9e, 0x00, 0xff, 0x9a, 0x00,
  0xff, 0x95, 0x00, 0xff, 0x91, 0x00, 0xff, 0x8c, 0x00, 0xff, 0x88, 0x00,
  0xff, 0x83, 0x00, 0xff, 0x7f, 0x00, 0xff, 0x7a, 0x00, 0xff, 0x76, 0x00,
  0xff, 0x71, 0x00, 0xff, 0x6d, 0x00, 0xff, 0x68, 0x00, 0xff, 0x63, 0x00,
  0xff, 0x5f, 0x00, 0xff, 0x5a, 0x00, 0xff, 0x56, 0x00, 0xff, 0x51, 0x00,
  0xff, 0x4d, 0x00, 0xff, 0x48, 0x00, 0xff, 0x44, 0x00, 0xff, 0x3f, 0x00,
  0xff, 0x3b, 0x00, 0xff, 0x36, 0x00, 0xff, 0x32, 0x00, 0xff, 0x2d, 0x00,
  0xff, 0x29, 0x00, 0xff, 0x24, 0x00, 0xff, 0x20, 0x00, 0xff, 0x1b, 0x00,
  0xff, 0x17, 0x00, 0xff, 0x12, 0x00, 0xff, 0x0e, 0x00, 0xff, 0x09, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int main()
{
  HostLink hostLink(USE_BOXES_X,USE_BOXES_Y);

  // Load application
  hostLink.boot("code.v", "data.v");

  // Start application
  hostLink.go();

  // 2D grid
  int NX = USE_BOXES_X*96;
  int NY = USE_BOXES_Y*64;
  int grid[NY][NX];

  // Initialise 2D grid
  for (int y = 0; y < NY; y++)
    for (int x = 0; x < NX; x++)
      grid[y][x] = 0;

  for (int n = 0; n < 1; n++) {
    // Fill 2D grid
    for (int i = 0; i < NX*NY; i++) {
      int flit[4];
      hostLink.recv(flit);
      grid[flit[1]][flit[0]] = flit[2];
    }

    FILE* fp = fopen("out.ppm", "w");
    if (fp == NULL) {
      fprintf(stderr, "Failed to open file 'out.ppm'\n");
    }

    // Emit PPM
    fprintf(fp, "P3\n%i %i\n255\n", MAG*NX, MAG*NY);
    for (int y = 0; y < MAG*NY; y++) {
      for (int x = 0; x < MAG*NX; x++) {
        if (x > 0 && y > 0 &&
              (x%(32*MAG) == 0 || y%(32*MAG) == 0))
          fprintf(fp, "255 255 255\n");
        else {
          uint32_t t = (uint32_t) grid[y/MAG][x/MAG] % 256;
          fprintf(fp, "%d %d %d\n", heat[t*3], heat[t*3+1], heat[t*3+2]);
        }
      }
    }

    fclose(fp);
  }

  return 0;
}