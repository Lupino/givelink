SRC=crc16.c \
	lora2mqtt.c \
	unhex.c \
	test/main.c

CFLAGS=-D__arm__ -I. -Itest

all:
	$(CC) -o lora2mqtt_test $(CFLAGS) $(SRC)
