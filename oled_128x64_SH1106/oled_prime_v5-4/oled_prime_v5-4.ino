/* Prime numbers in Arduino C v5.4 2023/12/22 for ESP8266-12E with SH1106 oled */
/* Update    */

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>
#include <math.h>
#include <ESP_EEPROM.h>

U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 12, /* data=*/ 14);

double start;
int column = 10;
int found = 4;   // we already know 2, 3, 5, 7
int divisors = found;
uint32_t primes[6550] = {3, 5, 7};
int led = LED_BUILTIN;

void setup() {
  Serial.begin(74880);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  u8g2.begin();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 8," Calculating primes v5.4");
    u8g2.drawStr(0,16,"to number   time seconds");
    u8g2.drawStr(0,24,"       100 ");
    u8g2.drawStr(0,32,"     1,000 ");
    u8g2.drawStr(0,40,"    10,000 ");
    u8g2.drawStr(0,48,"   100,000 ");
    u8g2.drawStr(0,56," 1,000,000 ");
    u8g2.drawStr(0,64,"10,000,000 ");
  } while (u8g2.nextPage() );

  for (int i = 0; i < 3; i++) {
    Serial.print(".");
    delay(1000);
  }
  digitalWrite(led, LOW);
  const uint32_t scope[] = {100, 1000, 10000, 100000, 1000000, 10000000, 25000000, 100000000, 1000000000, 2147483647, 4294967295 };
  const int reference[] = {25, 168, 1229, 9592, 78498, 664579, 1565927, 5761455, 50847534, 105097564, 203280221};

  // previous run
  EEPROM.begin(44);
  Serial.print("\nGet previous results for this ESP8266:\n");
  Serial.print("    last        seconds   \n");
  for(int i = 0; i < 11; i++) {
    int spaces = 12 - (int)log10(scope[i]);
    for(int j = 0; j < spaces; j++) {
      Serial.print(" ");
    }
    Serial.print(scope[i]);
    Serial.print("    ");
    float last_time;
    EEPROM.get(i * 4 + 1, last_time);
    Serial.print(last_time, 6);
    Serial.print("\n");
    if( i < 6) {
      char buff[10];
      snprintf (buff, sizeof(buff), "%.6f", last_time);
      do {
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(64, i * 8 + 24, buff);
      } while (u8g2.nextPage() );
    }
  }

  // start calculating with micros
  for (int i = 0; i < 6; i++) 
  {
    int last = scope[i];
    found = 4;   // we already know 2, 3, 5, 7
    Serial.println("\n\nPrime v5.4 in Arduino C - 2023/12/22 in micros()");
    Serial.print("Calculating prime numbers until ");
    Serial.println(last);
    // start = millis();      
    start = micros();        // use micros() for more precision in runtimes < 70 minutes
    int largest_divider = (int)(sqrt(last)); 
    if(largest_divider % 2 == 0)
    {
      largest_divider += 1;
    }
    find_primes(largest_divider);
    Serial.print("Found ");
    Serial.print(found);
    Serial.print(" primes until ");
    Serial.print(largest_divider);
    Serial.print(" to use as divisors.\n");
    int dot = millis();
    int column = 0;
    for(uint32_t number = largest_divider + 2; number < last; number += 2)
    {
      found += is_prime_fast(number);
      if((millis() - dot) > 1000) {
        Serial.print(".");
        dot = millis();
        column += 1;
        if(column % 2 == 0) {
          digitalWrite(led, HIGH);
        }
        else
        {
          digitalWrite(led, LOW);
        }
        if(column > 40) {
          column = 0;
          elapsed_time((micros()-start)/1000000);
          Serial.print(" - ");
          Serial.print(number);
          Serial.print(" ");
          Serial.print((int)(number / (last / 100)));
          Serial.print("% \n");
        }
      }
    }
    // float duration = (millis() - start)/1000;
    float duration = (micros() - start)/1000000; // use micros() for more precision in runtimes < 70 minutes
    if(duration > 2) {
      Serial.print("\n");
    }    
    Serial.print("Found ");
    Serial.print(found);
    Serial.print(" prime numbers. It should be ");
    Serial.print(reference[i]);
    Serial.print(".\nThis took ");
    Serial.print(duration, 6);
    Serial.print(" seconds.");
    elapsed_time(duration);
    EEPROM.put(i * 4 + 1, duration);
    boolean ok1 = EEPROM.commit();
    Serial.println((ok1) ? "First commit OK" : "Commit failed");    
  }
  Serial.print("\n");
}

void loop() {  // program finished, just keep printing some dots
  for(int i = 0; i < 80; i++) {
    Serial.print(".");
    for(int x = 0; x < 100; x++) {
      digitalWrite(led, HIGH);
      delay(20);     
      digitalWrite(led, LOW); 
      delay(30);   
    }
  }
  Serial.print(".\n");
}

int is_prime(uint32_t number) {
  int prime = 1;
  for (int divider = 3; divider < (int)(sqrt(number)) + 1; divider += 2)
  {
    if (number % divider == 0)
    {
      prime = 0;
      break;
    }
  }
  return prime;
}

int find_primes(uint32_t limit) {
  Serial.print(" ");
  for(uint32_t number=11; number < limit + 1; number += 2) { 
    if( is_prime(number) == 1) {
      primes[found - 1] = number;
      found++;
    }
    if((number+1) % 1000 == 0) { Serial.print("."); }
  }
  primes[found - 1] = limit;
  divisors = found - 1;
  return 1;
}

int is_prime_fast(uint32_t number) {
  uint32_t largest_divider = (int)(sqrt(number));
  int flag_prime = 1;
  for(int i=0; i < divisors; i++)
  {
    if(number % primes[i] == 0) 
    {
      flag_prime = 0;
      break;
    }
    if(primes[i] > largest_divider)
    {
      break;
    }
  }
  return flag_prime;
}

void elapsed_time(int seconds) {
  int hours = (int)seconds/3600;
  int minutes = (int)(seconds/60 - hours*60);
  int sec = (int)(seconds - minutes*60 - hours*3600);
  Serial.print(" ");
  Serial.print(hours);
  Serial.print("h ");
  Serial.print(minutes);
  Serial.print("min ");
  Serial.print(sec);
  Serial.print("s ");
}

