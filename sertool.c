#include <argp.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

struct serial_params {
	__u16 flags;
	__u32 baud_rate;
	__u32 data_bits;
	__s32 rcv_timeout;
	__s32 xmit_timeout;
	__u32 parity;
	__u32 stop_bits;
	__u8 rx_fifo_trigger;
	__u8 tx_fifo_trigger;
	bool dma;
	__u8 rx_fifo_dma_trigger;
	__u8 tx_fifo_dma_trigger;
	__u8 rx_gran;
	__u8 tx_gran;
};

#define SERIAL_IOC_MAGIC	'h'
#define SERIAL_GET_PARAMS	_IOR(SERIAL_IOC_MAGIC, 1, struct serial_params)
#define SERIAL_SET_PARAMS	_IOW(SERIAL_IOC_MAGIC, 2, struct serial_params)
#define SERIAL_RX_BUFFER_CLEAR  _IO(SERIAL_IOC_MAGIC, 3)
#define SERIAL_WRITE_IOC	_IOW(SERIAL_IOC_MAGIC, 4, char *)

/* params flags */
#define BIT(nr)			(1UL << (nr))
#define SERIAL_PARAMS_BAUDRATE		BIT(0)
#define SERIAL_PARAMS_DATABITS		BIT(1)
#define SERIAL_PARAMS_RCV_TIMEOUT	BIT(2)
#define SERIAL_PARAMS_XMIT_TIMEOUT	BIT(3)
#define SERIAL_PARAMS_PARITY		BIT(4)
#define SERIAL_PARAMS_STOPBITS		BIT(5)
#define SERIAL_PARAMS_FIFO_TRIGGER	BIT(6)
#define SERIAL_WAIT_FOR_XMIT		BIT(7)

enum msg_types {
	PING_PONG,
	REQ_BYTES,
	MSG_TYPES
};

const char *argp_program_version = "sertool 0.1.0";
const char *argp_program_bug_address = "<hernan@vanguardiasur.com.ar>";
static char doc[] = "Tool for /dev/serial.";
static char args_doc[] = "-s|g --param DEVICE";

static struct argp_option options[] = {
	{ "set", 's', 0, 0, "Set Params"},
	{ "get", 'g', 0, 0, "Get Params"},
	{ "rx-buff-clear", 'c', 0, 0, "Clear FIFOs"},
	{ "baudrate", 'b', "baudrate", 0, "Set baudrate"},
	{ "data-bits", 'd', "databits", 0, "Set data bits"},
	{ "parity", 'p', "parity", 0, "Set parity"},
	{ "stop-bits", 'o', "stopbits", 0, "Set stop bits"},
	{ "rcv-timeout", 'r', "rcvtimeout", 0, "rcv_timeout"},
	{ "xmit-timeout", 'x', "xmittimeout", 0, "xmit_timeout"},
	/* add fifo configuration */
	{ 0 }
};

struct arguments {
	enum { SERIAL_SET, SERIAL_GET, SERIAL_CLEAR } mode;
	int flags;
	int baud_rate;
	int data_bits;
	char parity;
	int stop_bits;
	int rcv_timeout;
	int xmit_timeout;
	bool fifoclear;
	char device[64];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;
	switch (key) {
	case 's': arguments->mode = SERIAL_SET; break;
	case 'g': arguments->mode = SERIAL_GET; break;
	case 'c': arguments->mode = SERIAL_CLEAR; break;
	case 'b': arguments->baud_rate = strtol(arg, NULL, 10);
		  arguments->flags |= SERIAL_PARAMS_BAUDRATE; break;
	case 'd': arguments->data_bits = strtol(arg, NULL, 10);
		  arguments->flags |= SERIAL_PARAMS_DATABITS; break;
	case 'o': arguments->stop_bits = strtol(arg, NULL, 10);
		  arguments->flags |= SERIAL_PARAMS_STOPBITS; break;
	case 'p': arguments->parity = *arg;
		  arguments->flags |= SERIAL_PARAMS_PARITY; break;
	case 'r': arguments->rcv_timeout = strtol(arg, NULL, 10);
		  arguments->flags |= SERIAL_PARAMS_RCV_TIMEOUT; break;
	case 'x': arguments->xmit_timeout =  strtol(arg, NULL, 10);
		  arguments->flags |= SERIAL_PARAMS_XMIT_TIMEOUT; break;
	case ARGP_KEY_ARG:
		  if (state->arg_num >= 1) {
			  argp_usage(state);
		  }
		  snprintf(arguments->device, 63, "%s", arg);
		  break;
	case ARGP_KEY_END:
		  if (state->arg_num < 1)
			  argp_usage(state);
		  break;
	default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

static void print_set(struct serial_params params)
{
	if (params.flags & SERIAL_PARAMS_BAUDRATE)
		printf("BAUDRATE = %d\n", params.baud_rate);
	if (params.flags & SERIAL_PARAMS_DATABITS)
		printf("DATABITS = %d\n", params.data_bits);
	if (params.flags & SERIAL_PARAMS_RCV_TIMEOUT)
		printf("RCV_TIMEOUT = %d\n", params.rcv_timeout);
	if (params.flags & SERIAL_PARAMS_XMIT_TIMEOUT)
		printf("XMIT_TIMEOUT = %d\n", params.xmit_timeout);
	if (params.flags & SERIAL_PARAMS_PARITY) {
		switch (params.parity) {
		case 0:
		case 2: printf("PARITY = NO PARITY\n"); break;
		case 1: printf("PARITY = ODD\n"); break;
		case 3: printf("PARITY = EVEN\n"); break;
		default: printf("Unknown parity\n");
		}
	}
	if (params.flags & SERIAL_PARAMS_STOPBITS)
		printf("STOPBITS = %d\n", params.stop_bits);
	if (params.flags & SERIAL_PARAMS_FIFO_TRIGGER)
		printf("RX_FIFO_TRIGGER = %d\nTX_FIFO_TRIGGER = %d\nDMA = %d\n"
		       "RX_FIFO_DMA_TRIGGER = %d\nTX_FIFO_DMA_TRIGGER = %d\n"
		       "RX_GRANULARITY = %d\nTX_GANULARITY = %d\n",
		       params.rx_fifo_trigger, params.tx_fifo_trigger,
		       params.dma, params.rx_fifo_dma_trigger,
		       params.tx_fifo_dma_trigger,params.rx_gran,
		       params.tx_gran);
}

static void print_get(struct serial_params params)
{
	printf("BAUDRATE = %d\n", params.baud_rate);
	printf("DATABITS = %d\n", params.data_bits);
	printf("RCV_TIMEOUT = %d\n", params.rcv_timeout);
	printf("XMIT_TIMEOUT = %d\n", params.xmit_timeout);
	switch (params.parity) {
	case 0:
	case 2: printf("PARITY = NO PARITY\n"); break;
	case 1: printf("PARITY = ODD\n"); break;
	case 3: printf("PARITY = EVEN\n"); break;
	default: printf("Unknown parity\n");
	}
	printf("STOPBITS = %d\n", params.stop_bits);
	printf("RX_FIFO_TRIGGER = %d\nTX_FIFO_TRIGGER = %d\nDMA = %d\n"
	       "RX_FIFO_DMA_TRIGGER = %d\nTX_FIFO_DMA_TRIGGER = %d\n"
	       "RX_GRANULARITY = %d\nTX_GANULARITY = %d\n",
	       params.rx_fifo_trigger, params.tx_fifo_trigger,
	       params.dma, params.rx_fifo_dma_trigger,
	       params.tx_fifo_dma_trigger,params.rx_gran,
	       params.tx_gran);
}

static void serial_set(int fd, struct serial_params *params)
{
	print_set(*params);
	/* TODO: check return value */
	ioctl(fd, SERIAL_SET_PARAMS, params);
}

static void serial_get(int fd, struct serial_params *params)
{
	/* TODO: check return value */
	ioctl(fd, SERIAL_GET_PARAMS, params);
	print_get(*params);
}

static void serial_rx_buff_clear(int fd)
{
	/* TODO: check return value */
	ioctl(fd, SERIAL_RX_BUFFER_CLEAR);
	printf("RX buffer cleared.\n");
}
int main(int argc, char *argv[])
{
	struct arguments arguments;
	struct serial_params params;
	struct stat dev_stat;
	int fd, ret;

	/* Default options */
	arguments.flags = 0;
	arguments.baud_rate = 115200;
	arguments.data_bits = 8;
	arguments.stop_bits = 1;
	arguments.parity = 'n';
	arguments.rcv_timeout = 10000;
	arguments.xmit_timeout = 10000;
	arguments.fifoclear = false;

	/* Add arguments to client mode to stop after i) N bytes or ii) M seconds. */
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	fd = open(arguments.device, O_RDWR);
	if (fd < 0) {
		perror("open error: ");
		exit(1);
	}

	ret = fstat(fd, &dev_stat);
	if (ret < 0) {
		perror("fstat error: ");
		exit(1);
	}

	if (!S_ISCHR(dev_stat.st_mode)) {
		printf("%s is not a character device/n", arguments.device);
		exit(1);
	}

	if (arguments.rcv_timeout < 0 || arguments.rcv_timeout > 300000) {
		printf("%d bytes: not a valid timeout (0 < rcv_timeout <= 30000)\n",
		       arguments.rcv_timeout);
		exit(1);
	}

	if (arguments.xmit_timeout < 0 || arguments.xmit_timeout > 300000) {
		printf("%d bytes: not a valid timeout (0 < xmit_timeout <= 30000)\n",
		       arguments.xmit_timeout);
		exit(1);
	}

	if (arguments.baud_rate < 0 || arguments.baud_rate > 3688400) {
		printf("%d bps: not a valid baud_rate. Baudrate must be between 0 and 3688400\n",
		       arguments.baud_rate);
		exit(1);
	}

	if (arguments.data_bits < 1 || arguments.data_bits > 64) {
		printf("%d not a valid amount of data_bits. Must be between 1 and 64\n",
		       arguments.data_bits);
		exit(1);
	}

	if (arguments.stop_bits < 1 || arguments.stop_bits > 64) {
		printf("%d not a valid amount of stop_bits. Must be 1 or 2\n",
		       arguments.data_bits);
		exit(1);
	}

	if (arguments.parity != 'n' && arguments.parity != 'e' && arguments.parity != 'o') {
		printf("Parity must be n (no parity), e (even) or o (odd)");
		exit(1);
	}

	/* Default params */
	params.flags = arguments.flags;
	params.baud_rate = arguments.baud_rate;
	params.data_bits = arguments.data_bits;
	params.rcv_timeout = arguments.rcv_timeout;
	params.xmit_timeout = arguments.xmit_timeout;
	switch (arguments.parity) {
	case 'n': params.parity = 0; break;
	case 'e': params.parity = 3; break;
	case 'o': params.parity = 1; break;
	}
	params.stop_bits = arguments.stop_bits;
	params.rx_fifo_trigger = 16;
	params.tx_fifo_trigger = 32;
	params.dma = 0;
	params.rx_fifo_dma_trigger = 0;
	params.tx_fifo_dma_trigger = 0;
	params.rx_gran = 0;
	params.tx_gran = 0;

	switch (arguments.mode) {
	case SERIAL_SET:
		printf("Going to SET:\n");
		serial_set(fd, &params);
		break;
	case SERIAL_GET:
		printf("GOT:\n");
		serial_get(fd, &params);
		break;
	case SERIAL_CLEAR:
		serial_rx_buff_clear(fd);
		break;
	default:
		printf("oops, unknown mode (%d). something is wrong!\n", arguments.mode);
		exit(1);
	}

	close(fd);
	return 0;
}
