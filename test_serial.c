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

#define MCU_SERIAL_PORT_DEV "/dev/ttyS1"
#define SERIAL_SPEED 921600


char test_data[256];
static pthread_t send_thread_id;
static pthread_t recv_thread_id;
int process_exit;

struct serial_test {
	unsigned int send_cnt;
	serial_t *serial_port;
};

struct serial_test _serial_test;

uint8_t do_checksum(uint8_t *data, uint16_t data_len)
{
	uint8_t ret = 0;
	uint32_t len = 0;

	do {
		ret = ret ^ data[len];
		len++;
	} while (len < data_len);

	sleep(1);
	return ret;
}

void timer_handler(size_t timer_id, void *user_data)
{
	struct serial_test *ser = (struct serial_test *)user_data;

	process_exit = 1;
}

void *send_thread(void * data)
{
	struct serial_test *ser = (struct serial_test *)data;
	int data_size, writed_cnt;
	#define HEAD1 0x5A
	#define HEAD2 0x87
	#define PAGE_RQ 0x0B
	#define HEAD1_OFF 0
	#define HEAD2_OFF 1
	#define ID_OFF 2
	#define LENH_OFF 3
	#define LENL_OFF 4
	#define PAGE_DATA_OFF 5
	#define PAGE_NUM 0x00
	#define METER_OFF_0 0x01
	#define METER_OFF_1 0x02
	#define METER_OFF_2 0x03
	#define PAGE_DATA 0x04

	test_data[HEAD1_OFF] = HEAD1;
	test_data[HEAD2_OFF] = HEAD2;
	test_data[ID_OFF] = PAGE_RQ;
	test_data[LENH_OFF] = 0x00;
	test_data[LENL_OFF] = 0x06;

	test_data[PAGE_DATA_OFF + PAGE_NUM] = 0x02;
	test_data[PAGE_DATA_OFF + METER_OFF_0] = 0x00;
	test_data[PAGE_DATA_OFF + METER_OFF_1] = 0x00;
	test_data[PAGE_DATA_OFF + METER_OFF_2] = 0x00;
	test_data[PAGE_DATA_OFF + PAGE_DATA + 0] = 0x00;
	test_data[PAGE_DATA_OFF + PAGE_DATA + 1] = 0x01;
	test_data[PAGE_DATA_OFF + PAGE_DATA + 2] = 0xd3;

	//5a 87 0b 00 06 02 00 00 00 00 01 d3

	printf("checksun 0x%x\n", do_checksum(test_data, 6 + 5));

	//data_size = sizeof(test_data) / sizeof(test_data[0]);
	data_size = 6 + 5 + 1;
	//while (1) {
	//	if (ser->send_cnt) {
	//		writed_cnt = serial_write(_serial_test.serial_port, test_data, data_size);
	//		if (writed_cnt != data_size) {
	//			printf("serial_write size fault writed_cnt %d data_size %d\n",
	//				writed_cnt, data_size);
	//		}
	//		ser->send_cnt--;
	//	}
	//
	//	usleep(10);
	//};
}

void *recv_thread(void * data)
{
	struct serial_test *ser = (struct serial_test *)data;
	uint8_t rec_buf[200];
	int buf_size, read_cnt, i;
	int data_len, buf_index, buf_index_tmp;
	unsigned char *page_data, checksum;

	buf_size = sizeof(rec_buf) / sizeof(rec_buf[0]);
	buf_index = 0;
	buf_index_tmp = 0;

	while (1) {
		printf("TEST 3\n");
		read_cnt = serial_read(_serial_test.serial_port, rec_buf, buf_size, 10);
		if (read_cnt == 0) {
			printf("read empy\n");
			continue;
		}

		printf("read_cnt %d\n", read_cnt);
		buf_index = 0;
		checksum = 0;

		while (buf_index < read_cnt) {
			if (rec_buf[buf_index + HEAD1_OFF] == HEAD1 && rec_buf[buf_index + HEAD2_OFF] == HEAD2) {
				if (rec_buf[buf_index + ID_OFF] == PAGE_RQ) {
					data_len = rec_buf[buf_index + LENH_OFF] << 8 | rec_buf[buf_index + LENL_OFF];

					page_data = (unsigned char *)malloc(sizeof(unsigned char) * data_len);
					memcpy(page_data, &rec_buf[buf_index + PAGE_DATA_OFF], data_len);

					for (i = 0; i < 5; i++) {
						checksum = checksum ^ rec_buf[i];
					}

					for (i = 0; i < data_len; i++) {
						checksum = checksum ^ page_data[i];
					}

					printf("page_data[0] 0x%x\n", page_data[0]);
					printf("1 checksum 0x%x\n", checksum);
					printf("2 checksun 0x%x\n", rec_buf[buf_index + PAGE_DATA_OFF + data_len]);
					free(page_data);
				}
			}

			buf_index++;
		}

		usleep(100);
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

	//_serial_test.serial_port = serial_new();
	//if (!_serial_test.serial_port) {
	//	printf("serial_port create fail\n");
	//	return 0;
	//}
	//
	//ret = serial_open(_serial_test.serial_port, MCU_SERIAL_PORT_DEV, SERIAL_SPEED);
	//if (ret) {
	//	printf("serial_port open fail\n");
	//	return 0;
	//}

	catch_sigterm();
	timer = start_timer(500, timer_handler, TIMER_PERIODIC, &_serial_test);

	//if (pthread_create(&recv_thread_id, NULL, recv_thread, &_serial_test)) {
	//	printf("Thread creation failed\n");
	//	return 0;
	//}

	if (pthread_create(&recv_thread_id, NULL, send_thread, &_serial_test)) {
		printf("Thread creation failed\n");
		return 0;
	}

	while (!process_exit) {
		sleep(1);
	};
	//
	//printf("EXIT!!!!!\n");
	//stop_timer(timer);
	//finalize();
	//serial_close(_serial_test.serial_port);
	//serial_free(_serial_test.serial_port);
	pthread_cancel(recv_thread_id);
	pthread_join(recv_thread_id, NULL);

	return 0;
}
