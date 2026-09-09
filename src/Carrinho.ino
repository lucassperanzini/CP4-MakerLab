#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_mac.h>


#define TRIG_PIN 5
#define ECHO_PIN 4
#define BUZZER_PIN 18  
#define LED 2          


const int MOTOR_ESQ_1 = 12; 
const int MOTOR_ESQ_2 = 13; 
const int MOTOR_DIR_1 = 14; 
const int MOTOR_DIR_2 = 27; 

long duracao;
float distancia;

#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"

BLECharacteristic *pCharacteristic;


void frente() {
  digitalWrite(MOTOR_ESQ_1, HIGH);  digitalWrite(MOTOR_ESQ_2, LOW);
  digitalWrite(MOTOR_DIR_1, HIGH);  digitalWrite(MOTOR_DIR_2, LOW);
}

void tras() {
  digitalWrite(MOTOR_ESQ_1, LOW);   digitalWrite(MOTOR_ESQ_2, HIGH);
  digitalWrite(MOTOR_DIR_1, LOW);   digitalWrite(MOTOR_DIR_2, HIGH);
}

void esquerda() {
  digitalWrite(MOTOR_ESQ_1, LOW);   digitalWrite(MOTOR_ESQ_2, HIGH); // Roda esquerda para trás
  digitalWrite(MOTOR_DIR_1, HIGH);  digitalWrite(MOTOR_DIR_2, LOW);  // Roda direita para frente
}

void direita() {
  digitalWrite(MOTOR_ESQ_1, HIGH);  digitalWrite(MOTOR_ESQ_2, LOW);  // Roda esquerda para frente
  digitalWrite(MOTOR_DIR_1, LOW);   digitalWrite(MOTOR_DIR_2, HIGH); // Roda direita para trás
}

void parar() {
  digitalWrite(MOTOR_ESQ_1, LOW);   digitalWrite(MOTOR_ESQ_2, LOW);
  digitalWrite(MOTOR_DIR_1, LOW);   digitalWrite(MOTOR_DIR_2, LOW);
}


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();

      if (value.length() > 0) {
        char comando = value[0]; 
        
        switch (comando) {
          case 'F': case 'f': frente();   break; // F = Frente
          case 'B': case 'b': tras();     break; // B = Trás (Back)
          case 'L': case 'l': esquerda(); break; // L = Esquerda (Left)
          case 'R': case 'r': direita();  break; // R = Direita (Right)
          case 'S': case 's': parar();    break; // S = Parar (Stop)
          
         
          case '1': digitalWrite(LED, HIGH); break;
          case '2': digitalWrite(LED, LOW);  break;
          
          default: parar(); break; 
        }
      }
    }
};

void setup() {
  Serial.begin(115200); 
  delay(500);        
  

  pinMode(LED, OUTPUT);           
  pinMode(TRIG_PIN, OUTPUT);    
  pinMode(ECHO_PIN, INPUT);     
  pinMode(BUZZER_PIN, OUTPUT);  
  
  pinMode(MOTOR_ESQ_1, OUTPUT); pinMode(MOTOR_ESQ_2, OUTPUT);
  pinMode(MOTOR_DIR_1, OUTPUT); pinMode(MOTOR_DIR_2, OUTPUT);
  
  parar(); 

  // Inicialização do Bluetooth BLE
  BLEDevice::init("ESP32_iPhone");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("ESP32 Inicializado com sucesso! Pronto para o Carrinho 2WD.");
}

void loop() {
  
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  
  duracao = pulseIn(ECHO_PIN, HIGH, 20000); 
  distancia = duracao * 0.034 / 2;


  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");

  
  if (distancia > 0 && distancia <= 5) {
    Serial.println("TA PERTO");
    tone(BUZZER_PIN, 3000); 
    
   
    parar(); 
  } else {
    noTone(BUZZER_PIN);     
  }

  delay(60); 
}
