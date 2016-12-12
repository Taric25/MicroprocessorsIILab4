all:
	g++ `pkg-config --cflags opencv` -pthread -lcurl lab4thread_v2.cpp sensor.cpp `pkg-config --libs opencv` -o lab4thread
.PHONY:clean
clean:
	rm lab4thread
