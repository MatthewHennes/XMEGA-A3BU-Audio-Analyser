#include <asf.h>
#include <fhtConfig.h>

#define AUDIO_IN ADCA
#define LEFT_AUDIO_IN_CHANNEL ADC_CH0
#define RIGHT_AUDIO_IN_CHANNEL ADC_CH1
#define SAMPLE_SIZE 256
#define LEFT_BASS_LED IOPORT_CREATE_PIN(PORTC, 7)
#define LEFT_MID_LED IOPORT_CREATE_PIN(PORTC, 6)
#define LEFT_TREBLE_LED IOPORT_CREATE_PIN(PORTC, 5)
#define RIGHT_BASS_LED IOPORT_CREATE_PIN(PORTC, 4)
#define RIGHT_MID_LED IOPORT_CREATE_PIN(PORTC, 3)
#define RIGHT_TREBLE_LED IOPORT_CREATE_PIN(PORTC, 2)
#define TRANSFORMING_LED IOPORT_CREATE_PIN(PORTR, 1)
#define BASS_THRESHOLD 6 //about 250Hz
#define TREBLE_THRESHOLD 25 //about 1000Hz
#define SIGNAL_THRESHOLD 25
#define GAIN 64

static void adc_init(void);
static void take_sample(void);

//hack! I think these should be inside main, but I cannot figure out how to pass them to take_sample(), so I just stuck them here for now
static int16_t left[SAMPLE_SIZE];
static int16_t right[SAMPLE_SIZE];
static int samples_taken = 0;

int main (void)
{
	sysclk_init();
	board_init();
	adc_init();
	pmic_init();
	ioport_init();

	adc_enable(&AUDIO_IN);
	
	//Set up a timer to call take_sample() every 3,200 cycles
	tc_enable(&TCC0);
	tc_set_overflow_interrupt_callback(&TCC0, take_sample);
	tc_set_wgm(&TCC0, TC_WG_NORMAL);
	tc_write_period(&TCC0, 3200);
	tc_set_overflow_interrupt_level(&TCC0, TC_INT_LVL_LO);
	cpu_irq_enable();
	tc_write_clock_source(&TCC0, TC_CLKSEL_DIV1_gc);
	
	ioport_set_pin_dir(LEFT_BASS_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(LEFT_MID_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(LEFT_TREBLE_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(RIGHT_BASS_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(RIGHT_MID_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(RIGHT_TREBLE_LED, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(TRANSFORMING_LED, IOPORT_DIR_OUTPUT);
	
	static int16_t left_results[SAMPLE_SIZE / 2];
	static int16_t right_results[SAMPLE_SIZE / 2];
	static bool left_bass = false;
	static bool left_mid = false;
	static bool left_treble = false;
	static bool right_bass = false;
	static bool right_mid = false;
	static bool right_treble = false;


	while(true)
	{		
		if (samples_taken == SAMPLE_SIZE)
		{
			cpu_irq_disable();
			ioport_set_pin_level(TRANSFORMING_LED, false);
			
			applyHannWindow(left);
			fhtDitInt(left);
			complexToDecibelWithGain(left);
			for (int i = 0; i < SAMPLE_SIZE / 2; i++)
				left_results[i] = left[i];
			
			applyHannWindow(right);
			fhtDitInt(right);
			complexToDecibelWithGain(right);
			for (int i = 0; i < SAMPLE_SIZE / 2; i++)
				right_results[i] = right[i];
				
			samples_taken = 0;
			ioport_set_pin_level(TRANSFORMING_LED, true);
			cpu_irq_enable();
		}
		else
		{
			int left_bass_sum = 0;
			int left_mid_sum = 0;
			int left_treble_sum = 0;
			int right_bass_sum = 0;
			int right_mid_sum = 0;
			int right_treble_sum = 0;
			
			//Activate LED when bass is playing on left
			for (int i = 1; i < BASS_THRESHOLD; i++)
				left_bass_sum += left_results[i];
			if (left_bass_sum > SIGNAL_THRESHOLD)
				left_bass = true;
			else
				left_bass = false;
				
			//Activate LED when mid is playing on left
			for (int i = BASS_THRESHOLD; i < TREBLE_THRESHOLD; i++)
				left_mid_sum += left_results[i];
			if (left_mid_sum > SIGNAL_THRESHOLD)
				left_mid = true;
			else
				left_mid = false;
			
			//Activate LED when treble is playing on left (for now, more like mids. True treble seems to almost never activate)
			for (int i = TREBLE_THRESHOLD; i < SAMPLE_SIZE / 2; i++)
				left_treble_sum += left_results[i];
			if (left_treble_sum > SIGNAL_THRESHOLD)
				left_treble = true;
			else
				left_treble = false;
			
			//Activate LED when bass is playing on right
			for (int i = 1; i < BASS_THRESHOLD; i++)
				right_bass_sum += right_results[i];
			if (right_bass_sum > SIGNAL_THRESHOLD)
				right_bass = true;
			else
				right_bass = false;
				
			//Activate LED when mid is playing on right
			for (int i = BASS_THRESHOLD; i < TREBLE_THRESHOLD; i++)
				right_mid_sum += right_results[i];
			if (right_mid_sum > SIGNAL_THRESHOLD)
				right_mid = true;
			else
				right_mid = false;
				
			//Activate LED when treble is playing on right (for now, more like mids. True treble seems to almost never activate)
			for (int i = TREBLE_THRESHOLD; i < SAMPLE_SIZE / 2; i++)
				right_treble_sum += right_results[i];
			if (right_treble_sum > SIGNAL_THRESHOLD)
				right_treble = true;
			else
				right_treble = false;
				
			// TODO: Possibly add some kind of auto-rescaling thing here such that if bass_sum, mid_sum, and treble_sum are all under SIGNAL_THRESHHOLD, but non-zero for a while it
			//			rescales itself to a higher multiplier on the ADC? If so, it would also need to detect clipping somehow and rescale down... Alternatively, perhaps I could just put
			//			either a button that cycles through scale factors or some kind of jumper or switch that allows you to set the scale factor?
				
			ioport_set_pin_level(LEFT_BASS_LED, left_bass);
			ioport_set_pin_level(LEFT_MID_LED, left_mid);
			ioport_set_pin_level(LEFT_TREBLE_LED, left_treble);
			ioport_set_pin_level(RIGHT_BASS_LED, right_bass);
			ioport_set_pin_level(RIGHT_MID_LED, right_mid);
			ioport_set_pin_level(RIGHT_TREBLE_LED, right_treble);
		}
	}
}

//hack! I am using static variables defined outside of main, which seems very sketchy, but I could not figure out how to pass argumets to this function, so I just did that...
static void take_sample(void)
{
	left[samples_taken] = adc_get_result(&AUDIO_IN, LEFT_AUDIO_IN_CHANNEL);
	right[samples_taken] = adc_get_result(&AUDIO_IN, RIGHT_AUDIO_IN_CHANNEL);
	samples_taken++;
}

static void adc_init(void)
{
	struct adc_config adca_conf;
	struct adc_channel_config adcch0_conf;
	struct adc_channel_config adcch1_conf;
	adc_read_configuration(&AUDIO_IN, &adca_conf);
	adcch_read_configuration(&AUDIO_IN, LEFT_AUDIO_IN_CHANNEL, &adcch0_conf);
	adcch_read_configuration(&AUDIO_IN, RIGHT_AUDIO_IN_CHANNEL, &adcch1_conf);
	adc_set_conversion_parameters(&adca_conf, ADC_SIGN_ON, ADC_RES_12,
	ADC_REF_BANDGAP);
	adc_set_conversion_trigger(&adca_conf, ADC_TRIG_FREERUN_SWEEP, 2, 0);
	adc_enable_internal_input(&adca_conf, ADC_INT_BANDGAP);
	adc_set_clock_rate(&adca_conf, 2000000UL);
	adcch_set_input(&adcch0_conf, ADCCH_POS_PIN0, ADCCH_NEG_PIN4, GAIN);
	adcch_set_input(&adcch1_conf, ADCCH_POS_PIN1, ADCCH_NEG_PIN5, GAIN);
	adc_write_configuration(&AUDIO_IN, &adca_conf);
	adcch_write_configuration(&AUDIO_IN, LEFT_AUDIO_IN_CHANNEL, &adcch0_conf);
	adcch_write_configuration(&AUDIO_IN, RIGHT_AUDIO_IN_CHANNEL, &adcch1_conf);
}