#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

unsigned long startTime = 0;        // Start time of water measurement
unsigned long elapsedTime = 0;      // Elapsed time in milliseconds
unsigned long interval = 60;    //  minutes in milliseconds cout waterflow
//**********************************************************************************
//Conection WiFi
const char* ssid="NAME_MODEM";
const char* password="YOUADRESS";
//server URL
const char* ServerUrl="https://us-east-1.aws.data.mongodb-api.com/app/application-0-wqmkc/endpoint/people";
//URLPUT
const char* PutServerUrl="TOUR_ENDPOINT_METHOD_PUT";
//**
int relayPin =17;  
volatile int NumPulsos; //variable for the number of received pulses
int PinSensor = 27;    //Sensor connected to pin 2
float factor_conversion=7.5; //to convert from frequency to flow
float volumen=0;
long dt=0; //time variation for each loop
long t0=0; //millis() del bucle anterior
int contador=0;
bool relayActive;
int getRelay;
StaticJsonDocument<100> doc;
void ContarPulsos ()  
{ 
  NumPulsos++;  //we increase the pulse variable
} 
int ObtenerFrecuecia() 
{
  int frecuencia;
  NumPulsos = 0;   //We set the number of pulses to 0
  interrupts();    //We enable interruptions
  delay(1000);   //muestra de 1 segundo
  noInterrupts(); //We disable interruptions
  frecuencia=NumPulsos; //Hz(pulsos por segundo)
  return frecuencia;
}
void setup() {
   pinMode(relayPin, OUTPUT);
  pinMode(PinSensor, INPUT); 
   //conection
Serial.begin(115200);
WiFi.begin(ssid,password);
delay(2000);
Serial.print("Se esta conectando a la red wif denominada...");
Serial.print(ssid);
while(WiFi.status()!= WL_CONNECTED){
  delay(500);
  Serial.print("\n Connecting to Wi-Fi...");
}
//connection successful
Serial.print(" \n");
Serial.print("WiFi CONECTED... \n");
Serial.print("IP address: ");
Serial.print(WiFi.localIP());
//************************************************************
 /*  pinMode(relayPin, OUTPUT);
  pinMode(PinSensor, INPUT); */
  attachInterrupt(digitalPinToInterrupt(PinSensor),ContarPulsos,RISING);//(Interrupción 0(Pin2),función,Flanco de subida)
 
  t0=millis();

 getRelay=MGetRelay();
Serial.println("\n valor de get relay :");
Serial.println(getRelay);
}

void loop() {
    // put your main code here, to run repeatedly:
    Serial.print ("\n PETICION GET para relay ");
    if(getRelay==0 ){
       Serial.print ("\n relay apagado: ");
      digitalWrite(relayPin, HIGH);
      relayActive=false;
    }
    if(getRelay==1 ){
       Serial.print ("\n relay esta encendido: ");
   
      digitalWrite(relayPin, LOW);
      relayActive=true;
    }
    
    if(relayActive){
      
      while(interval >= contador){
  float frecuencia=ObtenerFrecuecia(); //we obtain the frequency of the pulses in Hz
  float caudal_L_m=frecuencia/factor_conversion; //We calculate the flow in L/m
  dt=millis()-t0; //calculate time variance
  t0=millis();
  volumen=volumen+(caudal_L_m/60)*(dt/1000); // volumen(L)=caudal(L/s)*tiempo(s)

   //-----Enviamos por el puerto serie---------------
  Serial.print ("\n Caudal: "); 
  Serial.print (caudal_L_m,3); 
  Serial.print ("L/min\tVolumen: "); 
  Serial.print (volumen,3); 
  Serial.println (" L");
  Serial.print("\n seconds active \t");
  Serial.print(contador);
  contador++;
  delay(1000);
      }
      Serial.print("\n relay deactivated \t");
      digitalWrite(relayPin, HIGH);
      relayActive=false;
      Serial.print ("\n Hacer metodo PUT ");
      Serial.print ("\n L/min\tVolumen: "); 
      Serial.print (volumen,3); 
      doc["potenciometer"]=0;
      doc["relay"]=0;
      doc["liters"] = volumen;
      Serial.println("\n Uploading dates.... ");
      PUTDATA();
      delay(5000);
      volumen=0;
      contador=0;
    }
    
     if (digitalRead(relayPin) == HIGH && !relayActive) {
    // Wait for the relay value to change to 1
    while (digitalRead(relayPin) == HIGH) {
      Serial.print ("\n Do get to get value from relay..... "); 
      getRelay=MGetRelay();
      Serial.println("\n getrelay value inside while: ");
      Serial.println(getRelay);
      delay(10000);
      if(getRelay==1){
      Serial.print ("\n relay on: ");
      digitalWrite(relayPin, LOW);
      relayActive=true;
      }
    }
  }
  Serial.print ("\n We got out of the wait and it was paid .... "); 
   delay(1000);
}


int MGetRelay(){
  HTTPClient http;
// Specify the URL to make the GET request to
  http.begin(ServerUrl);
  //code
  int httpResponseCode = http.GET();
  //store Get
  String payload = http.getString();
 // Serial.println("\n Codigo de estado: " + String(httpResponseCode));
 // Serial.println("\n valor de payload");
 //Serial.println(payload);

  char documentoJson[500];
  payload.replace(" ", "");
  payload.replace("\n", "");
  payload.trim();
  payload.remove(0,1);
  payload.remove(payload.length()-1,1);
  payload.toCharArray(documentoJson,500);

//  Serial.println(documentoJson);

  StaticJsonDocument<200> doc;

  deserializeJson(doc,documentoJson);
  int Getrelay = doc["relay"];
  //Serial.println("\n  relay:" + String(Getrelay) + "\n");
  return Getrelay;
}
void PUTDATA(){
  if(WiFi.status()==WL_CONNECTED){
    HTTPClient http;

    http.begin(PutServerUrl);
    http.addHeader("Content-Type","application/json");

    //serialize the json objetc to string
    String jsonString;
     serializeJson(doc,jsonString);

     Serial.println(jsonString);
  
     int httpResponseCode = http.PUT(jsonString);
     Serial.println(httpResponseCode);

     if(httpResponseCode == 200){
      Serial.println("Data uploaded");
     }else{
      Serial.println("ERROR : Couldn't uploat deata");
      Serial.print("\n HTTP response code: ");
       Serial.println(httpResponseCode);
     }
  }
}
