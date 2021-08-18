#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "serial.h"
#include "libtimer.h"

#define SERIAL_PORT_DEV "/dev/ttyS0"

char test_data[10] = "123456789";
static pthread_t send_thread_id;
int process_exit;

struct serial_test {
	unsigned int send_cnt;
	serial_t *serial_port;
};

struct serial_test _serial_test;

void timer_handler(size_t timer_id, void *user_data)
{
	struct serial_test *ser = (struct serial_test *)user_data;

	ser->send_cnt++;
}

void *send_thread(void * data)
{
	struct serial_test *ser = (struct serial_test *)data;
	int data_size, writed_cnt;

	data_size = sizeof(test_data) / sizeof(test_data[0]);

	while (1) {
		if (ser->send_cnt) {
			writed_cnt = serial_write(_serial_test.serial_port, test_data, data_size);
			if (writed_cnt != data_size) {
				printf("serial_write size fault writed_cnt %d data_size %d\n",
					writed_cnt, data_size);
			}
			ser->send_cnt--;
		}

		usleep(10);
	};
}

void sig_term_handler(int signum)
{
	printf("terminator process\n");
	if (signum == SIGINT) {
		printf("terminator process\n");
		process_exit = 1;
	}
}

void catch_sigterm()
{
	signal(SIGINT, sig_term_handler);
}

int main(int argc, char *argvp[])
{
	int ret, test_cnt;
	size_t timer;

	initialize();
	process_exit = 0;

	_serial_test.send_cnt = 0;

	if (pthread_create(&send_thread_id, NULL, send_thread, &_serial_test)) {
		printf("Thread creation failed\n");
		return 0;
	}

	_serial_test.serial_port = serial_new();
	if (!_serial_test.serial_port) {
		printf("serial_port create fail\n");
		return 0;
	}

	ret = serial_open(_serial_test.serial_port, SERIAL_PORT_DEV, 115200);
	if (ret) {
		printf("serial_port open fail\n");
		return 0;
	}

	catch_sigterm();

	timer = start_timer(1000, timer_handler, TIMER_PERIODIC, &_serial_test);

	while (!process_exit) {
		sleep(1);
	};

	printf("EXIT!!!!!\n");
	stop_timer(timer);
	serial_close(_serial_test.serial_port);
	serial_free(_serial_test.serial_port);
	finalize();

	pthread_cancel(send_thread_id);
	pthread_join(send_thread_id, NULL);

	return 0;
}
