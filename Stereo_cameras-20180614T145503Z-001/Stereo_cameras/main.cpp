#include <opencv2/opencv.hpp>
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

#include "spi.h" 

#include "../leptonSDKEmb32PUB/LEPTON_SDK.h"
#include "../leptonSDKEmb32PUB/LEPTON_SYS.h"
#include "../leptonSDKEmb32PUB/LEPTON_Types.h"
#include "../leptonSDKEmb32PUB/LEPTON_AGC.h"

#define PACKET_SIZE 164
#define PACKET_SIZE_UINT16 (PACKET_SIZE/2)
#define PACKETS_PER_FRAME 60
#define FRAME_SIZE_UINT16 (PACKET_SIZE_UINT16*PACKETS_PER_FRAME)
#define FPS 25;

using namespace cv;
using namespace std;

int V_MAX = 65535;
int V_MIN = 0;

class Thermal
{
	private:
	
		LEP_CAMERA_PORT_DESC_T _port;
		
		uint8_t result[PACKET_SIZE*PACKETS_PER_FRAME];
		uint16_t *frameBuffer;
		
	public:
		
		Thermal()
		{
			Spi_Open_port();
			//Open I2C for communications
			LEP_OpenPort(1, LEP_CCI_TWI, 400, &_port);
			cout << "Thermal Object is being Created" << endl;
		}
		
		~Thermal()
		{
			Spi_Close_port();
			cout << "Thermal Object is being deleted" << endl;
		}
	
		Mat timg = Mat(Size(400, 300), CV_8UC1);
		Mat thermal_image = Mat(Size(80, 60), CV_8UC1);
		Mat thermal_image_16 = Mat(Size(80, 60), CV_16UC1);
		
        Mat thresh;
        Mat_<Vec3b> colored_img(80, 60, Vec3b(0,0,0));
        
		uint16_t minValue;
		uint16_t maxValue;
		
		void getThermalFrame()
		{
			//read data packets from lepton over SPI
			int resets = 0;
			for(int j = 0; j < PACKETS_PER_FRAME; j++)
			{
				//if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
				read(spi_cs0_fd, result + sizeof(uint8_t) * PACKET_SIZE * j, sizeof(uint8_t) * PACKET_SIZE);
				int packetNumber = result[j * PACKET_SIZE + 1];
				if(packetNumber != j)
				{
					j = -1;
					resets += 1;
					usleep(1000);

					//Note: we've selected 750 resets as an arbitrary limit, since there should never be 750 "null" packets between two valid transmissions at the current poll rate
					//By polling faster, developers may easily exceed this count, and the down period between frames may then be flagged as a loss of sync
					if(resets == 750)
					{
						Spi_Close_port();
						usleep(750000);
						Spi_Open_port();
					}
				}
			}
			
			if(resets >= 30)
			{
				//cout << "done reading, resets: " << resets << endl;
			}

			frameBuffer = (uint16_t *)result;
			int row, column;
			uint16_t value;
			minValue = 65535;
			maxValue = 0;
			float temp;

			
			for(int i=0;i<FRAME_SIZE_UINT16;i++)
			{
				//skip the first 2 uint16_t's of every packet, they're 4 header bytes
				if(i % PACKET_SIZE_UINT16 < 2)
				{
					continue;
				}
				
				//flip the MSB and LSB at the last second
				int temp = result[i*2];
				result[i*2] = result[i*2+1];
				result[i*2+1] = temp;
				
				value = frameBuffer[i];
				if(value > maxValue)
				{
					maxValue = value;
				}
				if(value < minValue)
				{
					minValue = value;
				}
			}

			float diff = maxValue - minValue;
			float scale = 255/diff;
			
			for(int i=0;i<FRAME_SIZE_UINT16;i++)
			{
				if(i % PACKET_SIZE_UINT16 < 2)
				{
					continue;
				}
				
				value = (frameBuffer[i] - minValue) * scale;
				
				column = (i % PACKET_SIZE_UINT16 ) - 2;
				row = i / PACKET_SIZE_UINT16;
				colored_img(row,column) = Vec3b(value/2,255,255);
				thermal_image.ptr()[row * thermal_image.cols + column] = value;	
				unsigned short *buf = (unsigned short *)thermal_image_16.ptr();
				buf[row * thermal_image_16.cols + column] = frameBuffer[i];
			}
            cvtColor(colored_img, colored_img, CV_HSV2RGB);
			resize(thermal_image, timg, Size(400, 300));
            resize(colored_img, colored_img, Size(400,300));
            //threshold
            threshold( thermal_image_16, thresh, V_MIN, 255, 3);//abaixo de MIM vai para 0
            threshold( thresh, thresh, V_MAX, 255, 4);          //a cima de MAX vai para 0
            threshold( thresh, thresh, V_MIN, 255, 2);          //a cima de MIN vai para 255
            resize(thresh, thresh, Size(400,300));
            //filtrar se necessário
            //Mat element = getStructuringElement(2, Size(4,4), Point(4,4));
            //morphologyEx(thresh, thresh, 2, element);
		}
		
		void FFCperform(){
			LEP_RunSysFFCNormalization(&_port);
		}
};



int main( int argc, char **argv )
{	
	cout << "PRESS 'ESC' TO EXIT" << endl;
	cout << "PRESS 'SPACE' TO FFC Perform" << endl;
	
	Thermal ir;
	
	while(true)
	{	
        creatTrackbars();
        
		ir.getThermalFrame();

		namedWindow("Thermal Image2");
		imshow("Thermal Image2", ir.colored_img);
		
        namedWindow("threshold");
		imshow("threshold", ir.thresh);
        
		int key = waitKey(10);
		
		if(key == 32) //"Space"
		{
			ir.FFCperform();
		}
		if(key == 27) //"Esc"
		{
			break;
		}
		//if(key == 9) // "Tab"
		//{
			//cout << "Min " << ir.minValue << " Max " << ir.maxValue << endl;
		//}
		//if(key == 92) // "\"
		//{
			//cout << "Size > " << test.size << endl;
		//}
		
	}

	return 0;
}

void creatTrackbars(){
    namedWindow("trackbars", 0);
    char TrackbarName[50];
    sprintf(TrackbarName, "V_MIN", V_MIN);
    sprintf(TrackbarName, "V_MAX", V_MAX);
    creatTrackbar("V_MIN", "trackbars", &V_MIN, V_MAX, on_trackbar);
    creatTrackbar("V_MAX", "trackbars", &V_MAX, V_MAX, on_trackbar);
}
