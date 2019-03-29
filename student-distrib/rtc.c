// vim:ts=4 noet
#include "rtc.h"
#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"



volatile int rtc_interrupt_occurred;
static int32_t rtc_max_user_freq = 1024;

void init_rtc(void) {
	char prev;
	cli();
	// Set interrupt handler
	SET_IDT_ENTRY(idt[RTC_INT], _rtc_isr);
	idt[RTC_INT].present = 1;

	// Enable IRQs
	enable_irq(RTC_IRQ);
	enable_irq(SLAVE_PIC_IRQ);

	// Init routine from https://wiki.osdev.org/RTC
	// Enable interrupt
	outb(RTC_REG_B, RTC_ADDR_PORT);		// select register B, and disable NMI
	prev = inb(RTC_DATA_PORT);		// read the current value of register B
	outb(RTC_REG_B, RTC_ADDR_PORT);		// set the index again (a read will reset the index to register D)
	outb(prev | 0x40, RTC_DATA_PORT);// write the previous value ORed with 0x40. This turns on bit 6 of register B

	// Set frequency to 1024 Hz
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);		// set index to register A, disable NMI
	prev = inb(RTC_DATA_PORT);		// get initial value of register A
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);		// reset index to A
	outb((prev & 0xF0) | RTC_RATE, RTC_DATA_PORT); //write only our rate to A. Note, rate is the bottom 4 bits.

	sti();
}

/* rtc_set_pi_freq
 *	Descrption:	Set the frequency of RTC periodic interrupt
 * 	
 *	Arg: freq: must be a power of 2 and inside the range of [1024,8192]
 * 
 * 	RETURN:
 * 		-1 if failed
 * 		0  if sucess
 *	reference :https://github.com/torvalds/linux/blob/master/drivers/char/rtc.c
 */
int rtc_set_pi_freq(int32_t freq){
	int tmp, rtc_rate_val;
	char prev;
	//The max we can do is 8192Hz.
	if ( (freq<2) || (freq>8192) ){
		return -1;
	}
	// limit user frequency
	if (freq > rtc_max_user_freq) {
		return -1;
	}

	//Check that the input was really a power of 2.
	tmp =1;
	while (freq > (1<<tmp))
		tmp++;
	if (freq != (1<<tmp))
		return -1;
	// set rtc
	rtc_rate_val = 16 - tmp; // 8192 is the 2 to the power of 16.

	// set frequency	
	cli();
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// set index to register A, disable NMI
	prev = inb(RTC_DATA_PORT);				// get initial value of register A
	outb(RTC_FREQ_SELECT, RTC_ADDR_PORT);	// reset index to A
	outb((prev & 0xF0) | (rtc_rate_val&0xF), RTC_DATA_PORT);	//write only our rate to A. Note, rate is the bottom 4 bits.
	sti();
	return 0;
}

void rtc_isr(void) {
    rtc_interrupt_occurred = 1;
	//test_interrupt_freq(0,0);
	//test_interrupts();
    /* test_rtc_freq(0); */
	outb(0x0C, RTC_ADDR_PORT);
	(void) inb(RTC_DATA_PORT);
    
}


int32_t rtc_read(){
    //int prev;
    //prev = inb(0x71);
    rtc_interrupt_occurred = 0;
    do{
        ;// wait until the rtc interrupt handler clear the flag
    }while (!rtc_interrupt_occurred);
    return 0;
}

int32_t rtc_open(){
    char prev;
    cli();
    // Set frequency to 2 Hz
    outb(RTC_REG_A, RTC_ADDR_PORT);        // set index to register A, disable NMI
    prev = inb(RTC_DATA_PORT);        // get initial value of register A
    outb(RTC_REG_A, RTC_ADDR_PORT);        // reset index to A
    outb((prev & 0xF0) | RTC_OPEN_RATE, RTC_DATA_PORT); //write only our rate to A. Note, rate is the bottom 4 bits.
    sti();
    return 0;
}

int32_t rtc_close(){
    return 0;
}

