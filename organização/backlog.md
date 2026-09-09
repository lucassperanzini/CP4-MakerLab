# Backlog

**Divisão de responsabilidades:**
- **Lucas** — software (firmware) e modelagem 3D (CAD)
- **João** — montagem física do chassi e da eletrônica

*(Exceção: LED, sensor de distância e buzzer já foram ligados no ESP32 pelo próprio Lucas — mantidos com ele por ser o que realmente aconteceu.)*

| ID | Tarefa | Responsável | MoSCoW | Depende de | Status |
|---|---|---|---|---|---|
| T01 | Montar chassi de madeira com motores fixados | João | Must | — | ✅ Concluído |
| T02 | Modelar e imprimir carenagem (base + tampa) | Lucas | Must | — | ✅ Concluído |
| T03 | Cortar furos das rodas na carenagem | Lucas | Must | T02 | ✅ Concluído |
| T04 | Projetar e validar encaixe por pressão base/tampa | Lucas | Should | T02 | ✅ Concluído |
| T05 | Ligar ESP32 ao L298N (fiação de sinal) | João | Must | T01 | ⬜ A fazer |
| T06 | Ligar L298N aos motores e à bateria | João | Must | T01, T05 | ⬜ A fazer |
| T07 | Escrever firmware base (andar frente/trás/girar) | Lucas | Must | T05, T06 | ⬜ A fazer |
| T08 | Testar locomoção com a eletrônica solta (fora da carenagem) | Lucas + João | Must | T07 | ⬜ A fazer |
| T09 | Fechar tudo dentro da carenagem e testar encaixe final | João | Must | T04, T08 | ⬜ A fazer |
| T10 | Organizar fiação interna (abraçadeiras/organizador) | João | Should | T06 | ⬜ A fazer |
| T11 | Implementar controle via Bluetooth | Lucas | Should | T07 | ⬜ A fazer |
| T12 | Testes de autonomia de bateria | João | Should | T09 | ⬜ A fazer |
| T13 | Ligar LED indicador de status no ESP32 | Lucas | Could | T05 | ✅ Concluído |
| T14 | Ajuste fino de velocidade (PWM suave) | Lucas | Could | T07 | ⬜ A fazer |
| T15 | Acabamento estético da carenagem (pintura) | João | Could | T09 | ⬜ A fazer |
| T16 | Atualizar documentação final do repositório | Lucas + João | Must | T09, T11 | ⬜ A fazer |
| T17 | Ligar sensor de distância no ESP32 | Lucas | Could | T05 | ✅ Concluído |
| T18 | Ligar buzzer no ESP32 | Lucas | Could | T05 | ✅ Concluído |

**Status usados:** ⬜ A fazer · 🔶 Em andamento · ✅ Concluído


Ver o quadro visual em [Kanban](./kanban.md) e o mapeamento de dependências em [Dependências](./dependencias.md).
