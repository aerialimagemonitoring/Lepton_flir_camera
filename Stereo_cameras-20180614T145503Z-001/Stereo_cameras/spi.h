// include guard
#ifndef SPI_H
#define SPI_H

#include <string>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <iostream>

extern unsigned char spi_mode; 
extern unsigned char spi_bitsPerWord;
extern unsigned int spi_speed;
extern int spi_cs0_fd;

int Spi_Open_port();
int Spi_Close_port();

#endif // SPI_H
