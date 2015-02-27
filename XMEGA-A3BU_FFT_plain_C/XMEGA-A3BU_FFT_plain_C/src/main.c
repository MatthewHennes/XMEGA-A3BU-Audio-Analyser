#include <asf.h>
#include <fhtConfig.h>

#define LEFT_AUDIO_IN ADCA
#define LEFT_AUDIO_IN_CHANNEL ADC_CH0
#define SAMPLE_SIZE 256
#define BASS_LED IOPORT_CREATE_PIN(PORTC, 1)
#define MID_LED IOPORT_CREATE_PIN(PORTC, 6)
#define TREBLE_LED IOPORT_CREATE_PIN(PORTB, 1)
#define TRANSFORMING_LED IOPORT_CREATE_PIN(PORTR, 1)
#define BASS_THRESHHOLD 5 //about 250Hz
#define TREBLE_THRESHHOLD 40 //about 1600Hz
#define SIGNAL_THRESHHOLD 25

static void adc_init(void);
static void take_sample(void);

//hack! I think these should be inside main, but I cannot figure out how to pass them to take_sample(), so I just stuck them here for now
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
	
	//Set up a timer to call take_sample() every 1,440 cycles or (about) 45µs
	tc_enable(&TCC0);
	tc_set_overflow_interrupt_callback(&TCC0, take_sample);
	tc_set_wgm(&TCC0, TC_WG_NORMAL);
	tc_write_period(&TCC0, 3200);
	tc_set_overflow_interrupt_level(&TCC0, TC_INT_LVL_LO);
	cpu_irq_enable();
	tc_write_clock_source(&TCC0, TC_CLKSEL_DIV1_gc);
	
	ioport_set_pin_dir(BASS_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MID_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(TREBLE_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(TRANSFORMING_LED, IOPORT_DIR_OUTPUT);
	
	static int16_t FHT_results[SAMPLE_SIZE / 2];
	static bool bass = false;
	static bool mid = false;
	static bool treble = false;


	while(true)
	{		
		if (samples_taken == SAMPLE_SIZE)
		{
			cpu_irq_disable();
			ioport_set_pin_level(TRANSFORMING_LED, false);
			applyHannWindow(ADC_results);
			fhtDitInt(ADC_results);
			complexToDecibelWithGain(ADC_results);
			samples_taken = 0;
			for (int i = 0; i < SAMPLE_SIZE / 2; i++)
				FHT_results[i] = ADC_results[i];
			ioport_set_pin_level(TRANSFORMING_LED, true);
			cpu_irq_enable();
		}
		else
		{
			int bass_sum = 0;
			int mid_sum = 0;
			int treble_sum = 0;
			
			//Activate LED when bass is playing
			for (int i = 1; i < BASS_THRESHHOLD; i++)
				bass_sum += FHT_results[i];
			if (bass_sum > SIGNAL_THRESHHOLD)
				bass = true;
			else
				bass = false;
				
			//Activate LED when mid is playing
			for (int i = BASS_THRESHHOLD; i < TREBLE_THRESHHOLD; i++)
				mid_sum += FHT_results[i];
			if (mid_sum > SIGNAL_THRESHHOLD)
				mid = true;
			else
				mid = false;
			
			//Activate LED when treble is playing (for now, more like mids. True treble seems to almost never activate)
			for (int i = TREBLE_THRESHHOLD; i < SAMPLE_SIZE / 2; i++)
				treble_sum += FHT_results[i];
			if (treble_sum > SIGNAL_THRESHHOLD)
				treble = true;
			else
				treble = false;
				
			// TODO: Possibly add some kind of auto-rescaling thing here such that if bass_sum, mid_sum, and treble_sum are all under SIGNAL_THRESHHOLD, but non-zero for a while it
			//			rescales itself to a higher multiplier on the ADC? If so, it would also need to detect clipping somehow and rescale down... Alternatively, perhaps I could just put
			//			either a button that cycles through scale factors or some kind of jumper or switch that allows you to set the scale factor?
				
			ioport_set_pin_level(BASS_LED, bass);
			ioport_set_pin_level(MID_LED, mid);
			ioport_set_pin_level(TREBLE_LED, treble);
		}
	}
}

//hack! I am using static variables defined outside of main, which seems very sketchy, but I could not figure out how to pass argumets to this function, so I just did that...
static void take_sample(void)
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
	adcch_set_input(&adcch_conf, ADCCH_POS_PIN0, ADCCH_NEG_PIN4, 32);
	adc_write_configuration(&LEFT_AUDIO_IN, &adc_conf);
	adcch_write_configuration(&LEFT_AUDIO_IN, LEFT_AUDIO_IN_CHANNEL, &adcch_conf);
}