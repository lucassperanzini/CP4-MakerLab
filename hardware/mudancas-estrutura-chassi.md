# Mudanças na Estrutura do Chassi

Histórico das decisões de projeto e o motivo de cada mudança.

## v0.1 — Chassi retangular original (kit)
- Kit "Chassi 2 Rodas Base de Acrílico 2WD" (Ref. CF04)
- Base de acrílico 15×21cm, motores DC 1:48, rodas, roda boba
- **Problema:** dimensões grandes demais para caber numa carenagem esférica de tamanho razoável (diagonal de ~26cm exigiria uma esfera de 30cm+)

## v0.2 — Chassi customizado em disco
- Desenhado um chassi circular de 160mm de diâmetro, com furos de fixação para motores, L298N e roda boba
- Objetivo: reduzir o footprint para caber numa esfera menor (~24cm)

## v0.3 — Carenagem esférica oca com colunas internas
- Definido formato "pokébola": duas metades ocas (não maciças), unidas na linha do equador
- Testada a opção de preencher a metade inferior com material sólido → **descartada**: peso estimado de ~9kg, incompatível com o torque dos motores DC 3-6V
- Adotado: casca oca de parede 6mm + colunas de apoio internas para sustentar o chassi
- Carenagem impressa com ~198mm de diâmetro (Bambu Lab A1)

## v0.4 — Corte plano na base + chassi em madeira
- **Mudança de abordagem:** ao invés de um disco de plástico preso por colunas na altura do equador, foi feito um corte plano na parte inferior da esfera (~36–40mm de altura a partir do polo)
- O corte gera um piso plano de ~146–150mm de diâmetro, onde um **chassi de madeira cortado a laser** é encaixado
- **Vantagem:** montagem de motores e componentes fica muito mais simples numa superfície plana do que na parede curva da esfera
- As colunas internas da v0.3 tornaram-se obsoletas e foram removidas do modelo

## v0.5 — Furos das rodas corrigidos
- Verificado que o trilho real entre rodas (140mm) não cabia dentro do buraco central do corte plano (a roda ultrapassava a borda em ~17mm)
- Solução: mantido o buraco central de acesso (para a madeira) e adicionados **2 furos individuais**, um por roda, posicionados exatamente na localização real de cada roda (70mm do centro), cortados na parede curva ao redor do buraco central
- Furos alinhados com o eixo real da roda (não com a direção radial da esfera), eliminando uma pequena interferência residual

## v0.6 — Encaixe por pressão entre as duas metades
- Substituído o encaixe original (peças soltas de dobradiça/botão) por um **lábio de encaixe por pressão** (anel de 2mm) na base, com **ranhura correspondente** na tampa (folga de 0,2mm)
- Permite abrir/fechar sem cola, para acesso à bateria e eletrônica
- A faixa do equador e as peças de botão/dobradiça originais foram fundidas permanentemente na base

## Estado atual
- Base e tampa modeladas e com encaixe validado (0 colisões verificadas)
- Chassi em madeira pronto (corte a laser)
- Próxima etapa: montagem elétrica final e fixação dos componentes na madeira
