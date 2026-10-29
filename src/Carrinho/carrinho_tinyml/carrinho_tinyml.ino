    // =====================================================================
// CP2 - TinyML | Carrinho com classificador de tipo de aproximação
// Evolução do código do CP1 (BLE + sensor de ré + motores + buzzer).
//
// Pipeline:  Ultrassônico (ré) -> janela de 10 leituras (500 ms @ 20 Hz)
//            -> Edge Impulse (run_classifier) -> classe -> ação no carrinho
//
// Classes do modelo (nomes IGUAIS aos labels no Edge Impulse):
//   estatico            -> mantém o comportamento atual
//   aproximacao_suave   -> reduz a velocidade gradativamente (PWM)
//   aproximacao_brusca  -> freio de emergência + buzzer + fuga por 1 s
//
// Mantido do CP1: comando de movimento fica memorizado até chegar outro
// comando; L/R invertidos de propósito; ré é interrompida a <= 5 cm;
// ao desconectar o celular o carrinho para.
// =====================================================================
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_mac.h>

// >>> TROQUE pelo nome da biblioteca exportada do Edge Impulse:
//     o header se chama  <nome-do-projeto>_inferencing.h
#include <JPSV-project-1_inferencing.h>

// ---------------------------------------------------------------------
// Pinos
// ---------------------------------------------------------------------
#define TRIG_PIN 5
#define ECHO_PIN 4
#define BUZZER_PIN 18
#define LED 2

const int MOTOR_ESQ_1 = 25;
const int MOTOR_ESQ_2 = 33;
const int MOTOR_DIR_1 = 32;
const int MOTOR_DIR_2 = 26;

// ---------------------------------------------------------------------
// Parâmetros ajustáveis
// ---------------------------------------------------------------------
int VEL_RETA  = 220;                       // 0 a 255
int VEL_CURVA = 140;

// Amostragem — IGUAL à usada na coleta/simulação do dataset
const unsigned long INTERVALO_MS = 50;     // 20 Hz
const float DIST_MAX_CM   = 200.0;         // sem eco / longe demais

// Zonas de distância (só valem de ré)
const float DIST_REDUZ_CM = 20.0;          // abaixo disso o modelo pode reduzir a velocidade
const float DIST_PISO_CM  = 10.0;          // a partir daqui a redução chega ao piso
const float DIST_FUGA_CM  = 5.0;           // <= isso: freia + buzzer + vai para frente
const uint8_t CONFIRMA_FUGA_N = 2;         // leituras seguidas <= DIST_FUGA_CM (filtra eco falso)

// Redução de velocidade
const float FATOR_MIN     = 0.45;          // piso do PWM (motor não "morre" abaixo disso)
const float LIMIAR_APROX  = 0.50;          // P(suave)+P(brusca) mínima p/ "está se aproximando"

// Emergência (fuga)
const unsigned long FREIO_MS      = 150;   // freio parado antes de fugir
const unsigned long EMERGENCIA_MS = 1000;  // duração total (freio + fuga)
const unsigned long COOLDOWN_MS   = 800;   // pausa de inferência depois da emergência

// O sensor fica na TRASEIRA: obstáculo detectado = atrás do carro, então
// fugir dele é ir para FRENTE. (Se o sensor estivesse na frente: false.)
const bool SENSOR_NA_TRASEIRA = true;

// ---------------------------------------------------------------------
// BLE
// ---------------------------------------------------------------------
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
BLECharacteristic *pCharacteristic;

// ---------------------------------------------------------------------
// Estado (compartilhado entre callbacks BLE e loop() -> volatile)
// ---------------------------------------------------------------------
float distancia = DIST_MAX_CM;

volatile bool tocarMusica = false;
volatile bool tocarIntro = false;
volatile bool movimentoLiberado = false;
volatile bool obstaculoRe = false;         // true quando o sensor detecta algo a <= DIST_FUGA_CM
volatile char direcaoAtual = 'P';          // 'F', 'B', 'L', 'R' ou 'P' (parado)
volatile bool emergenciaAtiva = false;

float fatorVel = 1.0;                      // 1.0 = velocidade cheia
uint8_t contPerto = 0;                     // leituras seguidas <= DIST_FUGA_CM

unsigned long emergenciaInicio = 0;
bool faseFugaIniciada = false;
unsigned long bloqueioAte = 0;

// Janela deslizante com as últimas N leituras (N = 10: impulse 500 ms @ 20 Hz)
static float janela[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static size_t amostrasNaJanela = 0;
unsigned long ultimaAmostra = 0;

// ---------------------------------------------------------------------
// Movimentação (velocidade escalada por fatorVel)
// ---------------------------------------------------------------------
void motorEsq(int vel) {
  if (vel >= 0) { analogWrite(MOTOR_ESQ_1, vel); analogWrite(MOTOR_ESQ_2, 0); }
  else          { analogWrite(MOTOR_ESQ_1, 0);   analogWrite(MOTOR_ESQ_2, -vel); }
}
void motorDir(int vel) {
  if (vel >= 0) { analogWrite(MOTOR_DIR_1, vel); analogWrite(MOTOR_DIR_2, 0); }
  else          { analogWrite(MOTOR_DIR_1, 0);   analogWrite(MOTOR_DIR_2, -vel); }
}

int vReta()  { return (int)(VEL_RETA  * fatorVel); }
int vCurva() { return (int)(VEL_CURVA * fatorVel); }

void parar()    { motorEsq(0);          motorDir(0); }
void frente()   { motorEsq(vReta());    motorDir(vReta()); }
void tras()     { motorEsq(-vReta());   motorDir(-vReta()); }
void esquerda() { motorEsq(-vCurva());  motorDir(vCurva()); }
void direita()  { motorEsq(vCurva());   motorDir(-vCurva()); }

// Reaplica a direção atual com o fatorVel vigente (usado pelo "suave").
// Mantém o mapeamento do CP1: 'L' -> direita(), 'R' -> esquerda().
void aplicarMovimento() {
  switch (direcaoAtual) {
    case 'F': frente();   break;
    case 'B': if (obstaculoRe) { parar(); direcaoAtual = 'P'; } else tras(); break;
    case 'L': direita();  break;
    case 'R': esquerda(); break;
    default:  parar();    break;
  }
}

// Fuga na velocidade cheia, para longe do obstáculo
void fugir() {
  if (SENSOR_NA_TRASEIRA) { motorEsq(VEL_RETA);  motorDir(VEL_RETA); }    // obstáculo atrás -> frente
  else                    { motorEsq(-VEL_RETA); motorDir(-VEL_RETA); }   // obstáculo na frente -> ré
}

bool podeMover() { return movimentoLiberado && !emergenciaAtiva; }

void efeitoArcadeStart() {
  tone(BUZZER_PIN, 1319, 150); delay(150);
  tone(BUZZER_PIN, 1568, 150); delay(150);
  tone(BUZZER_PIN, 2093, 150); delay(150);
  tone(BUZZER_PIN, 2637, 300); delay(300);
  noTone(BUZZER_PIN);
}

void reiniciarJanela() { amostrasNaJanela = 0; }

// ---------------------------------------------------------------------
// SEGURANÇA: se o celular desconectar, o carrinho para e volta a anunciar
// ---------------------------------------------------------------------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    Serial.println("Celular conectado");
  }
  void onDisconnect(BLEServer *pServer) {
    Serial.println("Celular desconectado - parando");
    parar();
    direcaoAtual = 'P';
    movimentoLiberado = false;
    emergenciaAtiva = false;
    fatorVel = 1.0;
    noTone(BUZZER_PIN);
    BLEDevice::startAdvertising();
  }
};

// ---------------------------------------------------------------------
// COMANDOS DO CELULAR
// Cada comando de movimento fica memorizado em direcaoAtual e o carrinho
// continua andando até chegar outro comando (ou o sensor de ré parar).
// Durante a emergência do modelo, comandos de movimento são ignorados.
// ---------------------------------------------------------------------
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    char comando = value[0];
    Serial.print("Comando recebido: ");
    Serial.println(comando);

    switch (comando) {
      case 'F': case 'f':
        if (podeMover()) { fatorVel = 1.0; direcaoAtual = 'F'; frente(); }
        break;

      case 'B': case 'b':
        if (podeMover()) {
          if (obstaculoRe) {            // não deixa dar ré com obstáculo colado
            direcaoAtual = 'P';
            parar();
          } else {
            direcaoAtual = 'B';
            tras();
          }
        }
        break;

      case 'L': case 'l':
        if (podeMover()) { fatorVel = 1.0; direcaoAtual = 'L'; direita(); }
        break;

      case 'R': case 'r':
        if (podeMover()) { fatorVel = 1.0; direcaoAtual = 'R'; esquerda(); }
        break;

      case 'P': case 'p':
        if (!emergenciaAtiva) {
          fatorVel = 1.0;
          direcaoAtual = 'P';
          parar();
          if (movimentoLiberado) tocarIntro = true;  // só marca; trata no loop()
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
        emergenciaAtiva = false;
        direcaoAtual = 'P';
        fatorVel = 1.0;
        noTone(BUZZER_PIN);
        parar();
        break;

      default:
        // Caracteres desconhecidos (ex.: "botão solto" do app) são ignorados,
        // para não interromper o movimento.
        break;
    }
  }
};

// ---------------------------------------------------------------------
// Sensor (mesma leitura/saturação usadas no dataset)
// ---------------------------------------------------------------------
float lerDistanciaCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 20000);   // timeout 20 ms
  if (duracao == 0) return DIST_MAX_CM;            // sem eco = longe
  float d = duracao * 0.034f / 2.0f;
  if (d > DIST_MAX_CM) d = DIST_MAX_CM;
  return d;
}

void empurrarAmostra(float d) {
  const size_t N = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
  for (size_t i = 0; i < N - 1; i++) janela[i] = janela[i + 1];
  janela[N - 1] = d;
  if (amostrasNaJanela < N) amostrasNaJanela++;
}

// ---------------------------------------------------------------------
// Emergência (não bloqueante: usa millis, o sensor continua amostrando)
// ---------------------------------------------------------------------
void iniciarEmergencia(const char *motivo) {
  emergenciaAtiva = true;
  emergenciaInicio = millis();
  faseFugaIniciada = false;
  direcaoAtual = 'P';
  parar();                       // 1) freio imediato
  tone(BUZZER_PIN, 4000);        // 2) buzzer
  Serial.printf("[ACAO] EMERGENCIA: %s -> freio + buzzer + frente\n", motivo);
}

void gerenciarEmergencia() {
  if (!emergenciaAtiva) return;
  unsigned long dt = millis() - emergenciaInicio;

  if (dt >= FREIO_MS && !faseFugaIniciada) {   // 3) foge do obstáculo
    fugir();
    faseFugaIniciada = true;
  }
  if (dt >= EMERGENCIA_MS) {                   // 4) fim: para e rearma
    parar();
    noTone(BUZZER_PIN);
    emergenciaAtiva = false;
    direcaoAtual = 'P';
    fatorVel = 1.0;
    reiniciarJanela();                         // descarta leituras da fuga
    bloqueioAte = millis() + COOLDOWN_MS;
  }
}

// ---------------------------------------------------------------------
// Inferência + ação
// ---------------------------------------------------------------------
void inferirEAgir() {
  signal_t signal;
  if (numpy::signal_from_buffer(janela, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal) != 0) {
    Serial.println("Erro ao montar o signal");
    return;
  }

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  if (err != EI_IMPULSE_OK) {
    Serial.printf("Erro run_classifier: %d\n", (int)err);
    return;
  }

  float pEst = 0, pSua = 0, pBru = 0;
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    const char *l = result.classification[ix].label;
    float v = result.classification[ix].value;
    if      (!strcmp(l, "estatico"))           pEst = v;
    else if (!strcmp(l, "aproximacao_suave"))  pSua = v;
    else if (!strcmp(l, "aproximacao_brusca")) pBru = v;
  }

  // O modelo diz que algo está se aproximando (suave OU brusca)?
  bool aproximando = (pSua + pBru) >= LIMIAR_APROX;
  const char *acao;

  if (!movimentoLiberado || direcaoAtual != 'B') {
    acao = "ignora(nao_esta_de_re)";             // regra 4: fora da ré, nada
  } else if (distancia > DIST_REDUZ_CM) {
    acao = "ignora(longe)";                      // regra 1: > 20 cm, nada
  } else if (distancia <= DIST_FUGA_CM) {
    acao = "perto";                              // regra 3: a fuga é decidida no loop()
  } else if (aproximando) {
    // Regra 2: entre 5 e 20 cm e o modelo confirma aproximação -> reduz
    // suave: gradual, conforme a distância; brusca: direto no piso.
    float t = (distancia - DIST_PISO_CM) / (DIST_REDUZ_CM - DIST_PISO_CM);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float alvo = FATOR_MIN + (1.0f - FATOR_MIN) * t;
    if (pBru > pSua) alvo = FATOR_MIN;
    if (alvo < fatorVel) {                       // só diminui (nunca acelera aqui)
      fatorVel = alvo;
      aplicarMovimento();
    }
    acao = (pBru > pSua) ? "reduz_vel(brusca)" : "reduz_vel(suave)";
  } else {
    acao = "mantem";                             // modelo diz "estatico"
  }

  Serial.printf("d=%5.1f cm | est=%.2f suave=%.2f brusca=%.2f | acao=%s | vel=%d%%\n",
                distancia, pEst, pSua, pBru, acao, (int)(fatorVel * 100));
}

// ---------------------------------------------------------------------
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
  pServer->setCallbacks(new MyServerCallbacks());

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
  Serial.printf("TinyML: janela=%d amostras, intervalo do modelo=%d ms, classes=%d\n",
                (int)EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
                (int)EI_CLASSIFIER_INTERVAL_MS,
                (int)EI_CLASSIFIER_LABEL_COUNT);
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

    reiniciarJanela();            // o bloqueio acima deixou a janela velha
  }

  if (tocarIntro) {
    tocarIntro = false;
  }

  gerenciarEmergencia();

  // Amostragem em intervalo fixo (20 Hz) — alimenta a janela do modelo.
  // Sem delay(50): o espaçamento entre leituras tem que ser o do treino.
  unsigned long agora = millis();
  if (agora - ultimaAmostra >= INTERVALO_MS) {
    ultimaAmostra = agora;

    distancia = lerDistanciaCm();
    empurrarAmostra(distancia);

    // ---- Regras do sensor de ré (só valem DANDO RÉ) ----
    bool perto = (distancia > 0 && distancia <= DIST_FUGA_CM);
    obstaculoRe = perto;                         // continua recusando 'B' com obstáculo colado
    if (perto) { if (contPerto < 255) contPerto++; } else { contPerto = 0; }

    if (!emergenciaAtiva) {
      if (direcaoAtual != 'B') {
        // Fora da ré: nenhuma ação do sensor/modelo; velocidade normal
        if (fatorVel < 1.0) { fatorVel = 1.0; aplicarMovimento(); }
      } else if (movimentoLiberado) {
        if (contPerto >= CONFIRMA_FUGA_N) {
          // Regra 3: de ré a <= 5 cm -> freio + buzzer + vai para frente
          iniciarEmergencia("re a <= 5 cm");
        } else if (distancia > DIST_REDUZ_CM && fatorVel < 1.0) {
          // Regra 1: longe de novo -> velocidade normal
          fatorVel = 1.0;
          aplicarMovimento();
        }
      }
    }

    // ---- TinyML: inferência quando a janela está cheia ----
    if (amostrasNaJanela >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE &&
        !emergenciaAtiva && agora >= bloqueioAte) {
      inferirEAgir();
    }
  }
}