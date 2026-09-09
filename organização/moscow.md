# Classificação MoSCoW

| Prioridade | Funcionalidade | Justificativa |
|---|---|---|
| **Must have** | Chassi montado (motores, ESP32, L298N, bateria) | Sem isso não existe carrinho |
| **Must have** | Carenagem fechando (base + tampa) | Requisito estético/estrutural definido no projeto |
| **Must have** | Locomoção básica (frente, trás, girar) via firmware | Núcleo funcional do MVP |
| **Must have** | Alimentação autônoma por bateria | Sem isso o carrinho não é portátil |
| **Should have** | Fiação organizada com abraçadeiras/organizador | Evita curto-circuito e falha mecânica, mas não impede o funcionamento básico |
| **Should have** | Encaixe da tampa testado e confiável (abrir/fechar repetidas vezes) | Importante para manutenção, mas o MVP pode rodar com a tampa presa provisoriamente |
| **Should have** | Controle remoto via Bluetooth (ESP32 já suporta nativamente) | Eleva bastante a usabilidade, mas não é bloqueante pro MVP |
| **Could have** | Indicador visual (LED) de status (ligado/bateria baixa) | Boa prática de UX, mas dispensável |
| **Could have** | Ajuste fino de velocidade via PWM (controle de aceleração suave) | Melhoria de qualidade de movimento |
| **Could have** | Pintura/acabamento estético fiel às cores da pokébola | Estética, não afeta funcionamento |
| **Won't have (por ora)** | Recarga automática / base de carregamento | Fora do escopo |
| **Won't have (por ora)** | Câmera ou sensores de visão | Fora do escopo |

Ver o cruzamento dessas prioridades com o cronograma em [Backlog](./backlog.md) e [Kanban](./kanban.md).
