#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>


/****************************
 * Remember to add line:
 * CONFIG_HEAP_MEM_POOL_SIZE=1024
 * to prj.conf
 ****************************/

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 5

K_THREAD_STACK_DEFINE(red_stack, STACKSIZE);
struct k_thread red_thread_data;


// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

void dispatcher_task(void *, void*, void*);
void uart_task(void*, void*, void*);
K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

int init_led();

void red_led_task(void *, void *, void*);
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);

void green_led_task(void *, void *, void*);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

void yellow_led_task(void *, void *, void*);
//static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// Condition variables
//K_MUTEX_DEFINE(thread_mutex);
K_CONDVAR_DEFINE(red_signal);
K_MUTEX_DEFINE(red_mutex);

K_CONDVAR_DEFINE(green_signal);
K_MUTEX_DEFINE(green_mutex);

K_CONDVAR_DEFINE(yellow_signal);
K_MUTEX_DEFINE(yellow_mutex);

K_CONDVAR_DEFINE(release_signal);
K_MUTEX_DEFINE(release_mutex);

// FIFO buffers
K_FIFO_DEFINE(dispatcher_fifo);
K_FIFO_DEFINE(debug_fifo);

K_FIFO_DEFINE(red_fifo);

// FIFO dispatcher data type
struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[20];
};


int init_uart(void) {
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}


int main(void)
{
	timing_init();
	timing_start();
	timing_t start_time = timing_counter_get();

	int ret_led = init_led();
	if(ret_led != 0){
		printk("led init failed\n");
		return ret_led;
	}
	int ret = init_uart();
	if (ret != 0) {
		printk("UART initialization failed!\n");
		return ret;
	}

	timing_t end_time = timing_counter_get();
	timing_stop();
	uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));
	printk("initialization time: %11llu\n", timing_ns);

	return 0;
}


void uart_task(void *unused1, void *unused2, void *unused3)
{
	// Received character from UART
	char rc=0;
	// Message from UART
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				// FIFO puskuri
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
				//strncpy(buf->msg, 20, uart_msg); // mitä ihmettä, miksi kaatuu!!
				snprintf(buf->msg, 20, "%s", uart_msg);

				// You need to:
				// Put dispatcher data to FIFO buffer
				k_fifo_put(&dispatcher_fifo, buf);
				// Clear UART receive buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);

				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(10);
	}
	//return 0;
}


void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);

		printk("Dispatcher: %s\n", sequence);
		int count = 0;

		while(sequence[count] != 0){
			
			if(sequence[count] == 'R'){
				printk("RED\n");

			/*	k_fifo_put(&red_fifo, rec_item);
				k_thread_create(&red_thread_data, red_stack, K_THREAD_STACK_SIZEOF(red_stack),
											   red_led_task, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
			*/							   

				k_mutex_lock(&red_mutex, K_FOREVER);
				k_condvar_broadcast(&red_signal);
				k_mutex_unlock(&red_mutex);
				

			}

			if(sequence[count] == 'Y'){
				printk("YELLOW\n");

				k_mutex_lock(&yellow_mutex, K_FOREVER);
				k_condvar_broadcast(&yellow_signal);
				k_mutex_unlock(&yellow_mutex);
				
			}

			if(sequence[count] == 'G'){
				printk("GREEN\n");

				k_mutex_lock(&green_mutex, K_FOREVER);
				k_condvar_broadcast(&green_signal);
				k_mutex_unlock(&green_mutex);

			}	

			k_mutex_lock(&release_mutex, K_FOREVER);
			k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
			k_mutex_unlock(&release_mutex);

			count++;
		}
		

        // You need to:
        // Parse color and time from the fifo data
        // Example
        //    char color = sequence[0];
        //    int time = atoi(sequence+2);
		//    printk("Data: %c %d\n", color, time);
        // Send the parsed color information to tasks using fifo
        // Use release signal to control sequence or k_yield
	}
}


void red_led_task(void *, void *, void*) {
	while(true){
	timing_start();
	timing_t red_start_time = timing_counter_get();
	printk("Red led task started\n");
	k_mutex_lock(&red_mutex, K_FOREVER);
	k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);
	k_mutex_unlock(&red_mutex);

	gpio_pin_set_dt(&red, 1);
	k_sleep(K_SECONDS(1));
	gpio_pin_set_dt(&red, 0);

	k_mutex_lock(&release_mutex, K_FOREVER);
	k_condvar_broadcast(&release_signal);
	k_mutex_unlock(&release_mutex);
	timing_t red_end_time = timing_counter_get();
	timing_stop();
	uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&red_start_time, &red_end_time));
	printk("red task time: %11llu\n", timing_ns);
	}

}


void green_led_task(void *, void *, void*) {

	while(true){
	printk("Green led task started\n");
	k_mutex_lock(&green_mutex, K_FOREVER);
	k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
	k_mutex_unlock(&green_mutex);

	gpio_pin_set_dt(&green,1);
	k_sleep(K_SECONDS(1));
	gpio_pin_set_dt(&green,0);

	k_mutex_lock(&release_mutex, K_FOREVER);
	k_condvar_broadcast(&release_signal);
	k_mutex_unlock(&release_mutex);
	}

}

void yellow_led_task(void *, void *, void*) {

	while(true){
	printk("Yellow led task started\n");
	k_mutex_lock(&yellow_mutex, K_FOREVER);
	k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
	k_mutex_unlock(&yellow_mutex);

	gpio_pin_set_dt(&green,1);
	gpio_pin_set_dt(&red,1);
	k_sleep(K_SECONDS(1));
	gpio_pin_set_dt(&green, 0);
	gpio_pin_set_dt(&red, 0);

	k_mutex_lock(&release_mutex, K_FOREVER);
	k_condvar_broadcast(&release_signal);
	k_mutex_unlock(&release_mutex);
	}

}

int  init_led() {

	// red led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&red,0);

        // green led pin initialization
        ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&green,0);

	printk("Led initialized ok\n");
	
	return 0;
}


