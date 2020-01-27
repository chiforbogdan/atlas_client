CC=gcc
CFLAGS=-Wall -w -lpcap -lpthread
TARGET =atlas_data_plane

all: $(TARGET)


$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c client.c ../logger/atlas_logger.c ../commands/atlas_command.h -o $(TARGET)

clean:
	$(RM) $(TARGET) 
