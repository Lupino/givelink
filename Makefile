SRC=crc16.c \
	givelink.c \
	unhex.c \
	test/main.c

CFLAGS=-D__arm__ -I. -Itest

all:
	$(CC) -o givelink_test $(CFLAGS) $(SRC)
