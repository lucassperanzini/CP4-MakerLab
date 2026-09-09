# Dependências entre Tarefas

![Mapa de dependências](./dependencias.png)

## Caminho crítico (sequência que mais trava o projeto)

```
T01 (chassi) → T05 (ESP32↔L298N) → T06 (L298N↔motores/bateria) → T07 (firmware) → T08 (teste solto) → T09 (fechar na carenagem) → T16 (documentação final)
```

Esse é o caminho que define o prazo mínimo possível: nenhuma tarefa dele pode começar antes da anterior terminar.

## Tarefas que podem rodar em paralelo

- **T02 → T03 → T04** (modelagem/impressão da carenagem) não depende de nada da linha elétrica — pode ser feito ao mesmo tempo que T01, T05, T06
- **T11 (Bluetooth)** só depende do firmware base (T07), não precisa esperar a carenagem estar fechada (T09)
- **T13, T14, T15** (itens "Could have") não bloqueiam nada — podem ficar para o final ou ser cortados sem afetar o restante

## Risco de atraso

O ponto mais sensível é **T09** (fechar tudo dentro da carenagem), porque depende tanto da linha elétrica (T08) quanto da linha mecânica (T04) terminarem. Se uma das duas atrasar, T09 atrasa junto — e com ela, toda a documentação final (T16).
