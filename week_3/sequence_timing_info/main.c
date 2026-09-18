#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>

// yhden pisteen suoritus

#define STACKSIZE 500
#define PRIORITY 5
#define BUTTON_0 DT_ALIAS(sw0)
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

int init_led();
int init_button();
int init_uart();
uint64_t task_runtime = 0;

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void dispatcher_task(void *, void*, void*);
void uart_task(void*, void*, void*);

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
static struct gpio_callback button_0_data;

struct k_thread led_data;


volatile int led_state = 0; // red = 1, yellow = 2, green = 3, pause = 4
volatile int yellow_direction = 0; // 0 = next -> red, 1 = next -> green
volatile int saved_state = 0;
volatile int paused = 0;


K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

K_THREAD_STACK_DEFINE(led_stack, STACKSIZE);

K_SEM_DEFINE(led_done_sem, 0, 1);

K_FIFO_DEFINE(dispatcher_fifo);



struct data_t {

	void *fifo_reserved;
	char msg[20];
};

void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button pressed\n");
	
	if(paused == 0){
		paused = 1;
		saved_state = led_state;
		led_state = 4;
		gpio_pin_set_dt(&green,0);
        gpio_pin_set_dt(&red,0);
	}
	else{
		paused = 0;
		led_state = saved_state;
	}
}


int main(void)
{
	timing_init();
	timing_start();
	timing_t start_time = timing_counter_get();
	init_led();
    init_button();
	init_uart();

	timing_t end_time = timing_counter_get();
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
			
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
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
	
}


void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);

		printk("Dispatcher: %s\n", sequence);
		int count = 0;

		while(sequence[count] != 0){
			
			k_thread_entry_t entry_fn = NULL;

			if(sequence[count] == 'R'){
				printk("RED\n");
				entry_fn = red_led_task;		 					   
			}
				else if(sequence[count] == 'Y'){
					printk("YELLOW\n");
					entry_fn = yellow_led_task;
			}
				else if(sequence[count] == 'G'){
					printk("GREEN\n");
					entry_fn = green_led_task;
			}	

			if (entry_fn != NULL){

				k_thread_create(&led_data,
								led_stack,
								K_THREAD_STACK_SIZEOF(led_stack),
					 			entry_fn,
								 NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);	
				k_sem_take(&led_done_sem, K_FOREVER);				 
			}
			
		count++;
		}
	printk("total sequence runtime: %11lluns\n ", task_runtime);
	printk("sequence runtime in microseconds: %11llu \n", task_runtime/1000);
	task_runtime = 0;
	}

}


void red_led_task(void *, void *, void*) {
	timing_t start_time = timing_counter_get();
	gpio_pin_set_dt(&red, 1);
	k_sleep(K_SECONDS(1));
	gpio_pin_set_dt(&red, 0);

	k_sem_give(&led_done_sem);
	timing_t end_time = timing_counter_get();
	uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));
	printk("red task runtime: %11llu\n", timing_ns);
	task_runtime += timing_ns;
}


void yellow_led_task(void *, void *, void*) {
	timing_t start_time = timing_counter_get();
	gpio_pin_set_dt(&green,1);
	gpio_pin_set_dt(&red,1);
	k_sleep(K_SECONDS(1));
	gpio_pin_set_dt(&green, 0);
	gpio_pin_set_dt(&red, 0);

	k_sem_give(&led_done_sem);
	timing_t end_time = timing_counter_get();
	uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));
	printk("yellow task runtime: %11llu\n", timing_ns);
	task_runtime += timing_ns;
}
      

void green_led_task(void *, void *, void*) {
	timing_t start_time = timing_counter_get();
	gpio_pin_set_dt(&green,1);
	k_sleep(K_SECONDS(1));
	gpio_pin_set_dt(&green,0);

	k_sem_give(&led_done_sem);
	timing_t end_time = timing_counter_get();
	uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));
	printk("green task runtime: %11llu\n", timing_ns);
	task_runtime += timing_ns;
}



int  init_led() {

	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	
	gpio_pin_set_dt(&red,0);

       
        ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	
	gpio_pin_set_dt(&green,0);

	printk("Led initialized ok\n");
	
	return 0;
}

int init_button() {

	int ret;
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);
	printk("Set up button 0 ok\n");
	
	return 0;
}



int init_uart(void) {
	
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}

