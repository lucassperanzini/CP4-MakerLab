# Ficha de Requisitos — Carrinho Robótico "Pokébola"

**Projeto:** Carrinho
**Equipe:** Lucas, João
**Turma** 4ESPY
**Disciplina:** Project Maker Lab

---

## 1. Dimensões gerais

| Item | Valor |
|---|---|
| Diâmetro externo da carenagem (esfera) | ~190,5 mm |
| Espessura da parede da carenagem | 6 mm |
| Material da carenagem | PLA (impressão 3D) |
| Altura do corte inferior (para encaixe do chassi) | ~36–40 mm |
| Diâmetro do piso resultante do corte | ~146–150 mm |
| Trilho entre rodas (centro a centro) | 140 mm |
| Diâmetro das rodas | 70 mm |
| Largura das rodas | 26 mm |
| Material do chassi interno | Madeira (corte a laser) |

## 2. Motores

| Item | Especificação |
|---|---|
| Quantidade | 2 |
| Tipo | Motor DC com caixa de redução 1:48 |
| Tensão | 3–6 V |
| Tração | Diferencial (2 rodas motorizadas + 1 roda boba de apoio) |

## 3. Placa controladora e eletrônica

| Componente | Função |
|---|---|
| ESP32 | Controlador principal (lógica, comunicação) |
| Módulo driver L298N | Aciona os motores (ponte H), recebe sinal do ESP32 e energia da bateria |
| Suporte de pilhas | Alimentação do sistema |
| Sensor de Distância | 

## 4. Posição dos componentes

Todos os componentes ficam montados sobre o chassi de madeira, encaixado no piso resultante do corte inferior da carenagem (ver croqui):

- **ESP32**: posição central-frontal do chassi
- **Módulo L298N**: posição central-traseira do chassi
- **Suporte de pilhas**: parte frontal, acima do ESP32
- **Motores DC**: um de cada lado, afastados 70 mm do centro, alinhados aos furos das rodas na carenagem
- **Roda boba**: parte traseira, para equilíbrio do terceiro ponto de apoio

*(posições sujeitas a ajuste fino conforme o layout real da madeira já cortada)*

## 5. Carenagem (cobertura)

- Formato esférico, estilo "pokébola", impressa em duas metades (base + tampa)
- **Base** (metade fixa): contém o corte inferior para o chassi, os furos das rodas e o lábio de encaixe
- **Tampa** (metade removível): contém a ranhura correspondente para o encaixe
- **Método de fechamento**: encaixe por pressão (lábio/ranhura), sem uso de cola — permite abrir para acesso à bateria e eletrônica
- Impressa em impressora Bambu Lab A1 (mesa útil 256×256×256 mm)

## 6. Croqui do chassi

Ver arquivo [`croqui-chassi.png`](./croqui-chassi.png) — vista de cima com posicionamento e cotas.

