#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
//static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

#define STACKSIZE 500
#define PRIORITY 5

int init_led();
void red_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);


volatile int led_state = 0; // red = 1, yellow = 2, green = 3, pause = 4
volatile int yellow_direction = 0; //
volatile int saved_state = 0;
volatile int paused = 0;

// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
// #define BUTTON_1 DT_ALIAS(sw1)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;
int init_button();

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
	init_led();
        init_button();
		printk("start from main\n");
        led_state = 1;
	return 0;
}

// Initialize leds
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

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {

        if (led_state == 1){
        	printk("state = 1\n");
			gpio_pin_set_dt(&red,1);
			k_sleep(K_SECONDS(1));
		
			if(led_state == 1){
				gpio_pin_set_dt(&red,0);
				yellow_direction = 1;
       	 		led_state = 2;
        	}
		}
                     
        k_yield();
	}
}


void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {

        if (led_state == 2){
         	printk("state = 2\n");
			gpio_pin_set_dt(&green,1);
        	gpio_pin_set_dt(&red,1);
			k_sleep(K_SECONDS(1));


			if(led_state == 2){
				gpio_pin_set_dt(&green,0);
        		gpio_pin_set_dt(&red,0);



				if(yellow_direction == 1){
					led_state = 3;
				}
				else{
					led_state = 1;
				}
			}
		}


        k_yield();
    }
}
      

// Task to handle green led
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {

        if (led_state == 3){
            printk("state = 3\n");
			gpio_pin_set_dt(&green,1);
			k_sleep(K_SECONDS(1));

			if(led_state == 3){
				gpio_pin_set_dt(&green,0);
				yellow_direction = 0;
				//k_sleep(K_SECONDS(1));
				led_state = 2;
			}
        }       
        k_yield();    
	}
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