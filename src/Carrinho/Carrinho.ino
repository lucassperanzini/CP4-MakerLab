#include <dht_calor_inferencing.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <esp_mac.h>
#include <DHT.h>

// >>> TINYML: biblioteca exportada do Edge Impulse (Arduino library).
// Troque pelo nome do SEU projeto (o .h termina em _inferencing.h).
#include <dht_calor_inferencing.h>

// Definição dos pinos do sensor ultrassônico e LED
#define TRIG_PIN 5
#define ECHO_PIN 4
#define BUZZER_PIN 18  // Pino do Buzzer
#define LED 2          // Pino do LED azul integrado

#define DHTPIN 19      // Pino de dados do DHT11
#define DHTTYPE DHT11  // Modelo do sensor

DHT dht(DHTPIN, DHTTYPE);

// =========================================================================
// MAPEAMENTO DOS PINOS DOS MOTORES
// =========================================================================
const int MOTOR_ESQ_1 = 25;
const int MOTOR_ESQ_2 = 33;
const int MOTOR_DIR_1 = 32;
const int MOTOR_DIR_2 = 26;

long duracao;
float distancia;
bool tocarMusica = false;
bool tocarIntro = false;
bool movimentoLiberado = false;

int VEL_RETA  = 220;          // 0 a 255
int VEL_CURVA = 140;


float temperatura = 0.0;
float umidade = 0.0;

unsigned long ultimaLeituraDHT = 0;
// >>> TINYML: o modelo foi treinado com 1 amostra por segundo (limite do DHT11).
// Antes era 5000 ms; agora lê a cada 1 s e o BLE continua enviando a cada 5 leituras.
const unsigned long INTERVALO_DHT = 1000;
const int LEITURAS_POR_ENVIO_BLE = 5;
int contadorBLE = 0;

// =========================================================================
// >>> TINYML: janela deslizante + decisão do alarme
// =========================================================================
// Estes valores PRECISAM bater com o Impulse: 2 eixos (temperatura, umidade) a 1 Hz.
static_assert(EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME == 2, "O Impulse deve ter 2 eixos: temperatura e umidade (nessa ordem)");
const int EIXOS = 2;
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];   // [T0,H0,T1,H1,...]
int amostrasNaJanela = 0;
bool temLeituraValida = false;

const char *CLASSE_ALERTA = "calor";       // mesmo nome do rótulo no Edge Impulse
const float LIMIAR_CALOR   = 0.80f;        // confiança mínima para contar como "calor"
const int   CONFIRMA_ALERTA = 2;           // inferências seguidas de calor para ligar o alarme
const int   CONFIRMA_NORMAL = 3;           // inferências seguidas sem calor para desligar
int seqCalor = 0, seqNormal = 0;
bool alertaCalor = false;                  // true = buzzer apitando por causa do modelo
bool buzzerAlertaLigado = false;

// Variáveis para controle de tempo ao soltar o botão
volatile unsigned long ultimoComandoTempo = 0;
const unsigned long TIMEOUT_PARADA = 200;

#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"

BLECharacteristic *pCharacteristic;

// =========================================================================
// FUNÇÕES DE MOVIMENTAÇÃO
// =========================================================================
void motorEsq(int vel) {
  if (vel >= 0) { analogWrite(MOTOR_ESQ_1, vel); analogWrite(MOTOR_ESQ_2, 0); }
  else          { analogWrite(MOTOR_ESQ_1, 0);   analogWrite(MOTOR_ESQ_2, -vel); }
}
void motorDir(int vel) {
  if (vel >= 0) { analogWrite(MOTOR_DIR_1, vel); analogWrite(MOTOR_DIR_2, 0); }
  else          { analogWrite(MOTOR_DIR_1, 0);   analogWrite(MOTOR_DIR_2, -vel); }
}

void parar()    { motorEsq(0);          motorDir(0); }
void frente()   { motorEsq(VEL_RETA);   motorDir(VEL_RETA); }
void tras()     { motorEsq(-VEL_RETA);  motorDir(-VEL_RETA); }
void esquerda() { motorEsq(-VEL_CURVA); motorDir(VEL_CURVA); }
void direita()  { motorEsq(VEL_CURVA);  motorDir(-VEL_CURVA); }

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
// >>> TINYML: empurra uma leitura (T,H) na janela e roda o modelo
// =========================================================================
void empurrarAmostra(float t, float h) {
  // desloca a janela uma amostra (2 floats) para a esquerda e coloca a nova no fim
  memmove(features, features + EIXOS, (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - EIXOS) * sizeof(float));
  features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 2] = t;
  features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - 1] = h;
  if (amostrasNaJanela < EI_CLASSIFIER_RAW_SAMPLE_COUNT) amostrasNaJanela++;
}

void classificarCalor() {
  if (amostrasNaJanela < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
    Serial.printf("TinyML: enchendo janela (%d/%d)\n", amostrasNaJanela, EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    return;
  }

  signal_t signal;
  if (numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal) != 0) {
    Serial.println("TinyML: erro ao criar signal");
    return;
  }

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR r = run_classifier(&signal, &result, false);
  if (r != EI_IMPULSE_OK) {
    Serial.printf("TinyML: erro run_classifier (%d)\n", r);
    return;
  }

  float pCalor = 0.0f;
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (strcmp(result.classification[i].label, CLASSE_ALERTA) == 0) pCalor = result.classification[i].value;
  }

  // Histerese: evita o alarme ligar/desligar por uma inferência isolada
  if (pCalor >= LIMIAR_CALOR) { seqCalor++;  seqNormal = 0; }
  else                        { seqNormal++; seqCalor = 0; }

  if (!alertaCalor && seqCalor  >= CONFIRMA_ALERTA) alertaCalor = true;
  if ( alertaCalor && seqNormal >= CONFIRMA_NORMAL) alertaCalor = false;

  Serial.printf("TinyML: P(calor)=%.2f | alerta=%s | %d ms\n",
                pCalor, alertaCalor ? "SIM" : "nao",
                (int)(result.timing.dsp + result.timing.classification));
}

// >>> TINYML: ação no carrinho = buzzer intermitente enquanto o modelo diz "calor"
// (não bloqueia o loop; só chama tone()/noTone() quando o estado muda)
void atualizarAlarmeCalor() {
  if (alertaCalor) {
    bool fase = ((millis() / 250) % 2) == 0;     // 250 ms ligado / 250 ms desligado
    if (fase && !buzzerAlertaLigado) { tone(BUZZER_PIN, 2500); buzzerAlertaLigado = true; }
    if (!fase && buzzerAlertaLigado) { noTone(BUZZER_PIN);     buzzerAlertaLigado = false; }
  } else if (buzzerAlertaLigado) {
    noTone(BUZZER_PIN);
    buzzerAlertaLigado = false;
  }
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
                tocarIntro = true;   // só marca; toca no loop()
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

  dht.begin();

  BLEDevice::init("ESP32_iPhone");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);
pCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID,
                    BLECharacteristic::PROPERTY_READ |
                    BLECharacteristic::PROPERTY_WRITE |
                    BLECharacteristic::PROPERTY_NOTIFY
                  );

pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("ESP32 Inicializado com sucesso!");
  Serial.println("TinyML: aguardando 10 s de leituras para a primeira inferência...");
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


if (millis() - ultimaLeituraDHT >= INTERVALO_DHT) {

    ultimaLeituraDHT = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      umidade = h;
      temperatura = t;
      temLeituraValida = true;

      Serial.print("Temp: ");
      Serial.print(temperatura, 1);
      Serial.print(" °C | Umidade: ");
      Serial.print(umidade, 0);
      Serial.println(" %");

      // Envia via BLE com \n no final (a cada LEITURAS_POR_ENVIO_BLE leituras = 5 s, como antes)
      if (++contadorBLE >= LEITURAS_POR_ENVIO_BLE) {
        contadorBLE = 0;
        String dadosSensor = "T:" + String(temperatura, 1) + "C H:" + String(umidade, 0) + "%\n";
        pCharacteristic->setValue(dadosSensor.c_str());
        pCharacteristic->notify();
      }
    } else {
      Serial.println("Falha ao ler o sensor DHT11!");
    }

    // >>> TINYML: se a leitura falhou, repete a última válida para não quebrar o ritmo de 1 Hz da janela
    if (temLeituraValida) {
      empurrarAmostra(temperatura, umidade);
      classificarCalor();
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
    frente();
    delay(1000);
    parar();
  } else if (!alertaCalor) {
    // >>> TINYML: só silencia o buzzer aqui se o alarme de calor NÃO estiver ativo
    // (antes o noTone() rodava a cada loop e cortaria o apito do modelo)
    noTone(BUZZER_PIN);
  }

  // >>> TINYML: ação do modelo -> buzzer intermitente enquanto houver calor detectado
  atualizarAlarmeCalor();

  delay(50);
}
