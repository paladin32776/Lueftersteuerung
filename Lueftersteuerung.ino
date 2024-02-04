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

#define PWM_PIN 4
#define PWM_MAX 1023	// For ESP8266 / ESP8285 maximal analog out is 1023
#define ONEWIRE_PIN 2
#define BUTTON_PIN 0
#define WIFI_LED_PIN 5

// Fan Parameters:
#define T_MAX_SPEED 40 	// Temperature at which fan is at maximum speed
#define T_MIN_SPEED 20 	// Minimum temperature for fan to run
#define V_MAX 0.5 		// Maximum fan speed (from 0 to 1)

#define FAN_UPDATE_INTERVAL 1000	// Update fan speed from temperature every 1000ms 

WiHomeComm* whc;
NoBounceButtons nbb;
SignalLED led(WIFI_LED_PIN, SLED_OFF, false);

char button0;
float T_max_speed, T_min_speed, v_max;
String html;

DS18B20 ds(ONEWIRE_PIN);
EnoughTimePassed etp_fan_update(FAN_UPDATE_INTERVAL);

float speed(float T)
{
	float v = (T-T_min_speed)/(T_max_speed-T_min_speed);
	if (v<0) v=0;
	if (v>1) v=1;
	return v*v_max;
}

void setup() 
{
	Serial.begin(115200);
	delay(500);
	
	T_max_speed = T_MAX_SPEED;
	T_min_speed = T_MIN_SPEED;
	v_max = V_MAX;

	whc = new WiHomeComm(false, true);
	whc->add_config_parameter(&T_max_speed, "T_max_speed", "T @ vmax");
	whc->add_config_parameter(&T_min_speed, "T_min_speed", "T @ vmin");
	whc->add_config_parameter(&v_max, "v_max", "v max");

	whc->set_status_led(&led);
	button0 = nbb.create(BUTTON_PIN);
	whc->set_button(&nbb, button0, NBB_LONG_CLICK);

	Serial.print("Devices: ");
	Serial.println(ds.getNumberOfDevices());
  	Serial.println("ds search:");
	uint8_t is_ds=0;
	while (!is_ds)
	{
		is_ds = ds.selectNext();
		Serial.println(is_ds);
	}
	Serial.println(is_ds);
	Serial.println("ds search done.");
	Serial.println();
	pinMode(PWM_PIN, OUTPUT); // Set PWM pin to output

	html = "";
	whc->attach_html(&html);
}

void loop() 
{
	//Serial.print("LOOP | ");
	nbb.check();
	//Serial.print("LED | ");
 	whc->check();
 	//Serial.print("WHC\n");

	if ((etp_fan_update.enough_time()) && (whc->status()!=WIHOMECOMM_SOFTAP))		
	{
		float T = ds.getTempC();
		float v = speed(T);
		unsigned int pwm = round((1-v)*PWM_MAX);
		analogWrite(PWM_PIN, pwm);
		Serial.printf("T=%4.1fC TL=%4.1fC TH=%4.1fC v=%4.2f v_max: %4.2f pwm=%d\n", 
			T, T_min_speed, T_max_speed, v, v_max, pwm);
		html = "<br>Temperature: ";
		html += String(T);
		html += " &deg;C<br>Fan speed: ";
		html += String(v,2);
		html += "<br>";
	}
	//Serial.print("BEFORE DELAY | ");
	delay(10);
	//Serial.print("AFTER DELAY\n");
}
