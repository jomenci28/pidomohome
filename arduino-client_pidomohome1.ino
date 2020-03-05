#include <Ethernet.h>
#include <PubSubClient.h>
#include <DHT.h>


//Configuració Ethernet
byte mac[] = { 0xDE, 0xED, 0xBF, 0xAF, 0xFF, 0xED };
IPAddress dnServer(1, 1, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress ip(192, 168, 1, 210);


//IP del servidor
IPAddress mqtt_server(192,168,1,220); // canviar IP segons rang dhcp si cal

//Definir client
EthernetClient ethClient;
PubSubClient client(ethClient);


//definir pins
#define DHTTYPE DHT22

const int LDRPin = A10;
const int DHTPin = 49;
const int DHT2Pin = 47;
const int PIRPin= 31;
const int BUZZPin = 39;
const int MQPin = 18;
const int POWERPin = 21;
const int WATERPin = 20;
const int DOORPin = 19;
const int SIRENAPin = 2;
const int INDICATIONPin = 3;
const int RELAYPin = 35;
const int RELAY2Pin = 41;
const int RELAY3Pin = 44;
const int RELAY4Pin = 4;
const int RELAY6Pin = 34;
const int RELAY5Pin = 30;
const int RELAY7Pin = 32;


DHT dht(DHTPin, DHTTYPE);
DHT dht2(DHT2Pin, DHTTYPE);

//definir variables dels sensors
unsigned long prevMillis1 = 0;
unsigned long prevMillis2 = 0;
unsigned long time_now_1 = 0;
unsigned long time_now_2 = 0;
float lum = 0;
char lumvalue[25];
char humvalue[30];
char hum2value[30];
char tempvalue[25];
char temp2value[25];
int pirState = 2;
int trigger = 0;
int hcsrState = 2;
volatile int mqState = 2;
volatile int waterState =2;
volatile int doorState =2;
volatile int sirenaState =2;
volatile int indicationState =2;
volatile int powerState =2;
int pos = 85;
int lightState;
int relayState;
int relay2State;
int relay3State;
int relay4State;
int relay5State;
int relay6State;
int relay7State;
char watervalue;
char doorvalue;
char sirenavalue;
char indicationvalue;
char powervalue;



//Reconnexió
void reconnect(){
  
  while (!client.connected()) {
    Serial.println("Connectant...");
    if (client.connect("Arduino_Client")) {
      Serial.println("Connectat");
    }else{
      Serial.print(F("Error de connexió, rc="));
      Serial.print(client.state());
      Serial.println(F("Es provarà de tornar a connectar en 3 segons"));
      delay(3000);
    }
  }
  
}


unsigned long timerLlum = 0;         // timer per emagatzemar temps quan llum baixa de x per cent de lluminositat
unsigned long delayLlum = 7200000;   // temps que el llum estara ences un cop detecti que lluminositat baixa
    
// Lectura lluminositat LDR
void lightRead(){
      Serial.println ("lightRead");

      int inputLDR = analogRead(LDRPin);
      lum = map(inputLDR, 0, 1023, 0, 100);//Definició del resultat a un valor en 0 i 100
      dtostrf(lum, 2, 1, lumvalue);
      client.publish("pidomohome/luminosity", lumvalue);


}

// Lectura temperatura i humitat DHT
void dhtRead(){
  Serial.println ("dhtRead");
  
  dht.begin();
  float t = dht.readTemperature();
  dtostrf(t, 1, 1, tempvalue);  //transforma float type a cadena de caracters -> 1 caracters maxims total i 1 decimals
  client.publish("pidomohome/temperature", tempvalue);
  Serial.println ("temperature done");
  float h = dht.readHumidity();
  dtostrf(h, 1, 1, humvalue);
  if (isnan(h) || isnan(t)) {
      Serial.println("Error de lectura del sensor de temperatura i humitat");
      return;
  }
  delay(300); // esperem a publicar per seguretat
  client.publish("pidomohome/humidity", humvalue);
  Serial.println ("humidity done");
}

// Lectura temperatura i humitat exteriors DHT2
void dht2Read(){
  Serial.println ("dht2Read");
  
  dht2.begin();
  float t = dht2.readTemperature();
  dtostrf(t, 1, 1, temp2value);  //transforma float type a cadena de caracters -> 1 caracters maxims total i 1 decimals
  client.publish("pidomohome/out_temperature", temp2value);
  Serial.println ("temperature2 done");
  float h = dht2.readHumidity();
  dtostrf(h, 1, 1, hum2value);
  if (isnan(h) || isnan(t)) {
      Serial.println("Error de lectura del sensor DHT-22");
      return;
  }
  delay(300); // esperem a publicar per seguretat
  client.publish("pidomohome/out_humidity", hum2value);
  Serial.println ("humidity2 done");
}

//Llegir moviment sensor PIR
void pirRead(){
  
  if(millis() > time_now_2 + 1000){//Esperem 1 segon entre lectures
    time_now_2 = millis();
    int state = digitalRead(PIRPin);
    if (state == HIGH){
      client.publish("pidomohome/movement", "OPEN");
      Serial.print("Sensor de moviment activat \n");
      pirState = 1;
      }

    if (state == LOW){
      pirState = 0;
      client.publish("pidomohome/movement", "CLOSED");
    }
}
}

//Llegir sensor gas MQ7
void mqRead(){
    bool gas = digitalRead(MQPin);

    if(gas and mqState != 1) {
      client.publish("pidomohome/fire", "OPEN");
      mqState = 1;   
      tone(BUZZPin, 500);
      Serial.println ("Alarma gas");
    }

    if (!gas and mqState != 0) {
      client.publish("pidomohome/fire", "CLOSED");
      mqState = 0;   // info published
      noTone(BUZZPin);
      Serial.println ("Gas OK");
    }
  }
  
//Llegir WaterSensor
void waterRead(){
    bool inputW = digitalRead(WATERPin);

    if(inputW and waterState != 1) {
      client.publish("pidomohome/flood", "OPEN");
      waterState = 1;  
      tone(BUZZPin, 500);
      Serial.println ("Alarma inundació");
    }

    if (!inputW and waterState != 0) {
      client.publish("pidomohome/flood", "CLOSED");
      waterState = 0; 
      noTone(BUZZPin);
      Serial.println ("Water OK");
    }
}

//Llegir door
void doorRead(){
    bool inputdoor = digitalRead(DOORPin);

    if(inputdoor and doorState != 1) {
      client.publish("pidomohome/door1", "OPEN");
      doorState = 1;   // info published
      digitalWrite(RELAY5Pin, HIGH);
      Serial.println ("open door");

    }

    if (!inputdoor and doorState != 0) {
      client.publish("pidomohome/door1", "CLOSED");
      doorState = 0;   // info published
      digitalWrite(RELAY5Pin, LOW);
      Serial.println ("closed door");
    }
}




//Llegir SIRENA
void sirenaRead(){
    bool inputsirena = digitalRead(SIRENAPin);

    if(inputsirena and sirenaState != 1) {
      client.publish("pidomohome/sirena", "ACTIU");
      client.publish("pidomohome/siren", "OPEN");
      sirenaState = 1;   // info published
      Serial.println ("on siren");

    }

    if (!inputsirena and sirenaState != 0) {
      client.publish("pidomohome/sirena", "NORMAL");
      client.publish("pidomohome/siren", "CLOSED");
      sirenaState = 0;   // info published
      Serial.println ("off siren");
    }
}




//Llegir INDICATION
void indicationRead(){
    bool inputindication = digitalRead(INDICATIONPin);

    if(inputindication and indicationState != 1) {
      client.publish("pidomohome/indication", "SISTEMA DESARMAT");
      client.publish("pidomohome/indication1", "OPEN");
      indicationState = 1;   // info published
      Serial.println ("system disarmed");

    }

    if (!inputindication and indicationState != 0) {
      client.publish("pidomohome/indication", "SISTEMA ARMAT");
      client.publish("pidomohome/indication1", "CLOSED");
      indicationState = 0;   // info published
      Serial.println ("system armed");
    }
}


//Llegir power
void powerRead(){
    bool inputpower = digitalRead(POWERPin);

    if(inputpower and powerState != 1) {
      client.publish("pidomohome/power", "BATERIA");
      client.publish("pidomohome/plugin", "OPEN");
      powerState = 1;   // info published
      Serial.println ("power OFF");

    }

    if (!inputpower and powerState != 0) {
      client.publish("pidomohome/power", "230V");
      client.publish("pidomohome/plugin", "CLOSED");
      powerState = 0;   // info published
      Serial.println ("power ON");
    }
}
  

//Funció Callback del protocol MQTT per subscriure's a uns determinats topics
void callback(char* topic, byte* payload, unsigned int length) {
 
if (strcmp ("pidomohome/light", topic) == 0){
    for (int i=0;i<length;i++) {
        char receivedChar = (char)payload[i];
        relayState = digitalRead(RELAYPin);

        if (receivedChar == 'F') {
          Serial.println ("light OFF");
          digitalWrite(RELAYPin, LOW);
        }

        if (receivedChar == 'N') {
          Serial.println ("light ON");
          digitalWrite(RELAYPin, HIGH);
        }
     }
  }



if (strcmp ("pidomohome/plug", topic) == 0){
    for (int i=0;i<length;i++) {
        char receivedChar = (char)payload[i];
        relay2State = digitalRead(RELAY2Pin);

        if (receivedChar == 'F') {
          Serial.println ("plug OFF");
          digitalWrite(RELAY2Pin, LOW);
        }

        if (receivedChar == 'N') {
          Serial.println ("plug ON");
          digitalWrite(RELAY2Pin, HIGH);
        }
     }
  }

if (strcmp ("pidomohome/light2", topic) == 0){
    for (int i=0;i<length;i++) {
        char receivedChar = (char)payload[i];
        relay3State = digitalRead(RELAY3Pin);

        if (receivedChar == 'F') {
          Serial.println ("light2 OFF");
          digitalWrite(RELAY3Pin, LOW);
        }

        if (receivedChar == 'N') {
          Serial.println ("light2 ON");
          digitalWrite(RELAY3Pin, HIGH);
        }
     }
  }

  if (strcmp ("pidomohome/heat", topic) == 0){
    for (int i=0;i<length;i++) {
        char receivedChar = (char)payload[i];
        relay4State = digitalRead(RELAY4Pin);

        if (receivedChar == 'F') {
          Serial.println ("heat OFF");
          digitalWrite(RELAY4Pin, LOW);
        }

        if (receivedChar == 'N') {
          Serial.println ("heat ON");
          digitalWrite(RELAY4Pin, HIGH);
        }
     }
  }

  if (strcmp ("pidomohome/alarma", topic) == 0){
    for (int i=0;i<length;i++) {
        char receivedChar = (char)payload[i];
        relay6State = digitalRead(RELAY6Pin);

        if (receivedChar == 'F') {
          Serial.println ("alarma OFF");
          digitalWrite(RELAY6Pin, LOW);
        }

        if (receivedChar == 'N') {
          Serial.println ("alarma ON");
          digitalWrite(RELAY6Pin, HIGH);
        }
     }
  }

  if (strcmp ("pidomohome/activation", topic) == 0){
    for (int i=0;i<length;i++) {
        char receivedChar = (char)payload[i];
        relay7State = digitalRead(RELAY7Pin);

        if (receivedChar == 'F') {
          Serial.println ("activation ON");
          digitalWrite(RELAY7Pin, HIGH);
        }

        if (receivedChar == 'N') {
          Serial.println ("activation OFF");
          digitalWrite(RELAY7Pin, LOW);
        }
     }
  }

  if (strcmp ("pidomohome/doortrial", topic) == 0){
    for (int i=0;i<length;i++) {
        char receivedChar = (char)payload[i];
        relay5State = digitalRead(RELAY5Pin);

        if (receivedChar == 'F') {
          Serial.println ("doortrial OFF");
          digitalWrite(RELAY5Pin, LOW);
        }

        if (receivedChar == 'N') {
          Serial.println ("doortrial ON");
          digitalWrite(RELAY5Pin, HIGH);
        }
     }
  }
  
}
  



void setup() {
  
  Serial.begin(9600);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(LDRPin, INPUT);
  pinMode(DHTPin, INPUT);
  pinMode(DHT2Pin, INPUT);
  pinMode(PIRPin, INPUT);
  pinMode(BUZZPin, OUTPUT);
  pinMode(RELAYPin, OUTPUT);
  pinMode(RELAY2Pin, OUTPUT);
  pinMode(RELAY3Pin, OUTPUT);
  pinMode(RELAY4Pin, OUTPUT);
  pinMode(RELAY5Pin, OUTPUT);
  pinMode(RELAY6Pin, OUTPUT);
  pinMode(RELAY7Pin, OUTPUT);
  pinMode(MQPin, INPUT_PULLUP);
  pinMode(WATERPin, INPUT_PULLUP);
  pinMode(SIRENAPin, INPUT_PULLUP);
  pinMode(INDICATIONPin, INPUT_PULLUP);
  pinMode(DOORPin, INPUT_PULLUP);
  pinMode(POWERPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WATERPin), waterRead, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SIRENAPin), sirenaRead, CHANGE);
  attachInterrupt(digitalPinToInterrupt(INDICATIONPin), indicationRead, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DOORPin), doorRead, CHANGE);
  attachInterrupt(digitalPinToInterrupt(MQPin), mqRead, CHANGE);
  attachInterrupt(digitalPinToInterrupt(POWERPin), powerRead, CHANGE);

  // while (true) {
  //   testRele();
  // }
  
}

void testRele() {
  Serial.println ("Test rele");
  digitalWrite(RELAYPin, HIGH);
  delay (1000);
  digitalWrite(RELAYPin, LOW);
  delay (1000);
}


bool mqttConnectedFlag = false;

void loop() {
  
  if (!client.connected()) {

    if (mqttConnectedFlag) mqttConnectedFlag = false; 

    Serial.print("Connectant el microcontrolador al port ...\n");
    delay(1000);

    // Attempt to start via DHCP. If not, do it manually:
    if (!Ethernet.begin(mac)) {
      Ethernet.begin(mac, ip, gateway, subnet);
    }
    // print IP address and start the server:
    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());

   
    reconnect();
    client.subscribe("pidomohome/#");
  }else{
    unsigned long time_now_1 = millis();
    if (time_now_1 - prevMillis1 >= 5000){
      Serial.print("Timer1\n");
      if (!mqttConnectedFlag) {   // Envia informacio gas i aigua nomes un cop quan es conecti al broker
        mqRead ();
        waterRead ();
        doorRead ();
        sirenaRead ();
        indicationRead ();
        powerRead ();
        mqttConnectedFlag = true;
      }

      lightRead();
      dhtRead();
      pirRead();
      light(); // on/off
      prevMillis1 = millis();

    }
    
  
    }
    client.loop();
  }
  delay(100);
 
}
