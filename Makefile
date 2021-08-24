SRC=givelink_common \
	givelink.c \
	test/main.c

CFLAGS=-D__arm__ -I. -Itest

all: givelink_test

givelink_test: $(SRC)
	$(CC) -o givelink_test $(CFLAGS) $(SRC)

clean:
	rm -f givelink_test
