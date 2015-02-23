/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
 /**
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>

#define MY_ADC    ADCA
#define MY_ADC_CH ADC_CH0
#define SAMPLE_SIZE 200

static void adc_init(void);

int main (void)
{
	// Insert system clock initialization code here (sysclk_init()).
	sysclk_init();

	board_init();
	adc_init();
	

	// Insert application code here, after the board has been initialized.
	static  int ADC_result;
	static  int ADC_results[SAMPLE_SIZE];
	static int i = 0;
// 	static int mean = 0;
// 	static int count = 1;
// 	static int double_count = 0;
		
	adc_enable(&MY_ADC);

	while(true)
	{
		adc_start_conversion(&MY_ADC, MY_ADC_CH);
// 		adc_wait_for_interrupt_flag(&MY_ADC, MY_ADC_CH);
		ADC_result = adc_get_result(&MY_ADC, MY_ADC_CH);
		ADC_results[i] = ADC_result;
// 		unsigned int running_total = 0;
// 		for (int j = 0; j < 100; j++)
// 		{
// 			running_total += ADC_results[j];
// 		}
// 		mean = running_total / 100;
		i++;
// 		count++;
// 		if (count == 0)
// 			double_count++;
		if (i == SAMPLE_SIZE)
			i = 0;
	}
}

static void adc_init(void)
{
	struct adc_config adc_conf;
	struct adc_channel_config adcch_conf;
	adc_read_configuration(&MY_ADC, &adc_conf);
	adcch_read_configuration(&MY_ADC, MY_ADC_CH, &adcch_conf);
	adc_set_conversion_parameters(&adc_conf, ADC_SIGN_ON, ADC_RES_12,
	ADC_REF_BANDGAP);
	adc_set_conversion_trigger(&adc_conf, ADC_TRIG_FREERUN, 1, 0);
	adc_set_clock_rate(&adc_conf, 2000000UL);
	adcch_set_input(&adcch_conf, ADCCH_POS_PIN0, ADCCH_NEG_PIN4, 16);
	adc_write_configuration(&MY_ADC, &adc_conf);
	adcch_write_configuration(&MY_ADC, MY_ADC_CH, &adcch_conf);
}