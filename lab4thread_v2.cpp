/*--------------- lab4thread.cpp ---------------
by:	Mr. Ryan D Aldrich
	Mr. Taric S Alani
	Mr. John A Nicholson
EECE.5520-201 Microprocessors II
ECE Dept.
UMASS Lowell
Based on specification by Dr. Yan Luo:
lab4mtopencv.pdf 2016-11-25 11:48:28

PURPOSE
Learn how to design multithreaded programs for embedded
multicore systems.  Understand synchronization and mutual
exclusion.  Learn POSIX thread libraries.  Learn HTTP 
protocol and libcurl library.  Understand image processing
concepts.  Implement image processing functions.     
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <curl/curl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "sensor.h"

using namespace cv;
using namespace std;


#define Strobe					(40)	// GPIO40, pin 8
#define GP_4					(6)		// GPIO6, pin 4
#define GP_5					(0)		// GPIO0, pin 5
#define GP_6					(1)		// GPIO1, pin 6
#define GP_7					(38)	// GPIO38, pin 7
#define GPIO_DIRECTION_IN		(1)  
#define GPIO_DIRECTION_OUT		(0)
#define ERROR					(-1)
#define HIGH					(1)
#define LOW						(0)

/* user commands */
#define	MSG_RESET	0x0			/* reset the sensor to initial state */
#define MSG_PING	0x1			/* check if the sensor is working properly */
#define MSG_GET		0x2			/* obtain the most recent ADC result */

/* I2C address defines */
#define TEMP_ADDR	0x48		// temp sensor address

// declare data read variables
int gp4;
int gp5;
int gp6;
int gp7;

//Training data and structures for face detection
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;

unsigned int adcdata = 0;			// initialize adc data variable

float temp_extval = 0.0;           // initialize temperature in Fahrenheit

void *thread_Command(void *arg);	    // function for command interface
void *thread_Sensor(void *arg);	        // function for sensor control
void *thread_Communication(void *arg);  // function for server communication
void *thread_Image(void *arg);          // function for image processing

pthread_mutex_t mutxa; // mutex for command thread
pthread_mutex_t mutxb; // mutex for sensor thread
pthread_mutex_t mutxc; // mutex for image thread 

char thread1[]="Command Interface thread";
char thread2[]="Sensor Control thread";
char thread3[]="Communication thread";
char thread4[]="Image Processing thread";

// system clock timer
void timer(unsigned int ms)
{
	// timer local variables
    clock_t start, end;
	long stop;

    stop = ms*(CLOCKS_PER_SEC/1000);
    start = end = clock();
	//do not use for(), cause it needs 3 clock cycle to execute.
    while((end - start) < stop) 
        end = clock();
}

//open GPIO and set the direction
void openGPIO(int gpio, int direction )
{		
        //  initialize character arrays for system commands and clear them
        char buf[] = {0,0,0,0}; // 4 = size of "out" plus '\0'
		char str_dir_set[47] = {0};  // 47 = max characters for system call
		
		// detect direction from input arguement
		if (direction == GPIO_DIRECTION_IN)
		{
			strcat(buf,"in");  
		}
		else if (direction == GPIO_DIRECTION_OUT)
		{
			strcat(buf,"out");
		}
		//Switch on gpio and create direction system commands.
		//Direction is a file that contains a string, either "out" or "in".
		switch(gpio)
		{
			case Strobe:
			sprintf	(
					str_dir_set,
					"echo -n %s > /sys/class/gpio/gpio40/direction",
					buf
					);
			system(str_dir_set);
			case GP_4: // pin 4 requires a linux and level shifter GPIO
			sprintf	(
					str_dir_set,
					"echo -n %s > /sys/class/gpio/gpio6/direction",
					buf
					);
			system(str_dir_set);
			sprintf	(
					str_dir_set,
					"echo -n %s > /sys/class/gpio/gpio36/direction",
					buf
					);
			system(str_dir_set);
			case GP_5: // pin 5 requires a linux and level shifter GPIO
			sprintf	(
					str_dir_set,
					"echo -n %s >/sys/class/gpio/gpio0/direction",
					buf
					);
			system(str_dir_set);
			sprintf	(
					str_dir_set,
					"echo -n %s > /sys/class/gpio/gpio18/direction",
					buf
					);
			system(str_dir_set);
			case GP_6:  // pin 6 requires a linux and level shifter GPIO
			sprintf	(
					str_dir_set,
					"echo -n %s > /sys/class/gpio/gpio1/direction",
					buf
					);
			system(str_dir_set);
			sprintf	(
					str_dir_set,
					"echo -n %s > /sys/class/gpio/gpio20/direction",
					buf
					);
			system(str_dir_set);
			case GP_7:
			sprintf	(
					str_dir_set,
					"echo -n %s > /sys/class/gpio/gpio38/direction",
					buf
					);
			system(str_dir_set);
		}
		
}

//write value
void writeGPIO(int gpio, int value)
{
	// initialize system call string for setting the value
	// value is a file that contains a string that is either "0" or "1"
    char str_st1[50] = {0};  // 50 = size of system() call
	
	// write the first portion of the value system() call
    if (value==HIGH) // if value is HIGH
    {
		strcat(str_st1,"echo -n \"1\" > /sys/class/gpio/");
    }
    else if (value==LOW) // if value is LOW
    {
		strcat(str_st1,"echo -n \"0\" > /sys/class/gpio/");
    }
	// switch on gpio and create value system commands
	// writing to the value file requires the Linux GPIO
    switch (gpio)
    {
		case Strobe:
			strcat(str_st1,"gpio40/value\n");
			system(str_st1);
			break;
        case GP_4:
            strcat(str_st1,"gpio6/value\n");
            system(str_st1);
            break;
        case GP_5:
            strcat(str_st1,"gpio0/value\n");
            system(str_st1);
            break;
        case GP_6:
            strcat(str_st1,"gpio1/value\n");
            system(str_st1);
            break;
        case GP_7:
            strcat(str_st1,"gpio38/value\n");
            system(str_st1);
            break;
    }
        
}

// write commands on the bus to the PIC
void writeBUS(int command)
{
	// set pins 4-7 to the out direction
    openGPIO(GP_4, GPIO_DIRECTION_OUT);
    openGPIO(GP_5, GPIO_DIRECTION_OUT);
    openGPIO(GP_6, GPIO_DIRECTION_OUT);
    openGPIO(GP_7, GPIO_DIRECTION_OUT);
    
	// detect command and write GPIO values for that command
    if(command==MSG_RESET)
    {
        writeGPIO(GP_4,LOW);
        writeGPIO(GP_5,HIGH);
    }
    if (command==MSG_PING)
    {
        writeGPIO(GP_4,HIGH);
        writeGPIO(GP_5,LOW);
    }
    if(command==MSG_GET)
    {
        writeGPIO(GP_4,HIGH);
        writeGPIO(GP_5,HIGH);
    }
	
    timer(1);  // delay 1 microsecond
}

// read the output of the system() call that had printed
// the contents of value to the screen
void readBUS(void)
{   
    // declare file pointers
    FILE *fp1;
    FILE *fp2;
    FILE *fp3;
    FILE *fp4;
	
    // open value files on Linux system for reading to pointer
    fp1 = popen("cat /sys/class/gpio/gpio6/value", "r");
    fp2 = popen("cat /sys/class/gpio/gpio0/value", "r");
    fp3 = popen("cat /sys/class/gpio/gpio1/value", "r");
    fp4 = popen("cat /sys/class/gpio/gpio38/value", "r");
    
	// get values in files and put them in data read variables
    gp4 = (fgetc(fp1) & 1);
    gp5 = (fgetc(fp2) & 1);
    gp6 = (fgetc(fp3) & 1);
    gp7 = (fgetc(fp4) & 1);
	
	// close the system file pointers
	pclose(fp1);
	pclose(fp2);
	pclose(fp3);
	pclose(fp4);
	
	// print data values to the terminal screen as hex numbers
	printf("%x, %x, %x, %x\n", gp7, gp6, gp5, gp4);
}

// clear the BUS 
void clearBUS(void)
{
	// set pins 4-7 to high impedance
    openGPIO(GP_4, GPIO_DIRECTION_IN);
    openGPIO(GP_5, GPIO_DIRECTION_IN);
    openGPIO(GP_6, GPIO_DIRECTION_IN);
    openGPIO(GP_7, GPIO_DIRECTION_IN);
	
}

void HTTP_POST(const char* url, const char* image, int size){
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,(long) size);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, image);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
      			fprintf(stderr, "curl_easy_perform() failed: %s\n",
              			curl_easy_strerror(res));
		curl_easy_cleanup(curl);
	}
}

// Command thread
void *thread_Command(void *arg)
{
	size_t len = 0;						//Used for getline()
	char * msg = (char *)"Feeling it";	//Initilize the array with anything.
	
	while(1)
	{
		pthread_mutex_lock(&mutxb);		//Make thread_Communication() wait.
		printf("Temperature in fahrenheit, %f\n", temp_extval);
		pthread_mutex_unlock(&mutxb);	//Done waiting
	
		printf	(
				"Enter your option\n"
				"1.Reset\n"
				"2.Ping\n"
				"3.Message Get\n"
				);
		getline(&msg, &len, stdin);		//Get user input, and store it in msg.
		
		/*The following code communicates with the PICKit3, using the Intel
		Galileo's General Purpose Input-Outpit (GPIO) pins. For propper
		operation, it's necessary to first Reset, then Ping, then finally
		Message Get. Once Message Get completes, the ADC value will show
		on screen.*/
		
		if(!strncmp(msg,"1",1))
		{//Door #1 	
			writeGPIO(Strobe, LOW);
			writeBUS(MSG_RESET);	
			writeGPIO(Strobe, HIGH);
			timer(10);
			writeGPIO(Strobe, LOW);
			clearBUS();
			writeGPIO(Strobe, HIGH);
			timer(1);
			readBUS();
			if((gp4&&gp5&&gp6&&gp7)==1)  // ACK is 1111
			{
				printf("Acknowledge MSG_RESET\n");
			}
			writeGPIO(Strobe, LOW);
			clearBUS();
		}
	    else if(!strncmp(msg,"2",1))
		{//Door #2
			writeGPIO(Strobe, LOW);
			writeBUS(MSG_PING);	
			writeGPIO(Strobe, HIGH);
			timer(10);
			writeGPIO(Strobe, LOW);
			clearBUS();
			writeGPIO(Strobe, HIGH);
			timer(1);
			readBUS();
			if((gp4&&gp5&&gp6&&gp7)==1)  // ACK is 1111
			{
				printf("Acknowledge MSG_PING\n");
			}
			writeGPIO(Strobe, LOW);
			clearBUS();
		}
		else if(!strncmp(msg,"3",1))
		{//Door #3     
			pthread_mutex_lock(&mutxa); //thread_Communication() waits.
			writeGPIO(Strobe, LOW);
			clearBUS();
			writeBUS(MSG_GET);
			timer(1);
			writeGPIO(Strobe, HIGH);
			writeGPIO(Strobe, LOW);
			clearBUS();
			writeGPIO(Strobe, HIGH);
			timer(10);
			readBUS();
			if((gp4&&gp5&&gp6&&gp7)==1)  // ACK is 1111
			{
				printf("Acknowledge MSG_GET\n");
			}
			writeGPIO(Strobe, LOW);
			clearBUS();
			writeGPIO(Strobe, HIGH);
			timer(10);
			readBUS();
			adcdata = gp4;
			adcdata = ((gp5<<1)+adcdata);
			adcdata = ((gp6<<2)+adcdata);
			adcdata = ((gp7<<3)+adcdata);
			writeGPIO(Strobe, LOW);
			clearBUS();
			writeGPIO(Strobe, HIGH);
			timer(10);
			readBUS();
			adcdata = ((gp4<<4)+adcdata);
			adcdata = ((gp5<<5)+adcdata);
			adcdata = ((gp6<<6)+adcdata);
			adcdata = ((gp7<<7)+adcdata);
			writeGPIO(Strobe, LOW);
			clearBUS();
			writeGPIO(Strobe, HIGH);
			timer(10);
			readBUS();
			adcdata = ((gp4<<8)+adcdata);
			adcdata = ((gp5<<9)+adcdata);
			writeGPIO(Strobe, LOW);
			clearBUS();
			printf("ADC Value = %d\n", adcdata);
			pthread_mutex_unlock(&mutxa); //Done waiting
		}
		 else
		{//ZONK! The user did not enter 1, 2 or 3.
			printf("Invalid input. Enter another input\n");
		}	
	}
}

// Sensor thread
void *thread_Sensor(void *arg)
{
    // declare variables
	int fd;
	int r;
	unsigned char i;
	useconds_t delay = 2000;
    unsigned short temp_val[2] = {0,0};
	
	char *dev = (char *)"/dev/i2c-0";
	printf("Sensor capture running\n");
	
	while(1)
	{
		// open the I2C channel
		fd = open(dev, O_RDWR );
		if(fd < 0)
		{
			perror("Opening i2c device node\n");
		}
		
		// select temperature sensor as the I2C slave device
		r = ioctl(fd, I2C_SLAVE, TEMP_ADDR);
		if(r < 0)
		{
			perror("Selecting i2c device\n");
		}

		// read out data bytes from the temperature sensor
		for(i = 0; i < 2; i++)
		{
			r = read(fd, &temp_val[i], 2);
			if(r != 2)
			{
				perror("reading i2c device\n");
			}
			usleep(delay);
		}
		pthread_mutex_lock(&mutxb); 		//thread_Communication waits.
		temp_extval=temp_math(temp_val[0]);
		pthread_mutex_unlock(&mutxb);		//Done waiting
		
		// close I2C channel
		close(fd);
	}
}

void *thread_Communication(void *arg)
{
	char timestamp[18] = {0};	//text output of timestamp
	time_t rawTime;		//number of seconds since the UNIX epoch	
	struct tm * ptm;	//structure pointer of time for strftime
	
	while (1)
	  {	
		//First check if a file is already on the SD card.
		if(!system("ls Image.png 2> /dev/null")) //Don't print stderror.
		{//If there is a file on the SD card, rename it.
			system("mv Image.png Image1.png 2> /dev/null");
		}
		
		while(system("ls Image.png 2> /dev/null"))	 
		{  //Stay and check every ten seconds, until there is a new photo.
		   sleep(10);
		}
		
		// Get the number of seconds since 19700101-00:00:00
		time ( (time_t *)&rawTime );		// Store it in rawTime.
		ptm = gmtime( (time_t *)&rawTime);  // Format into UTC time.
		ptm->tm_hour = ptm->tm_hour-5; 		// Adjust the time zone to EST.
		
		strftime	(//Put this in a human-readable format.
					timestamp,
					18,
					"%Y%m%d-%H:%M:%S", //YYYYMMDD-hh:mm:ss
					ptm
					);
					
		pthread_mutex_lock(&mutxa);					//thread_Command() waits.
		pthread_mutex_lock(&mutxb);					//thread_Sensor() waits.
		
		/*The following reqquires an IP address either on the same local
		network of the client, or a public IP and propper port forwarding and
		Network Address	Translation (NAT). Since eduroam does not give its
		users access to port forwarding, this requires a local IP. It also
		requires modifying authfile.py with the propper password, shown below.*/
		const    char* hostname = "10.253.84.169";	//IP address of server
		const    int   port = 8000; 				//Server communication port
		const    int   id = 5;						//Group 005
		const    char* password = "UMass.Lowell2016";
		const    char* name = "5";					//Group 005
		const    int   adcval = (int) adcdata;		//ADC value
		const    float status   = temp_extval;		//Degrees Farenheight
		const 	 char* filename="Image.png";		
		
		char buf[1024];	//Storage for URL
		snprintf	(//Prepare the URL for HTTP_POST()
					buf,
					1024,
					"http://%s:%d/"	//	http://10.253.84.169:8000/
					"update?id=%d"
					"&password=%s"
					"&name=%s"
					"&data=%d"
					"&status=%f"
					"&timestamp=%s"
					"&filename=%s",
					hostname, port,
					id,
					password,
					name,
					adcval,
					status,
					timestamp,
					filename
					);
		pthread_mutex_unlock(&mutxa);	//thread_Command() is done waiting.
		pthread_mutex_unlock(&mutxb);	//thread_Sensor() is done waiting.
		
		FILE *fp;	//Declare the file pointer for the photo.
	
		struct stat num;		//Declare to eventually get file size.
		stat(filename, &num);	//The st_size member of num has the file size.
		int size = num.st_size;	//Store that member in "size".
		printf	(//Show the client how many bytes the photo is.
				"Image size: %dB\n",
				size
				);	

		//Create enough space to load the photo into memory.
		char *buffer = (char*)malloc(size);

		fp = fopen(filename,"rb");	//Open the photo as binary.
		int n = fread	(//Read the file.
						buffer,	//Store the file in buffer.
						1,		//Allows n to equal the number of bytes read
						size,	//How big the opened file is
						fp		//The photo is the opened file.
						); 
		/*	HTTP_POST() will send the metadata viewable at
			http://10.253.84.169:8000/ to anyone on the network,
			and it will send the image file to the server.	*/
		HTTP_POST	(//Send over the network.
					buf,	//URL to make the metadata
					buffer,	//Image file
					size	//Image file size
					);
		fclose(fp);					//Close the photo.
		
	  }
	  
}

void detectAndDisplay( Mat frame )
{
	useconds_t delay = 2000;
	
    std::vector<Rect> faces;
    Mat frame_gray;
	//vector that stores the compression parameters of the image
    vector<int> compression_params;
	//specify the compression technique
	compression_params.push_back(IMWRITE_PNG_COMPRESSION); 
	//specify the compression quality
	compression_params.push_back(0);

    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );

    //-- Detect faces
    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0, Size(80, 80) );

    for( size_t i = 0; i < faces.size(); i++ )
    {
        Mat faceROI = frame_gray( faces[i] );
        std::vector<Rect> eyes;

        //-- In each face, detect eyes
        eyes_cascade.detectMultiScale	(
										faceROI,
										eyes,
										1.1,
										2,
										0 |CASCADE_SCALE_IMAGE,
										Size(30, 30)
										);
        if( eyes.size() == 2)
        {
			printf("Face detected!\n");
            //-- Draw the face
            Point center	(
							faces[i].x + faces[i].width/2,
							faces[i].y + faces[i].height/2
							);
            ellipse	(
					frame,
					center,
					Size( faces[i].width/2, faces[i].height/2 ),
					0,
					0,
					360,
					Scalar( 255, 0, 0 ),
					2,
					8,
					0
					);

            for( size_t j = 0; j < eyes.size(); j++ )
            { //-- Draw the eyes
                Point eye_center	(
									faces[i].x + eyes[j].x + eyes[j].width/2,
									faces[i].y + eyes[j].y + eyes[j].height/2
									);
                int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
                circle	(
						frame,
						eye_center,
						radius,
						Scalar( 255, 0, 255 ),
						3,
						8,
						0
						);
            }
            
			//The following line of code had been used for debugging.
			//bool bSuccess1 = cap.read(frame);
			
			// read a new frame from video and store it in img
			bool bSuccess2 = imwrite	(//write the frame image to file
										"/media/card/Image.png",
										frame,
										compression_params
										); 
			usleep(delay);
			printf("Image saved to file, /media/card/Image.png\n");
        }

    }
}

void *thread_Image(void *arg)
{
	
	VideoCapture cap(0); // open the video camera no. 0
	
	//-- 1. Load the cascades
    if( !face_cascade.load( face_cascade_name ) )
	{
		printf("--(!)Error loading face cascade\n");
	};
    if( !eyes_cascade.load( eyes_cascade_name ) )
	{
		printf("--(!)Error loading eyes cascade\n"); 
	};

    //-- 2. Read the video stream
    cap.open( -1 );
    if ( ! cap.isOpened() ) { printf("--(!)Error opening video capture\n"); }
	
	//Get the width of frames of the video.
	double dWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	//Get the height of frames of the video.
	double dHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	//Display size of the image.
	cout << "Frame Size = " << dWidth << "x" << dHeight << endl;

	//Create an image with dHeight x dWidth size and CV_8UC4 array type.
	Mat img(dHeight, dWidth, CV_8UC4);
	
	while ( cap.read(img) )
    {
        if( img.empty() )
        {
            printf(" --(!) No captured frame -- Break!");
            break;
        }
        
        //-- 3. Apply the classifier to the frame
        detectAndDisplay(img);
        
    }

}


int main(int argc, char **argv)
{
	//intially export all the pins
	char command[40] = {0};
	// pin 8 strobe
	strcpy( command, "echo -n \"40\" > /sys/class/gpio/export\n");
	system( command);
	// pin 4 D0 requires a linux and level shifter GPIO
	strcpy( command, "echo -n \"6\" > /sys/class/gpio/export\n");
	system(command);
	strcpy( command, "echo -n \"36\" > /sys/class/gpio/export\n");
	system(command);
	// pin 5 D1 requires a linux and level shifter GPIO
	strcpy( command, "echo -n \"0\" > /sys/class/gpio/export\n");
	system(command);
	strcpy( command, "echo -n \"18\" > /sys/class/gpio/export\n");
	system(command);
	// pin 6 D2 requires a linux and level shifter GPIO
	strcpy( command, "echo -n \"1\" > /sys/class/gpio/export\n");
	system(command);
	strcpy( command, "echo -n \"20\" > /sys/class/gpio/export\n");
	system(command);
	// pin 7 D3
	strcpy(command, "echo -n \"38\" > /sys/class/gpio/export\n");
	system(command);
	
	openGPIO(Strobe, GPIO_DIRECTION_OUT);  // set Strobe GPIO direction
	
	pthread_t t1, t2, t3, t4;  // create space for four threads
	void *thread_result;       // void pointer for thread result
	
	//Declare state variables for mutex initialization
	int state1, state2, state3;        
	
	//Mutex initialization
	state1 = pthread_mutex_init(&mutxa, NULL);  
	state2 = pthread_mutex_init(&mutxb, NULL);
	state3 = pthread_mutex_init(&mutxc, NULL);

	//Previously used for debugging
	//if(state1||state2!=0)
    //puts("Error mutex initialization!!!");

	// Create thread1, thread2, thread3, thread4
	pthread_create(&t1, NULL, thread_Command, (void *)&thread1);
	pthread_create(&t2, NULL, thread_Sensor, (void *)&thread2);
	pthread_create(&t3, NULL, thread_Communication, (void *)&thread3);
	pthread_create(&t4, NULL, thread_Image, (void *)&thread4);

	/*	Since all the threads run forever, the following will probably never
		execute, unless the system runs out of memory.	*/
	
	// Waiting thread to terminate
	pthread_join(t1, &thread_result);
	pthread_join(t2, &thread_result);
	pthread_join(t3, &thread_result);
	pthread_join(t4, &thread_result);

	printf	(
			"Terminate => %s, %s, %s!!!\n",
			&thread1,
			&thread2,
			&thread3,
			&thread4
			);
	
	//Destroy each mutex.
	pthread_mutex_destroy(&mutxa);
	pthread_mutex_destroy(&mutxb);
	pthread_mutex_destroy(&mutxc);
  
	return 0;
}