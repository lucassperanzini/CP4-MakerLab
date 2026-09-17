#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_mac.h>

// Definição dos pinos do sensor ultrassônico e LED
#define TRIG_PIN 5
#define ECHO_PIN 4
#define BUZZER_PIN 18  // Pino do Buzzer
#define LED 2          // Pino do LED azul integrado

// =========================================================================
// MAPEAMENTO DOS PINOS DOS MOTORES
// =========================================================================
const int MOTOR_ESQ_1 = 25; 
const int MOTOR_ESQ_2 = 33; 
const int MOTOR_DIR_1 = 32; 
const int MOTOR_DIR_2 = 35; 

long duracao;
float distancia;
bool tocarMusica = false;
bool movimentoLiberado = false; 

// Variáveis para controle de tempo ao soltar o botão
unsigned long ultimoComandoTempo = 0;
const unsigned long TIMEOUT_PARADA = 150; // Se ficar 150ms sem receber sinal, para sozinho!

#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"

BLECharacteristic *pCharacteristic;

// =========================================================================
// FUNÇÕES DE MOVIMENTAÇÃO
// =========================================================================
void parar() {
  digitalWrite(MOTOR_ESQ_1, LOW);   digitalWrite(MOTOR_ESQ_2, LOW);
  digitalWrite(MOTOR_DIR_1, LOW);   digitalWrite(MOTOR_DIR_2, LOW);
}

void frente() {
  digitalWrite(MOTOR_ESQ_1, HIGH);  digitalWrite(MOTOR_ESQ_2, LOW);
  digitalWrite(MOTOR_DIR_1, HIGH);  digitalWrite(MOTOR_DIR_2, LOW);
}

void tras() {
  digitalWrite(MOTOR_ESQ_1, LOW);   digitalWrite(MOTOR_ESQ_2, HIGH);
  digitalWrite(MOTOR_DIR_1, LOW);   digitalWrite(MOTOR_DIR_2, HIGH);
}

void esquerda() {
  digitalWrite(MOTOR_ESQ_1, LOW);   digitalWrite(MOTOR_ESQ_2, HIGH); 
  digitalWrite(MOTOR_DIR_1, HIGH);  digitalWrite(MOTOR_DIR_2, LOW);  
}

void direita() {
  digitalWrite(MOTOR_ESQ_1, HIGH);  digitalWrite(MOTOR_ESQ_2, LOW);  
  digitalWrite(MOTOR_DIR_1, LOW);   digitalWrite(MOTOR_DIR_2, HIGH); 
}

void efeitoArcadeStart() {
  tone(BUZZER_PIN, 1319, 150); 
  delay(150);
  tone(BUZZER_PIN, 1568, 150); 
  delay(150);
  tone(BUZZER_PIN, 2093, 150); 
  delay(150);
  tone(BUZZER_PIN, 2637, 300); 
  delay(300);
  noTone(BUZZER_PIN);          
}

// =========================================================================
// LEITURA DE COMANDOS ENVIADOS PELO IPHONE
// =========================================================================
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();

      if (value.length() > 0) {
        char comando = value[0]; 
        
        // Atualiza o cronômetro sempre que um comando de movimento é recebido
        ultimoComandoTempo = millis(); 

        switch (comando) {
          case 'F': case 'f': 
            if (movimentoLiberado) frente();   
            break; 
          case 'B': case 'b': 
            if (movimentoLiberado) tras();     
            break; 
          case 'L': case 'l': 
            if (movimentoLiberado) esquerda(); 
            break; 
          case 'R': case 'r': 
            if (movimentoLiberado) direita();  
            break; 

          case 'P': case 'p': 
            if (movimentoLiberado) {
              parar();
              fireRedIntro(BUZZ_PIN,LED);  
            }
            break; 
          
          case '1': 
            digitalWrite(LED, HIGH);
            tocarMusica = true; 
            break;

          case '2': 
            digitalWrite(LED, LOW);
            tocarMusica = false; 
            movimentoLiberado = false; 
            noTone(BUZZER_PIN);
            parar();
            break;
          
          default: 
            parar(); 
            break;
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

  Serial.println("ESP32 Inicializado com sucesso!");
}

void loop() {
  if (tocarMusica) {
    efeitoArcadeStart();
    tocarMusica = false;       
    movimentoLiberado = true;  

    delay(200);
    digitalWrite(LED, LOW);
    delay(100);
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED, LOW);
    delay(200);
    digitalWrite(LED, HIGH);
    digitalWrite(LED, LOW);    
  }

  // Se o carrinho estiver andando, verifica se o tempo sem receber comandos estourou
  if (movimentoLiberado) {
    // Se passou mais tempo do que o TIMEOUT_PARADA sem receber sinal novo do celular, significa que o usuário soltou o botão!
    if (millis() - ultimoComandoTempo > TIMEOUT_PARADA) {
      parar(); // Para sozinho imediatamente
    }
  }

  // =========================================================================
  // LEITURA DO SENSOR ULTRASSÔNICO
  // =========================================================================
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duracao = pulseIn(ECHO_PIN, HIGH, 20000); 
  distancia = duracao * 0.034 / 2;

  if (movimentoLiberado && distancia > 0 && distancia <= 5) {
    Serial.println("TA PERTO");
    tone(BUZZER_PIN, 4000); 
    parar(); 
    tras();      
    delay(1000);
    parar();
  } else {
    noTone(BUZZER_PIN);     
  }

  delay(50); 
}