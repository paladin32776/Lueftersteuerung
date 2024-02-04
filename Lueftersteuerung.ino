// -----------------------------------
// Arduino Settings
// -----------------------------------
// Board: DOIT ESP-Mx DevKit (ESP8285)
// Flash Size: 1MB (FS:64KB OTA:470KB)
// CPU Frequency: 80 MHz
// -----------------------------------

#include <DS18B20.h>
#include "EnoughTimePassed.h"
#include "WiHomeComm.h"
#include "NoBounceButtons.h"
#include "SignalLED.h"

#define VERSION "v1.0"

#define PWM_PIN 4     	// GPIO pin for PWM output
#define PWM_MAX 1023	// For ESP8266 / ESP8285 maximal analog out is 1023
#define PWM_FREQ 10000	// For ESP8266 / ESP8285 maximal analog frequency is 40000
#define ONEWIRE_PIN 2   // GPIO pin for OneWire temperature sensor data signal
#define BUTTON_PIN 0    // GPIO pin for user button 
#define WIFI_LED_PIN 5  // GPIO pin for Wifi status led 

// Fan Parameter Defaults:
#define T_MAX_SPEED 40 	// Temperature at which fan is at maximum speed
#define T_MIN_SPEED 20 	// Minimum temperature for fan to run
#define V_MAX 0.5 		// Maximum fan speed (from 0 to 1)

#define FAN_UPDATE_INTERVAL 1000	// Update fan speed from temperature every 1000ms 

WiHomeComm* whc;
NoBounceButtons nbb;
SignalLED led(WIFI_LED_PIN, SLED_OFF, false);

char button0;
float T_max_speed, T_min_speed, v_max;
bool ds_found = false;
bool invert_pwm = false;
String html;
// uint8_t ds_address[8];

DS18B20 ds(ONEWIRE_PIN);
EnoughTimePassed etp_fan_update(FAN_UPDATE_INTERVAL);

// Function to calculate fan speed from measured temperature:
float speed(float T, float _T_min_speed, float _T_max_speed, float _v_max)
{
	float v = (T-_T_min_speed)/(_T_max_speed-_T_min_speed);
	if (v<0) v=0;
	if (v>1) v=1;
	return v*_v_max;
}

// Setup function executing once after reset:
void setup() 
{
	Serial.begin(115200);
	delay(500);
	
	// Set default parameter values for fan speed calculation:
	T_max_speed = T_MAX_SPEED;
	T_min_speed = T_MIN_SPEED;
	v_max = V_MAX;

	// Create WiHome object:
	whc = new WiHomeComm(false, true);
	// Attach config parameters we want to safe in the config file:
	whc->add_config_parameter(&T_max_speed, "T_max_speed", "T @ vmax");
	whc->add_config_parameter(&T_min_speed, "T_min_speed", "T @ vmin");
	whc->add_config_parameter(&v_max, "v_max", "v max");
	whc->add_config_parameter(&invert_pwm, "invert_pwm", "Invert PWM");

	// Setup user button and status led pin and attacged to WiHome object:
	whc->set_status_led(&led);
	button0 = nbb.create(BUTTON_PIN);
	whc->set_button(&nbb, button0, NBB_LONG_CLICK);

	// Searching for DS1820 sensor:
	if (ds.getNumberOfDevices()==1)
		if (ds.selectNext())
			ds_found = true;
			// ds.getAddress(ds_address);

	// Set PWM pin to output:
	analogWriteRange(PWM_MAX);
	analogWriteFreq(PWM_FREQ);
	pinMode(PWM_PIN, OUTPUT); 

	// Initiate String for additional html info on website when connected to Wifi:
	html = "";
	// Attached html String to WiHome object:
	whc->attach_html(&html);
}

// Loop function executing continuously:
void loop() 
{
	// Callback functions for buttons, WiHome, etc.:
	nbb.check();
 	whc->check();

 	// If enough time has passed since last fan speed update and we are not in SoftAP mode 
 	// and temperature sensor was found after reset:
	if ((etp_fan_update.enough_time()) && (whc->status()!=WIHOMECOMM_SOFTAP))		
	{
		// Verify sensor connection:
		// if (ds.select(ds_address))
		if (ds_found)
		{
			// Read temperature from sensor:
			float T = ds.getTempC();
			// Calculate fan speed from temperature:
			float v = speed(T, T_min_speed, T_max_speed, v_max);
			// Convert fan speed to PWM pin value and write it to pin:
			unsigned int pwm = round(((1-v)*(1-invert_pwm) + v*invert_pwm)*PWM_MAX);
			analogWrite(PWM_PIN, pwm);
			// Show status on serial interface:
			Serial.printf("T=%4.1fC TL=%4.1fC TH=%4.1fC v=%4.2f v_max: %4.2f pwm=%d\n", 
				T, T_min_speed, T_max_speed, v, v_max, pwm);
			// Content for website to be displayed when connected to Wifi: 
			html = "Software Version: ";
			html += VERSION;
			html += "<br>";
			html += "<br>Temperature: ";
			html += String(T);
			html += " &deg;C<br>Fan speed: ";
			html += String(v,2);
			html += "<br>";
		}
		else 
		{
			// Error handling for disconnected sensor:
			Serial.println("DS18B20 temperature sensor not found.");
			html = "Software Version: ";
			html += VERSION;
			html += "<br>";
			html += "<br>Temperature sensor not found.<br>";
			// Stop fans:
			analogWrite(PWM_PIN, PWM_MAX*(1-invert_pwm));
		}
	}
	delay(10);
}
