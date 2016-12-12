--------------- Assignment #4  ---------------
by:	Mr. Taric S Alani
	Mr. Ryan D Aldrich
	Mr. John A Nicholson
EECE.5520-201
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

SOURCE FILES
lab4thread_v2.c	- 	Galileo main program Client implementation
sensor.cpp		-	Galileo function for main program
sensor.h		-	Galielo header file for sensor.cpp 
makefile		-	Preceding program can be build using default make argument

Lab4.X			-	Directory of source files for PICKit3

server.py		-	Program Server implementation
authfile.py		-	Authentication definition for server.py

EXECUTABLES
lab4thread
server.py

Before compiling, modify "10.253.84.169" in lab4thread_v2.cpp to the address
of the machine that will be the server, such as "8.8.8.8" or "example.com".
Utilize make at the command line to build the client implementation.

On the PICKit3, use the IDE to run the source files in the Lab4.X directory.

Run the server by at the command line by building and executing the program:
python server.py

Next, run the client by executing the program on the Intel Galileo, with the USB
camera connected and shown in the filesystem as video0:
./lab4thread

On the client, choose option 1, followed by 2 and then 3. Point the camera at a
human face. Once the program detects a face, it will update http://hostname:8000
where hostname is the address of the server. Any machine on the network can
access the http://hostname:8000 website from virtually any web browser. The
program sends the image file to the server, in the same directory where
server.py is in the filesystem on that machine.

CHANGES
2016-12-12 TSA	-	created