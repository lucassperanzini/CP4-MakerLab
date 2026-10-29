# CP2 — TinyML no Carrinho-Robô: Classificador de Tipo de Aproximação

> Evolução do carrinho do CP1 (BLE + sensor ultrassônico de ré + motores + buzzer). Não foi construído um carrinho novo.

**Integrantes:**
- João 
- Lucas

**Ciclo:** COLETAR → TREINAR → TESTAR → EMBARCAR → INFERIR → AGIR


## 1. Ideia do projeto

O ultrassônico (traseiro) mede a distância a 20 Hz. O modelo recebe as **últimas 10 leituras (500 ms)** e classifica **como** o obstáculo se aproxima, e não apenas quão perto ele está.

| Classe | O que é |
|---|---|
| `aproximacao_suave` | distância diminui devagar e de forma constante |
| `aproximacao_brusca` | distância cai drasticamente em poucos ms |
| `estatico` | distância não muda |

**Regras de ação** (valem só quando o carrinho está **de ré**, pois o sensor é traseiro):

| Situação | Ação |
|---|---|
| De ré, algo a mais de 20 cm (ex.: 30 cm) | não faz nada |
| De ré, algo entre 5 e 20 cm (ex.: 15 cm) e o modelo diz `suave` ou `brusca` | diminui a velocidade (PWM): `suave` gradual conforme a distância (de 100% a 45%); `brusca` direto em 45% |
| De ré, algo entre 5 e 20 cm e o modelo diz `estatico` | mantém a velocidade |
| De ré, algo a 5 cm ou menos | freio + buzzer + vai para **frente** por 1 s |
| Qualquer outra direção (frente, curvas, parado) | não faz nada, independente da distância |

## 2. Estrutura do repositório

```text
CP4-MakerLab-main/
├── README.md
├── Vídeo do WhatsApp 2026-10-28 às 23:09:41.mp4
├── croqui-chassi.png
├── hardware/
│   ├── ficha-requisitos.md
│   ├── lista-componentes.md
│   ├── mudancas-estrutura-chassi.md
│   ├── conexoes-eletronicas.png
│   └── diagrama-alimentacao.png
├── organização/
│   ├── README.md
│   ├── mvp.md
│   ├── moscow.md
│   ├── backlog.md
│   ├── dependencias.md
│   ├── kanban.md
│   └── planilha-custos.xlsx
├── src/
│   └── Carrinho/
│       ├── Carrinho.ino
│       ├── carrinho_tinyml/
│       │   └── carrinho_tinyml.ino
│       ├── fireRed.ino
│       ├── fireRedIntro.ino
│       ├── hobbits.ino
│       └── musica.ino
└── stl/
    ├── baixo.stl
    ├── cima.stl
    ├── Centro.stl
    └── Fusion.md
```

## 3. Dataset e coleta de dados 

- **Sensor:** HC-SR04 (ultrassônico), na **traseira** do carrinho (TRIG=5, ECHO=4).
- **Dado:** distância em cm (1 eixo, `distancia`) com `timestamp` em ms, a **20 Hz** (uma leitura a cada 50 ms); sem eco = 200 cm.
- **Como foi obtido: simulação.** O script `scripts/gerar_dataset_simulado.py` (semente 42) gera trajetórias de distância e aplica as características do sensor: ruído gaussiano de ~0,3 cm, resolução de 0,1 cm, saturação em 200 cm, falhas ocasionais de eco (0,4% das leituras) e jitter de 0–2 ms no timestamp. O formato é idêntico ao do `coletar_csv.py`.
- **Como cada classe foi gerada:**
  - `estatico`: distância fixa (6–150 cm), deriva lenta (±0,4 cm/s, tremor), obstáculo se afastando (3–25 cm/s) ou sem obstáculo no alcance (200 cm).
  - `aproximacao_suave`: queda constante de 3 a 12 cm/s ao longo de 10 s, parando entre 8 e 25 cm.
  - `aproximacao_brusca`: queda de 50 a 130 cm/s em 1,5 s, com perfil linear ou acelerando, parando entre 5 e 30 cm.
- **Rótulo:** vem do nome do arquivo (`<classe>.simNNN.csv`).

| Classe | Arquivos CSV | Amostras (linhas) | Tempo total |
|---|---|---|---|
| estatico | 16 | 3200 | 160 s |
| aproximacao_suave | 16 | 3200 | 160 s |
| aproximacao_brusca | 40 | 1200 | 60 s |
| **Total** | **72** | **7600** | **380 s** |

**Limitação:** um modelo treinado só com dados simulados pode errar com o sensor real, porque o ruído, o eco e a vibração dos motores reais são diferentes. Os parâmetros do firmware (`LIMIAR_CONF`, `PASSO_FATOR`, `FATOR_MIN`) permitem ajustar o comportamento. O fluxo de coleta real está pronto (`coletar_dados.ino` + `coletar_csv.py`) e, com ele, o modelo pode ser retreinado sem mudar o firmware.

## 4. Projeto no Edge Impulse 

1. **Importação:** *Data acquisition → Add data → Upload existing data*; selecionar os 72 CSVs; *Label: infer from filename*; *Category: automatically split between training and testing* (~80/20). No assistente de CSV: série temporal, coluna `timestamp` em ms, valor = `distancia`, frequência 20 Hz.
2. **Divisão dos dados:** treino ____ % / teste ____ % (preencher conforme o painel).
3. **Impulse (*Create impulse*):**
   - Time series: eixo `distancia`, **janela 500 ms**, **incremento 100 ms**, **frequência 20 Hz** (10 amostras por janela).
   - Processamento: **Raw data**.
   - Aprendizado: **Classification** (rede neural densa).
4. **Modelo:** 2 camadas densas (20 e 10 neurônios), ____ épocas, learning rate ______ , 20% de validação (preencher).
5. **Resultados do treinamento:** acurácia de validação ____ %, perda ____ . Matriz de confusão: _inserir print_.
6. **Teste do modelo:** *Model testing → Classify all*: acurácia ____ %. _inserir print_.
7. **Deployment:** *Arduino library* (Quantized int8; se a acurácia cair muito, usar float32).

Prints em `edge_impulse/`: `01_dados.png`, `02_impulse.png`, `03_treino.png`, `04_matriz_confusao.png`, `05_teste.png`.

## 5. Integração com o ESP32

1. Arduino IDE → *Sketch → Include Library → Add .ZIP Library* (zip exportado).
2. O `carrinho_tinyml.ino` já inclui a biblioteca do projeto: `#include <JPSV-project-1_inferencing.h>`.
3. Placa ESP32 (a mesma do CP1); gravar e abrir o Serial Monitor a 115200.

Fluxo: leitura a 20 Hz (sem `delay`) → janela deslizante de 10 amostras → `run_classifier()` → limiar de confiança (0,70) → ação.

Exemplo de saída serial:
```
d= 76.4 cm | est=0.05 suave=0.92 brusca=0.03 | acao=reduz_vel | vel=75%
d= 80.0 cm | est=0.03 suave=0.02 brusca=0.95 | acao=EMERGENCIA | vel=88%
```

### O que foi mantido do CP1
- O comando de movimento fica memorizado em `direcaoAtual` até chegar outro comando.
- `L`/`R` com o mapeamento invertido de propósito (`L → direita()`, `R → esquerda()`).
- O comando `B` continua sendo recusado com obstáculo a ≤ 5 cm. A diferença é que a ré agora termina em fuga para frente, e não só em parada.
- Ao desconectar o celular, o carrinho para e volta a anunciar o BLE.

### Decisões de projeto
- **Sensor na traseira:** o obstáculo está atrás do carro, então a fuga da emergência é para **frente** (`SENSOR_NA_TRASEIRA = true`).
- **Ação não bloqueante:** a emergência usa `millis()`; o `delay(50)` do loop foi trocado por amostragem em intervalo fixo, para o espaçamento entre leituras ser o mesmo do dataset.
- **Só age de ré:** fora da ré o carrinho ignora o sensor e o modelo, e a velocidade fica em 100%. Isso evita avanços causados por falsos "brusca" quando o carrinho não está dando ré.
- **Papel do modelo:** na faixa de 5 a 20 cm o modelo decide **se** há aproximação (`suave`/`brusca` contra `estatico`) e **como** reduzir (gradual ou direto no piso). A fuga a ≤ 5 cm é decidida pela distância, por segurança, e exige 2 leituras seguidas para filtrar eco falso.
- **Redução gradual:** o fator de PWM é função da distância: 100% a 20 cm e 45% a 10 cm ou menos. Ele só diminui enquanto está na faixa e volta a 100% quando o obstáculo passa de 20 cm, o carrinho deixa de dar ré ou o app manda outro comando.
- **Durante a emergência**, comandos de movimento do app são ignorados por 1 s; depois há 0,8 s de pausa na inferência.
- **Música:** o botão `P` toca a `fireRedIntro()` do CP1 (definida em outra aba do projeto). Como ela bloqueia o loop, o carrinho não aceita comandos de movimento enquanto ela toca, e a janela do modelo é zerada ao terminar.
- **Buzzer:** toca só na emergência (de ré a ≤ 5 cm). No CP1 ele tocava a ≤ 5 cm em qualquer direção.
- **Parâmetros:** `DIST_REDUZ_CM`, `DIST_PISO_CM`, `DIST_FUGA_CM`, `FATOR_MIN` e `LIMIAR_APROX` ficam no topo do `.ino`.
- **Sem eco:** a leitura vira 200 cm (no CP1 virava 0), igual ao dataset.

### Verificação da lógica
A lógica de ação do firmware foi compilada e executada no PC, com relógio simulado e um carro simulado em malha fechada (a distância depende do PWM aplicado). Testes: ré a 40, 15 e 6 cm/s contra uma parede (100% a 30 cm, cerca de 72% a 15 cm, fuga para frente a 5 cm) e obstáculo se aproximando rápido com o carrinho indo para frente, curvando ou parado (nenhuma ação). Isso valida a máquina de estados, não o modelo treinado nem o hardware.


