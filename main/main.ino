/*
 Name:    NhietDoCoNguongMax.ino
 Created: 6/17/2022 9:36:51 PM
 Author:  Pea
*/

#include <Arduino.h>
#include <OneWire.h>
#include <Adafruit_MLX90614.h>

// scl - a5, sda - a4
#define BTN0 A2 // btn for choosing mode
#define BTN1 A3 // btn for choosing temperature unit
#define muc 0
#define BOM 1
#define ALERT 0
// pin 10->13: pin for choosing digit to be displayed on 7 segment led
// pin 2->9: pin controlling the display of each segment of the led (abcdefg)
#define TRIG = A0; // Ultrasonic sensor's trigger pin
#define ECHO = A1; // Ultrasonic sensor's echo pin
Adafruit_MLX90614 cb = Adafruit_MLX90614();   // temperature's handler
unsigned long ta, tb, t, thoigian, tl;   // variables for timing 
float x;   // temperature in floating point 
int a, b, c;    // tens digit, units digit and tenths digit displayed on 7-segment led
int input = 0; // flag indicating there's a hand at the soap dispenser
int longin = 0; // flag indicating the hand remains at the dispensing position 
bool cf = 0; // flag indicating if temperature unit is degree or farenheit
             // 0 means degree, 1 means farenheit
int khoangcach; // distance between the hand and the sensor
int flag; // flag indicating there's a hand at the soap dispenser

int state = 0; // indicating program mode
enum State {
    docNhietDo,
    chinhNguong,
};
int thresHold = 200; // button holding time threshold // decrease temperature alert threshold continuously
float thres = 37.5; // temperature alert threshold
unsigned long pastChinhNguong = 0; // timing purposes
int thresChinhNguong = 4000; // timing purposes


int number[13][7] = {
  {0,0,0,0,0,0,1}, //0
  {1,0,0,1,1,1,1}, //1
  {0,0,1,0,0,1,0}, //2
  {0,0,0,0,1,1,0}, //3
  {1,0,0,1,1,0,0}, //4
  {0,1,0,0,1,0,0}, //5
  {0,1,0,0,0,0,0}, //6
  {0,0,0,1,1,1,1}, //7
  {0,0,0,0,0,0,0}, //8
  {0,0,0,0,1,0,0}, //9
  {0,1,1,0,0,0,1}, // C
  {0,1,1,1,0,0,0}, // F
  {1,1,1,1,1,1,1}, // led off
};

// display number from 0 to 9 and C/F
void display(int i) {
    for (int pin = 2; pin <= 8; pin++) {
        digitalWrite(pin, number[i][pin - 2]);
    }
}


/* code phía dưới là hiển thị nhiệt độ ra led theo nguyên tắc quét led
     bật chân điều khiển led cần sáng lên
     sau đó xuất tín hiệu hiển thị (hàm display())
     tạo sự delay
     sau đó tắt led, tắt luôn tín hiệu hiển thị để tránh
     led bị nhiễu và sáng những chỗ ko cần */

void xuat() { //hàm xuất không có dấu chấm
 // display tens digit on the 1st 7 segment led
    digitalWrite(10, 1); // select the first led to be on
    display(a); // display tens digit 
    delay(2); 
    digitalWrite(10, 0); // unselect the first led
    display(12);   // clear number

  // display units digit on the 2nd 7 segment led
    digitalWrite(11, 1); // select the second led to be on
    display(b); // display units digit
    delay(2); 
    digitalWrite(11, 0); // unselect the second led
    display(12); // clear number

    //display tenths digit on the 3rd 7 segment led
    digitalWrite(12, 1); // select the third led to be on
    display(c); // display tenths digit 
    delay(2); 
    digitalWrite(12, 0); // unselect the third led
    display(12); // clear number


    //display unit on the 4th 7 segment led
    digitalWrite(13, 1); // select the fourth led to be on
    display(int(cf) + 10); // display C/F unit 
    delay(2);
    digitalWrite(13, 0); // unselect the fourth led
    display(12); // clear unit
}

void xuatDP() { // display decimal point
  // display tens digit on the 1st 7 segment led
    digitalWrite(10, 1); // select the first led to be on
    display(a); // display tens digit 
    delay(2); 
    digitalWrite(10, 0); // unselect the first led
    display(12);   // clear number

  // display units digit on the 2nd 7 segment led
    digitalWrite(11, 1); // select the second led to be on
    display(b); // display units digit
    digitalWrite(9, 0);  // display decimal point
    delay(2); 
    digitalWrite(11, 0); // unselect the second led
    digitalWrite(9, 1);  // undisplay decimal point
    display(12); // clear number

  //display tenths digit on the 3rd 7 segment led
    digitalWrite(12, 1); // select the third led to be on
    display(c); // display tenths digit 
    delay(2); 
    digitalWrite(12, 0); // unselect the third led
    display(12); // clear number


    //display unit on the 4th 7 segment led
    digitalWrite(13, 1); // select the fourth led to be on
    display(int(cf) + 10); // display C/F unit 
    delay(2);
    digitalWrite(13, 0); // unselect the fourth led
    display(12); // clear unit
}

/*seperate digits*/
void hienthi(float x) {
    if (x < 100) {
        a = int(x) / 10;
        b = int(x) % 10;
        c = int((x) * 10) % 10;
        xuatDP();
    }
    else {
        a = int(x) / 100;
        b = (int(x) / 10) % 10;
        c = int(x) % 10;
        xuat();
    }
}

void vatcan() {
    //Hold trigger pin at high level for at least 10ms to signal the module to emit sound wave

    digitalWrite(TRIG, 0); //Set trigger pin to low level
    delayMicroseconds(2); 
    digitalWrite(TRIG, 1); // Hold trigger pin high for 10ms
    delayMicroseconds(10); 
    digitalWrite(TRIG, 0); // Set trigger pin to low level


    // echo will raise to high level right after the module receive the sound wave has emitted earlier
    // distance = (width of the pulse's high level * velocity of sound(340 m/s))/2
    // distance = width of the pulse's high level * 29.412 (velocity's unit is cm/microS)
    thoigian = pulseIn(ECHO, HIGH);
    khoangcach = int(thoigian / 2 / 29.412);


    if (khoangcach <= 15) { // If the distance is smaller than 15cm, an obsticle is detected
        flag = 1;          
    }
    else { flag = 0; }
    //Serial.println(flag);}
}



void setup() {
    // put your setup code here, to run once:
    pinMode(BOM, OUTPUT); // water pumping pin 
    digitalWrite(BOM, 0);
    pinMode(ALERT, OUTPUT); // alerting led pin
    digitalWrite(ALERT, 0);
    pinMode(BTN0, INPUT);
    pinMode(BTN1, INPUT);
    //Serial.begin(115200);

    cb.begin();  // initialize sensor
    pinMode(A0, OUTPUT); // sensor's trigger pin
    pinMode(A1, INPUT); // sensor's echo pin
    pinMode(A3, INPUT); // btn for choosing temperature unit

    for (int pin = 10; pin <= 13; pin++) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 0);
    }; 

    for (int pin = 2; pin <= 9; pin++) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, 1);
    }; 

}

void loop() {

    switch (state) {
    case docNhietDo:
        if (readBTN() == 3){ //both button are being pressed
          delay(100); // debouncing
          while (readBTN() == 3){} // wait until button is released
          state = 1; // enter mode chinhNguong
        delay(100);
        }
                
        vatcan();
        //donvi();
        //bên dưới là code kiểm tra xem tay có để dưới cảm biến trong vòng 1s hay ko
        if (flag != input) { 
            t = millis(); // mark the time when the sensor detects the hand
            input = flag;
        };

        if (millis() - t > 600) { // if the hand stays in front of the sensor for more than 0.6s
            longin = flag;  // turn on the flag indicating it's ready to read temperature of the hand and dispense soap
            t = millis();
        };

        if (longin == 1) {
            longin = 0;
            if (cf == 0) { x = cb.readObjectTempC(); } // read temperature in degree celcius  
            else { x = cb.readObjectTempF(); }; // read temperature in degree farenheit
            ta = millis(); tb=tl = millis(); //cho ta, tb bằng thời điểm hiện tại
              if(x>=thres){digitalWrite(ALERT,1);}; // turn on alerting led if temperature is higher than normal
                digitalWrite(BOM, 1);
              while (millis() - ta < 3000) {  // display temperature in 3 seconds
                if (millis() - tb > 1000) { digitalWrite(BOM, 0); } // puming for 2 seconds
                if (millis() - tl >2000) {digitalWrite(ALERT,0);}; // keeping the alerting led on for 1 seconds
                hienthi(x); //display
              }
        }             
        break;
    case chinhNguong:
      //Serial.println(digitalRead(BTN0));
      //Serial.println(digitalRead(BTN1));
      if (readBTN() == 0){ // 2 button are not being pressed 
        pastChinhNguong = millis();
        delay(100);
        while(readBTN() == 0){
          hienthi(thres); // display the thres where temperature above it is considered abnormal, therefore turn on the alert light
          if(millis() - pastChinhNguong > thresChinhNguong){ // 2 button are not being pressed for a certain amount of time
            state = 0; // back to normal mode which is measuring temperature and dispensing soap
            break;
          }
        }
        delay(100);
      }
      else if (readBTN() == 1){ // 1st button is pressed
        delay(100);
        thres -= 0.1; // decrease the temperature alert threshold
        if (thres < 34){ // the bottom line for temperature range is 34
          thres = 34;
        }
        unsigned long pastTang = millis(); 
        while(readBTN() == 1){ // 1st button is hold
          //quet LED
          hienthi(thres);
          //Serial.println(thres);
          if(millis() - pastTang > thresHold){
            thres -= 0.1; // decrease temperature alert threshold continuously
            pastTang = millis();
            if(thres < 34){
              thres = 34;
            }
          }
        }
        delay(100);
      }
      else if (readBTN() == 2) { // 2nd button is pressed
        delay(100);
        thres += 0.1; // increase the temperature alert threshold
        if (thres > 40){
          thres = 40;
        }
        unsigned long pastTang = millis();
        while(readBTN() == 2){ // 2nd button is being hold
          //quet LED
          hienthi(thres);
          //Serial.println(thres);
          if(millis() - pastTang > thresHold){
            thres += 0.1; // increase temperature alert threshold continuously
            pastTang = millis();
            if (thres > 40){
              thres = 40;
            }
          }
        }
        delay(100);
      }
      else { // 2 buttons are pressed
        delay(100); 
        while (readBTN() == 3){} // wait until the 2 button are released
        state = 0;
        delay(100);
      }
    }
}


int readBTN() {
    if (digitalRead(BTN0) == muc && digitalRead(BTN1) == muc) // 2 buttons are being pressed
        return 3;
    else if (digitalRead(BTN0) == muc && digitalRead(BTN1) != muc) // 1st button are being pressed whilst the second not
        return 2;
    else if (digitalRead(BTN0) != muc && digitalRead(BTN1) == muc) // 2nd button are being pressed whilst the first not
        return 1;
    else return 0; // 2 buttong are not being pressed
}
