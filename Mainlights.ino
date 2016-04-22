#include <Arduino.h>
#include "avr/sleep.h"

#define MAINLIGHT 12
#define DIPLIGHT 11
#define FOGLIGHT 10
#define BACKLIGHT 9
#define PARKLIGHT 8
#define BTN_MAINLIGHT 2
#define BTN_DIPLIGHT 3
#define BTN_FOGLIGHT 4
#define BTN_PARKLIGHT 5
//PIN 0 == INTERRUPT
#define ENGINE_ON 0 //interrupt signal pin

//TODO: Standlicht!!

int currentState = 0;
int fogState = 0;
long time = 0;
long turnOffTime = 0;
int enableSleep = 0;
int count = 0;

void handleStates(int state) {
    switch(state) {
        case 1://lights on, fog off, main off
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(BACKLIGHT, HIGH);
            digitalWrite(DIPLIGHT, HIGH);
            digitalWrite(MAINLIGHT, LOW);
            break;

        case 2://lights on, fog off, main on
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(BACKLIGHT, HIGH);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, HIGH);
            break;

        case 3://lights with fog on
            digitalWrite(FOGLIGHT, HIGH);
            digitalWrite(BACKLIGHT, HIGH);
            digitalWrite(DIPLIGHT, HIGH);
            digitalWrite(MAINLIGHT, LOW);
            break;

        case 4://main lights with fog on
            digitalWrite(FOGLIGHT, HIGH);
            digitalWrite(BACKLIGHT, HIGH);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, HIGH);
            break;

        case 5: //only main lights high
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(BACKLIGHT, LOW);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, HIGH);
            break;

        case 6: //park lights on
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(BACKLIGHT, LOW);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, LOW);
            digitalWrite(PARKLIGHT, HIGH);
            break;
            
        default://all off
            digitalWrite(FOGLIGHT, LOW);
            digitalWrite(BACKLIGHT, LOW);
            digitalWrite(DIPLIGHT, LOW);
            digitalWrite(MAINLIGHT, LOW);
            break;
    }
}

/**
 * interrupt wake up routine
 */
void wakeUp() {
    enableSleep = 0;
    handleStates(0);
}

/**
 * power down device
 */
void gotoSleep() {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(0, wakeUp, HIGH);

    sleep_mode();
    sleep_disable();
    detachInterrupt(0);
}

/**
 * setup device
 */
void setup() {
    //Serial.begin(9600);
    // put your setup code here, to run once:
    pinMode(MAINLIGHT, OUTPUT);
    pinMode(DIPLIGHT, OUTPUT);
    pinMode(FOGLIGHT, OUTPUT);
    pinMode(BACKLIGHT, OUTPUT);
    pinMode(BTN_MAINLIGHT, INPUT);
    pinMode(BTN_DIPLIGHT, INPUT);
    pinMode(BTN_FOGLIGHT, INPUT);
    pinMode(ENGINE_ON, INPUT);

    handleStates(0); //turn all off on start

    attachInterrupt(0, wakeUp, HIGH); //attach wakeup
}

/**
 * working loop
 */
void loop() {

    time = millis();

    int readMain = digitalRead(BTN_MAINLIGHT);
    int readDip = digitalRead(BTN_DIPLIGHT);
    int readFog = digitalRead(BTN_FOGLIGHT);
    int readPark = digitalRead(BTN_PARKLIGHT);
    int readPowerOn = digitalRead(ENGINE_ON);

    //check if we will suspend
    if (enableSleep == 1) {
        if (readPowerOn == LOW &&
            readPark == LOW &&
            (time > (turnOffTime * 60 *1000)) ) {
            gotoSleep();
        }
    }

    //handle buttons
    if (readDip == HIGH) { //lights on
  
        //test if fog light button pressed and toggle it
        if (readFog == HIGH && count%5 == 0) {
            fogState = !fogState;
        }

        if (fogState == 1) {
            //fog on, dip on, main off
            if (readMain == LOW) {
                currentState = 3;
            }
            //fog light on, dip off, main on
            if (readMain == HIGH) {
                currentState = 4;
            }
        } else {
            //fog off, dip on, main off
            if (readMain == LOW) {
                currentState = 1;
            }
            //fog off, dip off, main on
            if (readMain == HIGH) {
                currentState = 2;
            }
        }

    } else if (readDip == LOW && readMain == HIGH) {
        //using main light on daytime
        currentState = 5; // only main lights on
        
    } else if (readDip == LOW && readPark == HIGH && readPowerOn == LOW) {
        enableSleep = 0;
        // swith on park lights
        currentState = 6;
        
    } else if (readPowerOn == LOW && readPark == LOW && enableSleep == 0) {
        enableSleep = 1;
        turnOffTime = millis();
        
    } else {
        //turn all off
        currentState = 0;
        fogState = 0;
    }

    handleStates(currentState);

    count++;
    delay(100);
}
