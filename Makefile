all: test_serial.c
	${CROSS_COMPILE}gcc  test_serial.c serial.a libtimer.a -o test_serial -lpthread

clean:
	rm *.o test_serial
