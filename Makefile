
CC = gcc
CFLAGS = -g -Wall

TARGET = httpserver
SRCS = main.c request.c keyvalue.c response.c

$(TARGET): 
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) 
	

clean:
	 rm httpserver
