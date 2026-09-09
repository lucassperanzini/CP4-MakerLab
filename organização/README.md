# Projeto Carrinho — Carrinho Robótico "Pokébola"

Projeto de montagem de um carrinho robótico 2WD (duas rodas motorizadas + roda boba), com carenagem esférica impressa em 3D no estilo pokébola.

## Equipe

- Lucas
- João

**Laboratório:** Project Maker Lab

## Status atual

- [x] Definição dos requisitos e componentes
- [x] Croqui do chassi
- [x] Carenagem esférica modelada e impressa (base + tampa, encaixe por pressão)
- [x] Chassi interno em madeira (corte a laser) definido
- [x] Diagramas de conexão e alimentação
- [x] MVP, MoSCoW, Backlog, Kanban e planilha de custos definidos
- [ ] Montagem eletrônica completa (ESP32 + L298N + motores)
- [ ] Upload e testes do firmware
- [ ] Testes de locomoção

## Estrutura do repositório

```
├── docs/                          Documentação geral (Parte 1)
│   ├── ficha-requisitos.md
│   ├── croqui-chassi.png
│   ├── mudancas-estrutura-chassi.md
│   ├── conexoes-eletronicas.png
│   ├── diagrama-alimentacao.png
│   └── organizacao-componentes.png
│
├── hardware/                      Parte 2
│   ├── arquitetura/                 imagens e diagramas
│   └── componentes/                 lista-componentes.md
│
├── cad/                            Parte 2
│   ├── STL/
│   │   ├── v0.1/                    primeiro chassi customizado (disco)
│   │   ├── v0.2/                    carenagem oca + colunas internas
│   │   ├── v0.3/                    corte plano + furos das rodas
│   │   └── v0.4-atual/              base + tampa, com encaixe por pressão
│   └── fonte-do-modelo/             scripts Python (trimesh) usados para gerar os STL
│
├── src/                            Parte 2
│   └── codigo.ino                   firmware ESP32 (controle dos motores via L298N)
│
└── organização/                    Planejamento do projeto
    ├── mvp.md                        definição do MVP e da conclusão
    ├── moscow.md                     classificação MoSCoW das funcionalidades
    ├── backlog.md                    tarefas, responsáveis e status
    ├── dependencias.md + .png        mapa de dependências entre tarefas
    ├── kanban.md                     quadro Kanban (snapshot)
    └── planilha-custos.xlsx          custos, lojas, links e quantidades
```

## Documentação (Parte 1)

- [Ficha de Requisitos](./docs/ficha-requisitos.md)
- [Mudanças na estrutura do chassi](./docs/mudancas-estrutura-chassi.md)
- [Conexões dos componentes eletrônicos](./docs/conexoes-eletronicas.png)
- [Diagrama de alimentação](./docs/diagrama-alimentacao.png)
- [Organização dos componentes (fixação e fiação)](./docs/organizacao-componentes.png)

## Organização do projeto

- [MVP — quando estará minimamente funcional / concluído](./organização/mvp.md)
- [Classificação MoSCoW](./organização/moscow.md)
- [Backlog de tarefas](./organização/backlog.md)
- [Mapa de dependências](./organização/dependencias.md)
- [Kanban](./organização/kanban.md)
- [Planilha de custos](./organização/planilha-custos.xlsx)

## Componentes principais

Ver [lista completa](./hardware/componentes/lista-componentes.md) e a [planilha de custos](./organização/planilha-custos.xlsx). Resumo:

- ESP32 (controlador)
- Módulo driver de motor L298N
- 2x motor DC com redução 1:48
- Rodas 70×26mm
- Roda boba
- Suporte de pilhas
- Carenagem esférica em PLA (impressa em Bambu Lab A1)
- Chassi em madeira (corte a laser)

## Modelagem 3D

Os modelos foram gerados de forma paramétrica via scripts Python (biblioteca `trimesh`), no lugar de um software de CAD tradicional (OpenSCAD/Fusion). Os scripts-fonte estão em [`cad/fonte-do-modelo`](./cad/fonte-do-modelo), e os `.stl` versionados em [`cad/STL`](./cad/STL).

## Firmware

Código-fonte em [`src/codigo.ino`](./src/codigo.ino). Pinagem documentada no cabeçalho do arquivo e no [diagrama de conexões](./docs/conexoes-eletronicas.png).

---

*Documentação em construção — próximas atualizações incluirão testes de locomoção e ajustes finais de montagem.*
