CC=g++
CFLAGS += -Wall -Wextra -g -O3

OBJ = convm3u.o m3upipe.o main.o

BIN = convm3u_channel

$(BIN):  $(OBJ)
	$(CC) -o $(BIN) $(OBJ)

clean:
	rm -f $(BIN) *.o *~
