/*
 * CP2 - Coleta de dados do DHT11 (temperatura + umidade) no ESP32
 *
 * Envia pela Serial, a 1 Hz (limite do DHT11), linhas no formato:
 *     timestamp_ms,temperatura_C,umidade_%
 * O script gravar_csv_dht.py le essas linhas e salva os CSVs rotulados.
 *
 * Mesmo pino do carrinho: DHT11 no GPIO 19.
 * Use este sketch SOMENTE para coletar. Para o carrinho, use carrinho_dht_tinyml.ino.
 */
#include <DHT.h>

#define DHTPIN  19
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const unsigned long INTERVALO_MS = 1000;   // DHT11 nao le mais rapido que 1 Hz
unsigned long ultimaLeitura = 0;
float ultimaT = NAN, ultimaH = NAN;

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  unsigned long agora = millis();
  if (agora - ultimaLeitura >= INTERVALO_MS) {
    ultimaLeitura = agora;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { ultimaT = t; ultimaH = h; }  // leitura falhou -> repete a anterior
    if (isnan(ultimaT)) return;                                // ainda sem nenhuma leitura valida

    Serial.print(agora);
    Serial.print(",");
    Serial.print(ultimaT, 1);
    Serial.print(",");
    Serial.println(ultimaH, 1);
  }
}
