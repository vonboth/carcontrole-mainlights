#include <Arduino.h>
#include "avr/sleep.h"

#define DIPLIGHT 8      //output dip lights
#define PARKLIGHT 9     //output park lights
#define FOGLIGHT 11     //output fog lights
#define REARLIGHT 12    //output rear lights
#define MAINLIGHT 13    //output main/beam
#define BTN_FOGLIGHT 4  //button fog lights
#define BTN_MAINLIGHT 5 //button main/beam
#define BTN_DIPLIGHT 6  //button dip lights
#define BTN_PARKLIGHT 7 //input park lights (switched by key)
#define ENGINE_ON 3     //interrupt signal pin
#define DELAY 100       //delay time [ms]
#define MODULO 5        //corresponds to delay and loop. e.g. flash every 500 ms (100 * 5)
#define SLEEP_TIME 10   //delay before Atmega goes to sleep [minutes]

int currentState = 0;
int fogState = 0;
long time = 0;
long powerOffTime = 0;
int enableSleep = 0;
unsigned int count = 0;

void handleLightState(int state) {
    switch(state) {
        case 1://lights on, fog off, main off
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(REARLIGHT, HIGH);
            digitalWrite(DIPLIGHT, HIGH);
            digitalWrite(MAINLIGHT, LOW);
            break;

        case 2://lights on, fog off, main on
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(REARLIGHT, HIGH);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, HIGH);
            break;

        case 3://lights with fog on
            digitalWrite(FOGLIGHT, HIGH);
            digitalWrite(REARLIGHT, HIGH);
            digitalWrite(DIPLIGHT, HIGH);
            digitalWrite(MAINLIGHT, LOW);
            break;

        case 4://main lights with fog on
            digitalWrite(FOGLIGHT, HIGH);
            digitalWrite(REARLIGHT, HIGH);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, HIGH);
            break;

        case 5: //only main lights high
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(REARLIGHT, LOW);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, HIGH);
            break;

        case 6: //park lights on
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(REARLIGHT, LOW);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, LOW);
            digitalWrite(PARKLIGHT, HIGH);
            break;
            
        default://all off
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(REARLIGHT, LOW);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, LOW);
            digitalWrite(PARKLIGHT, LOW);
            break;
    }
}

/**
 * interrupt wake up routine
 */
void wakeUp() {
    enableSleep = 0;
    handleLightState(0);
}

/**
 * power down device
 */
void gotoSleep() {
    handleLightState(0);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(1, wakeUp, HIGH);

    sleep_mode();
    sleep_disable();
    detachInterrupt(1);
}

/**
 * setup device
 */
void setup() {
    //Serial.begin(9600);
    //setup output pins
    pinMode(MAINLIGHT, OUTPUT);
    pinMode(DIPLIGHT, OUTPUT);
    pinMode(FOGLIGHT, OUTPUT);
    pinMode(REARLIGHT, OUTPUT);
    pinMode(PARKLIGHT, OUTPUT);

    //setup button pins
    pinMode(BTN_MAINLIGHT, INPUT);
    digitalWrite(BTN_MAINLIGHT, HIGH); //enable internal pullup
    pinMode(BTN_DIPLIGHT, INPUT);
    digitalWrite(BTN_DIPLIGHT, HIGH);  //enable internal pullup
    pinMode(BTN_FOGLIGHT, INPUT);
    digitalWrite(BTN_FOGLIGHT, HIGH);  //enable internal pullup
    pinMode(BTN_PARKLIGHT, INPUT);
    pinMode(ENGINE_ON, INPUT);

    handleLightState(0); //turn all off on start

    attachInterrupt(1, wakeUp, HIGH); //wake up on interrupt 1 == PIN 3 == power switch
}

/**
 * working loop
 */
void loop() {

    time = millis();

    int readBtnMainLight = digitalRead(BTN_MAINLIGHT);
    int readBtnDipLight = digitalRead(BTN_DIPLIGHT);
    int readBtnFogLight = digitalRead(BTN_FOGLIGHT);
    int readParkLight = digitalRead(BTN_PARKLIGHT);
    int readPowerOn = digitalRead(ENGINE_ON);

    //check if we will suspend
    if (enableSleep == 1) {
        if (readPowerOn == LOW &&
            readParkLight == LOW &&
            (time > (powerOffTime + SLEEP_TIME * 60 *1000 ))) {
            gotoSleep();
        }
    }

    //handle buttons
    if (readBtnDipLight == LOW) { //lights on

        //test if fog light button pressed and toggle it
        if (readBtnFogLight == LOW && count % MODULO == 0) {
            fogState = !fogState;
        }

        if (fogState == 1) {
            //fog on, dip on, main off
            if (readBtnMainLight == HIGH) {
                currentState = 3;
            }
            //fog light on, dip off, main on
            if (readBtnMainLight == LOW) {
                currentState = 4;
            }
        } else {
            //fog off, dip on, main off
            if (readBtnMainLight == HIGH) {
                currentState = 1;
            }
            //fog off, dip off, main on
            if (readBtnMainLight == LOW) {
                currentState = 2;
            }
        }

    } else if (readBtnDipLight == HIGH && readBtnMainLight == LOW) {
        //using main light on daytime
        currentState = 5; // only main lights on
        
    } else if (readPowerOn == LOW && readParkLight == HIGH) {
        enableSleep = 0;
        // swith on park lights
        currentState = 6;
    } else {
        //turn all off
        currentState = 0;
        fogState = 0;
    }

    //check
    if (readPowerOn == LOW && readParkLight == LOW && enableSleep == 0) {
        powerOffTime = time;
        enableSleep = 1;
    }

    handleLightState(currentState);

    count++;
    delay(DELAY);
}
