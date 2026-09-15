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
// MAPEAMENTO DOS PINOS DOS MOTORES (Passe essa lista para o seu amigo!)
// =========================================================================
const int MOTOR_ESQ_1 = 12; // Pino D12 ➔ Entrada IN1 do Driver
const int MOTOR_ESQ_2 = 13; // Pino D13 ➔ Entrada IN2 do Driver
const int MOTOR_DIR_1 = 14; // Pino D14 ➔ Entrada IN3 do Driver
const int MOTOR_DIR_2 = 27; // Pino D27 ➔ Entrada IN4 do Driver

long duracao;
float distancia;

bool tocarMusica = false;

#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"

BLECharacteristic *pCharacteristic;

// =========================================================================
// FUNÇÕES DE MOVIMENTAÇÃO DO CARRINHO
// =========================================================================
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

void efeitoArcadeStart() {
  tone(BUZZER_PIN, 1319, 150); // Nota Mi (E6)
  delay(150);
  tone(BUZZER_PIN, 1568, 150); // Nota Sol (G6)
  delay(150);
  tone(BUZZER_PIN, 2093, 150); // Nota Dó (C7)
  delay(150);
  tone(BUZZER_PIN, 2637, 300); // Nota Mi de novo (E7) - mais longa
  delay(300);
  noTone(BUZZER_PIN);          // Desliga o som
}



// =========================================================================
// LEITURA DE COMANDOS ENVIADOS PELO IPHONE (MAPPED GAMEPAD VIA TEXT)
// =========================================================================
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();

      if (value.length() > 0) {
        char comando = value[0]; // Pega a primeira letra enviada pelo app
        
        switch (comando) {
          case 'F': case 'f': frente();   break; // F = Frente
          case 'B': case 'b': tras();     break; // B = Trás (Back)
          case 'L': case 'l': esquerda(); break; // L = Esquerda (Left)
          case 'R': case 'r': direita();  break; // R = Direita (Right)
          case 'S': case 's': parar();    break; // S = Parar (Stop)
          
          // Mantém o teste do LED azul se você quiser piscar para testar
        case '1': 
            digitalWrite(LED, HIGH);
            tocarMusica = true; // Apenas ativa o gatilho, sem travar aqui!
            break;

          case '2': 
            digitalWrite(LED, LOW);
            tocarMusica = false; // Desativa se o comando for recebido
            break;
          
          default: parar(); break;
        }
      }
    }
};

void setup() {
  Serial.begin(115200); // Inicializa a comunicação serial com o computador
  delay(500);           // Pausa para deixar o monitor estável
  
  // Configuração dos Pinos como Saídas e Entradas
  pinMode(LED, OUTPUT);           
  pinMode(TRIG_PIN, OUTPUT);    
  pinMode(ECHO_PIN, INPUT);     
  pinMode(BUZZER_PIN, OUTPUT);  
  
  pinMode(MOTOR_ESQ_1, OUTPUT); pinMode(MOTOR_ESQ_2, OUTPUT);
  pinMode(MOTOR_DIR_1, OUTPUT); pinMode(MOTOR_DIR_2, OUTPUT);
  
  parar(); // Garante que o carrinho comece totalmente parado por segurança

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

  // A execução da música acontece no loop principal
  if (tocarMusica) {
    // fireRed1(BUZZER_PIN,LED);
    // delay(1000);
    // fireRedIntro(BUZZER_PIN,LED);
    // delay(500);
    Hobbit(BUZZER_PIN,LED);
    delay(1000);

    efeitoArcadeStart();
    
    tocarMusica = false; // Desliga o gatilho ao terminar a música


    delay(200);
    digitalWrite(LED, LOW);
      delay(100);
    digitalWrite(LED, HIGH);
      delay(200);
        digitalWrite(LED, LOW);
           delay(200);
        digitalWrite(LED, HIGH);

  }
  // =========================================================================
  // LEITURA DO SENSOR ULTRASSÔNICO E BUZZER
  // =========================================================================
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // CORREÇÃO: Adicionado timeout de 20000ms para o sensor não congelar a recepção do Bluetooth
  duracao = pulseIn(ECHO_PIN, HIGH, 20000); 
  distancia = duracao * 0.034 / 2;

  // Mostra os valores obtidos no Monitor Serial do Windows

  if(distancia != 0){
    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.println(" cm");

  }


  // Alerta de proximidade do obstáculo (Garante o aviso e proteção)
  if (distancia > 0 && distancia <= 5) {
    Serial.println("TA PERTO");
    tone(BUZZER_PIN, 3000); 
    
    // SISTEMA DE SEGURANÇA ANTIBATIDA:
    // Se o sensor detectar algo menor ou igual a 5cm, força o carrinho a PARAR
    parar(); 
  } else {
    noTone(BUZZER_PIN);     
  }

  delay(50); // Delay ideal para leitura contínua e manter os controles rápidos
}
