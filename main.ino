#include <Wire.h>            // incluye libreria para interfaz I2C https://www.youtube.com/watch?v=5et-N1-pK9A
#include <Time.h>
#include <TimeAlarms.h>
#include "RTClib.h"         // libreria para el reloj
#include <OneWire.h>
#include "DallasTemperature.h"

// RTC_DS1307 rtc;
RTC_DS3231 rtc;

char t[32];

bool evento_inicio = true;         //variable de control para inicio de evento con valor true
bool evento_fin = true;            // variable de control para finalización de evento con valor true

# define RELE 7
// constante RELE con valor 12 que corresponde a pin digital 7

# define bombaAguaRele1 0 // Bomba de agua
# define peltier1Rele2 1  
# define peltier2Rele3 2 
//#define rele4 3

//Sensores de temperatura
int pinTempSensor1 = A0;
int pinTempSensor2 = A1;
OneWire ourWire1(pinTempSensor1);
OneWire ourWire2(pinTempSensor2);

DallasTemperature sensors1(&ourWire1);
DallasTemperature sensors2(&ourWire2);
int temp1;
int temp2;
int umbralTemp1 = 25;
int umbralTemp2 = 27;


// Variables para el uso de la bomba de agua
int misHoras[] = {10, 12, 14, 16, 18, 20, 22};
int horaEjecutada = 0;
int minutosStartBomba = 0; // Minutos de la hora en la que se enciende, que se enciende la bomba (intentar que el inicio + tiempo encendido no sean 60
int tiempoEncendidoBomba = 5; // minutos

// Nivel del agua
int pinNivelAgua = 4;



void setup() {
  Serial.begin(9600);            // Iniciializa comunicación serie a 9600 bps
  Wire.begin();
  pinMode(bombaAguaRele1, OUTPUT);
  pinMode(peltier1Rele2, OUTPUT)  ;
  pinMode(peltier2Rele3, OUTPUT);
  //pinMode(rele4, OUTPUT);
  pinMode(pinNivelAgua, OUTPUT); 
  
  // Las variables tienen un funcionamiento al reves, LOW apaga HIGH enciende
  digitalWrite(bombaAguaRele1, HIGH); // Este rele se enciendo en LOW y se apaga en HIGH
  digitalWrite(peltier1Rele2, HIGH); // Este rele se enciendo en LOW y se apaga en HIGH
  digitalWrite(peltier2Rele3, HIGH); // Este rele se enciendo en LOW y se apaga en HIGH
  //digitalWrite(rele4, HIGH); // Este rele se enciendo en LOW y se apaga en HIGH
  digitalWrite(pinNivelAgua, HIGH); // Este rele se enciendo en LOW y se apaga en HIGH


  //Begin sensores temp
  sensors1.begin();
  sensors2.begin();



if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  // Configuracion de la hora si se apaga la bateria
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //rtc.begin()
  //setSyncProvider(rtc.now);

}



void loop() {
  //El codigo solo funciona si el sensor de nivel esta a 1
  Serial.print(digitalRead(pinNivelAgua) + "\n");
  if (digitalRead(pinNivelAgua) == LOW){
  
    DateTime now = rtc.now();            //captura la Hora
    sprintf(t, "%02d:%02d:%02d %02d/%02d/%02d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    
    Serial.print(F("Date/Time: "));
    Serial.println(t);
  
    //cojemos la temperatura
    temp1 = getTemperatura(sensors1);
    temp2 = getTemperatura(sensors2);
  
    Serial.print("Temperatura1=");
    Serial.print(temp1);
    Serial.print("\nTemperatura2=");
    Serial.print(temp2);
    Serial.print("\n");
  
    //Funcionamiento normal del sistema, peltiers apagados y bomba funcionamiento normal
    if (temp1 < umbralTemp1 && temp2 < umbralTemp1){
      //Esta variable la vamos a utilizar como control de uso de las excepciones de la temperatura, cuando salta la temperatura ponemos el valor a 0 para aqui poder apagarla al volver a la
      // temperatura optima
      if (horaEjecutada == 0){
        apagarBomba();
      }
      checkEncenderBomba();
      if (digitalRead(bombaAguaRele1) == LOW){
          checkApagarBomba();
      }
      if (digitalRead(peltier1Rele2) == LOW){
          digitalWrite(peltier1Rele2, HIGH);
      }
      if (digitalRead(peltier2Rele3) == LOW){
          digitalWrite(peltier2Rele3, HIGH);
      }
    }
    if (temp1 >= umbralTemp1 || temp2 >= umbralTemp1){
      //Funcionamiento normal (hay excepciones de uso segun el resto de los reles)
      encenderBomba();
      horaEjecutada = 0;
      if (digitalRead(peltier1Rele2) == HIGH){
          digitalWrite(peltier1Rele2, LOW);
      }
      if (digitalRead(peltier2Rele3) == LOW){
          digitalWrite(peltier2Rele3, HIGH);
      }
    }
    if (temp1 >= umbralTemp2 || temp2 >= umbralTemp2){
      //Funcionamiento normal (hay excepciones de uso segun el resto de los reles)
      encenderBomba();
      horaEjecutada = 0;
      if (digitalRead(peltier1Rele2) == HIGH){
          digitalWrite(peltier1Rele2, LOW);
      }
      if (digitalRead(peltier2Rele3) == HIGH){
          digitalWrite(peltier2Rele3, LOW);
      }
    }
  }else{
    Serial.print("Deposito vacio\n");
    apagarBomba();
    digitalWrite(peltier1Rele2, HIGH);
    digitalWrite(peltier2Rele3, HIGH);
    
  }
  
  delay(1000);
}



int getTemperatura(DallasTemperature sensors){
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  return temp;
}


void checkEncenderBomba(){
   for (int elem = 0; elem < sizeof(misHoras); elem++) {
    if (misHoras[elem] == rtc.now().hour() && minutosStartBomba == rtc.now().minute() && misHoras[elem] != horaEjecutada){
      encenderBomba();
      horaEjecutada = misHoras[elem];
    }
  }
}

void checkApagarBomba(){
  if (rtc.now().minute() >= minutosStartBomba + tiempoEncendidoBomba){
    Serial.print("\nApagar Bomba SUMA:");
    Serial.print(minutosStartBomba + tiempoEncendidoBomba);
    apagarBomba();
  }
}

void encenderBomba(){
  digitalWrite(bombaAguaRele1, LOW);
  //Serial.print("encender Bomba\n");
}

void apagarBomba(){
  digitalWrite(bombaAguaRele1, HIGH);
  //Serial.print("Apagar Bomba\n");
  
}
