#include <asf.h>
#include <fhtConfig.h>
#include <math.h>

#define LEFT_AUDIO_IN    ADCA
#define LEFT_AUDIO_IN_CHANNEL ADC_CH0
#define SAMPLE_SIZE 256
#define BASS_LED IOPORT_CREATE_PIN(PORTR, 0)
#define TRANSFORMING_LED IOPORT_CREATE_PIN(PORTR, 1)

static void adc_init(void);
void take_sample(void);

// hack! I think these should be inside main, but I cannot figure out how to pass them to take_sample(), so I just stuck them here for now
static int16_t ADC_results[SAMPLE_SIZE];
static int samples_taken = 0;

int main (void)
{
	sysclk_init();
	board_init();
	adc_init();
	pmic_init();
	ioport_init();

	adc_enable(&LEFT_AUDIO_IN);
	
	//Set up a timer to call take_sample() every 338 cycles or (about) 45µs
	tc_enable(&TCC0);
	tc_set_overflow_interrupt_callback(&TCC0, take_sample);
	tc_set_wgm(&TCC0, TC_WG_NORMAL);
	tc_write_period(&TCC0, 338);
	tc_set_overflow_interrupt_level(&TCC0, TC_INT_LVL_LO);
	cpu_irq_enable();
	tc_write_clock_source(&TCC0, TC_CLKSEL_DIV1_gc);
	
	ioport_set_pin_dir(BASS_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(TRANSFORMING_LED, IOPORT_DIR_OUTPUT);
	
	static int16_t FHT_results[SAMPLE_SIZE / 2];
	static bool bass = false;

	while(true)
	{		
		if (samples_taken == SAMPLE_SIZE)
		{
			ioport_set_pin_level(TRANSFORMING_LED, false);
			cpu_irq_disable();
			applyHannWindow(ADC_results);
			fhtDitInt(ADC_results);
			complexToDecibelWithGain(ADC_results);
			samples_taken = 0;
			for (int i = 0; i < SAMPLE_SIZE / 2; i++)
				FHT_results[i] = ADC_results[i];
			cpu_irq_enable();
			ioport_set_pin_level(TRANSFORMING_LED, true);
		}
		else
		{
			int bass_sum = 0;
			for (int i = 0; i < 10; i++)
				bass_sum += FHT_results[i];
			if (bass_sum > 10)
				bass = true;
			else
				bass = false;
			ioport_set_pin_level(BASS_LED, !bass);
		}
	}
}

//hack! I am using static variables defined outside of main, which seems very sketchy, but I could not figure out how to pass argumets to this function, so I just did that...
void take_sample(void)
{
	ADC_results[samples_taken] = adc_get_result(&LEFT_AUDIO_IN, LEFT_AUDIO_IN_CHANNEL);
	samples_taken++;
}

static void adc_init(void)
{
	struct adc_config adc_conf;
	struct adc_channel_config adcch_conf;
	adc_read_configuration(&LEFT_AUDIO_IN, &adc_conf);
	adcch_read_configuration(&LEFT_AUDIO_IN, LEFT_AUDIO_IN_CHANNEL, &adcch_conf);
	adc_set_conversion_parameters(&adc_conf, ADC_SIGN_ON, ADC_RES_12,
	ADC_REF_BANDGAP);
	adc_set_conversion_trigger(&adc_conf, ADC_TRIG_FREERUN, 1, 0);
	adc_set_clock_rate(&adc_conf, 2000000UL);
	adcch_set_input(&adcch_conf, ADCCH_POS_PIN0, ADCCH_NEG_PIN4, 16);
	adc_write_configuration(&LEFT_AUDIO_IN, &adc_conf);
	adcch_write_configuration(&LEFT_AUDIO_IN, LEFT_AUDIO_IN_CHANNEL, &adcch_conf);
}