# Game Design: regras de negócio e mecânicas

Documento vivo com as regras do jogo. Itens marcados como **TBD** ainda não foram decididos. Não implemente uma regra TBD sem antes defini-la aqui.

## Visão geral
- Gênero: estratégia com construção de base e elementos de tower defense.
- Perspectiva: isométrica 2D.
- Modo atual: **single player sandbox**. Não há objetivo obrigatório: o jogador constrói e experimenta livremente.
- Modos futuros (fora do escopo por enquanto):
  - Multiplayer PvE (cooperativo).
  - Multiplayer PvP.

## Fluxo de cenas
1. **Menu principal**: opções `Iniciar` (começa uma partida), `Configurações` e `Sair`. Navegação por teclado (cima/baixo + confirmar) ou mouse (hover + clique).
2. **Configurações**: permite escolher uma resolução entre 1024x576, 1280x720, 1600x900 e 1920x1080, alternar entre janela e tela cheia e editar o nome do jogador. A resolução e o modo de tela só são aplicados ao confirmar `Aplicar`; `Voltar` ou `ESC` retorna ao menu. Se o nome estiver vazio, o jogador usa `Jogador`.
3. **Partida**: o mundo é criado vazio ao iniciar. `ESC` cancela a construção selecionada ou limpa a seleção de tropas/construção; sem nada selecionado, volta ao menu e descarta a partida atual.

## Jogador
- Cada partida cria a entidade ECS de identidade do jogador local, com o `PlayerId` gerado pelo Model, o nome configurado e o componente de dono. Ela ainda não tem posição, vida, combate nem representação visual.
- O avatar só spawna quando o jogador coloca o Centro de Comando: nesse momento recebe `Position`, vida e atributos de combate e passa a ser representado no mundo.
- O avatar tem 10 de vida, causa 1 de dano, tem alcance de 1 tile e velocidade de 3,5 tiles/s (a velocidade do Bode).
- WASD controla o avatar nas quatro direções da tela isométrica. O movimento é contínuo em coordenadas lógicas; diagonais mantêm a mesma velocidade.
- O ponto inicial é a casa livre mais próxima, fora do Centro de Comando colocado pelo jogador. Ao chegar a 0 de vida, o avatar reaparece nessa casa com a vida cheia.
- O avatar colide com os footprints das construções, tem raio de colisão de 0,3 tile e desliza pelas paredes ao mover-se na diagonal.
- A câmera acompanha o avatar; o zoom permanece fixo.
- No sandbox atual há somente o jogador local. O `PlayerId` é propagado aos Controllers e Views pelo event bus, permitindo que novos jogadores sejam adicionados depois.

## Modo debug
- `F3` liga e desliga o modo debug em qualquer cena. Ele só muda o que aparece na tela e não afeta as regras do jogo.
- Painel no canto superior esquerdo com FPS e tempo do frame (verde ≥ 55, amarelo ≥ 30, vermelho abaixo disso) e a cena atual. Durante a partida mostra também a quantidade de construções e tropas, a posição e o zoom da câmera, e o tile sob o mouse.
- Em cada entidade da partida aparecem:
  - o contorno da área ocupada no chão (ciano);
  - para tropas, a caixa de clique usada na seleção (amarelo);
  - um ponto na posição (magenta) e o rótulo `#id Nome (x, y)`. Para tropas a posição é contínua. Para construções aparecem a origem e o tamanho.

## Mapa
- Grid isométrico. Toda posição de jogo é uma coordenada de tile (lógica), independente da projeção na tela.
- Tile de 64x32 px na tela (zoom 1).
- O sandbox segue com mapa **infinito** e sem terreno. O Model também oferece `BoundedMap` para missões finitas: retângulo definido por origem lógica, largura e altura; tiles dentro dele começam como planície transitável, tiles fora são bloqueados e o terreno pode ser marcado como bloqueado. A classe não depende de raylib.
- Câmera: acompanha o avatar do jogador com zoom fixo de 1,0x.
- Renderização de construções, tropas e avatar usa o algoritmo do pintor: objetos mais distantes no eixo de profundidade do grid são desenhados primeiro.
- Origem do mapa definitivo (Tiled `.tmx` ou procedural), tamanho e tipos de terreno: **TBD**.

## Economia / recursos
- O inventário é composto por slots; cada slot armazena um item e sua quantidade atual em unidades inteiras (não há frações de item). Cada pilha comporta até 100 unidades. O inventário do jogador tem 32 slots e o de cada construção tem 64.
- O jogador começa com 500 unidades de ferro, distribuídas em cinco pilhas. Inventários de construções começam vazios. Itens definidos: ferro, carvão, aço, cobre, eletrônicos e ferramentas.
- A tecla `I` abre/fecha o inventário do jogador. Ao selecionar uma construção, o painel mostra o inventário do jogador e o da construção lado a lado. Arrastar uma pilha de um painel ao outro transfere o máximo que couber; o restante permanece na origem se não houver espaço. `ESC` ou `X` fecha o painel.
- Produção usa taxas por segundo, mas itens e consumo são sempre unidades inteiras. O progresso fracionário da taxa fica acumulado internamente até completar uma unidade; produção também respeita os itens disponíveis e a capacidade do inventário de saída:
  - Mina de Ferro: produz 1 ferro/s sem consumir entradas.
  - Mina de Carvão: produz 1 carvão/s sem consumir entradas.
  - Siderurgia: consome 1 ferro/s e 1 carvão/s e produz 0,5 aço/s.
  - Mina de Cobre: produz 1 cobre/s sem consumir entradas.
  - Fábrica de Eletrônicos: consome 1 ferro/s e 1 cobre/s e produz 0,5 eletrônico/s.
  - Fábrica de Ferramentas: consome 1 aço/s e 1 eletrônico/s e produz 0,5 ferramenta/s.
- Custos de construção e recursos, estoque/capacidade antigos e limite populacional foram removidos. Construções são grátis nesta etapa. O recrutamento continua instantâneo, requer Quartel e não tem custo nem limite populacional.
- Casa, Gerador e Depósito permanecem como estruturas sem efeitos econômicos nesta etapa. Unidades ainda não recebem inventários.

## Construção
- Barra de construção no rodapé da tela: clique (ou teclas `1`-`9`) seleciona a estrutura. Clicar de novo na mesma desmarca.
- Com uma estrutura selecionada, a prévia segue o mouse (verde = válido, vermelho = inválido). O tile sob o mouse é o centro da área ocupada.
- Botão esquerdo constrói. Segurar e arrastar continua construindo a cada tile novo (útil para muros).
- Botão direito ou `ESC` cancela a seleção.
- Estruturas ocupam uma área quadrada de tiles e não podem se sobrepor.
- Estruturas atuais:

| Estrutura | Área | Regra |
|---|---|---|
| Centro de Comando | 5x5 | Um por jogador. Precisa ser a primeira construção. Depois de construído, some da seleção. |
| Muro | 1x1 | Só liberado depois que o jogador tem um Centro de Comando. |
| Casa | 2x2 | Requer Centro de Comando; sem efeito econômico nesta etapa. |
| Gerador | 2x2 | Requer Centro de Comando; sem efeito econômico nesta etapa. |
| Depósito | 2x2 | Requer Centro de Comando; sem efeito econômico nesta etapa. |
| Quartel | 3x3 | Requer Centro de Comando; libera recrutamento. |
| Cruzador | 3x3 | Requer Centro de Comando; recebe duas tropas para gerar uma descendente. |
| Mina de Ferro | 2x2 | Requer Centro de Comando; inventário de 64 slots; produz 1 ferro/s. |
| Mina de Carvão | 2x2 | Requer Centro de Comando; inventário de 64 slots; produz 1 carvão/s. |
| Siderurgia | 3x3 | Requer Centro de Comando; inventário de 64 slots; consome 1 ferro/s e 1 carvão/s e produz 0,5 aço/s. |
| Mina de Cobre | 2x2 | Requer Centro de Comando; inventário de 64 slots; produz 1 cobre/s. |
| Fábrica de Eletrônicos | 3x3 | Requer Centro de Comando; inventário de 64 slots; consome 1 ferro/s e 1 cobre/s e produz 0,5 eletrônico/s. |
| Fábrica de Ferramentas | 3x3 | Requer Centro de Comando; inventário de 64 slots; consome 1 aço/s e 1 eletrônico/s e produz 0,5 ferramenta/s. |

- Todas as construções são grátis por enquanto. Clicar numa construção abre seu inventário para consulta e transferência ao jogador.
- **TBD**: tempo de construção, demolição/reembolso, terreno válido.

## Defesa (tower defense)
- Torre defensiva: construção 2x2, tem 500 de vida, alcance de 6 tiles e causa 20 de dano a cada segundo.
- A torre ataca automaticamente a unidade ou avatar inimigo mais próximo já dentro do alcance. Torres e unidades podem receber ordens de ataque por comando do Model; alvos aliados, mortos ou sem vida são inválidos. Ataques fora de alcance não causam dano.
- Ataques reduzem vida; ao chegar a zero, unidades ou construções são removidas do mundo e liberam a área ocupada.
- Ondas e comportamento/pathfinding de inimigos ainda não fazem parte desta fatia; os alvos inimigos são entidades de jogador distintas criadas pelo jogo ou pelos testes.

## Unidades
- Tropas móveis controladas pelo jogador (estilo RTS). Cada tropa tem dono.
- **Recrutamento**: clicar no próprio Centro de Comando (sem construção selecionada) seleciona o Centro e abre o painel de recrutamento no canto superior direito. O painel fecha com o `X`, `ESC` ou ao selecionar outra coisa / clicar no chão vazio.
  - No MVP sandbox, o recrutamento exige ao menos um Quartel e é instantâneo: a tropa aparece no tile livre mais próximo do Centro. Não há custo nem limite populacional nesta etapa.
- **Seleção**: clique esquerdo numa tropa seleciona só ela. Arrastar com o botão esquerdo desenha uma caixa e seleciona todas as tropas do jogador dentro dela. `Shift` soma à seleção atual. Clique no chão vazio (sem `Shift`) limpa a seleção. Selecionar uma construção tira as tropas da seleção e vice-versa.
- **Ordens**: botão direito sobre uma unidade inimiga ordena ataque às tropas selecionadas; no chão, manda as tropas para o tile clicado.
  - Formação: cada tropa recebe um tile livre diferente, os mais próximos do destino. A tropa mais próxima de cada tile fica com ele.
  - Caminho por A* em 8 direções, desviando de construções e sem cortar quinas. Tropas não colidem entre si durante o trajeto, mas paradas nunca dividem tile.
  - Se uma construção for colocada no caminho, a tropa recalcula a rota. Se o destino não for alcançável, ela para no tile livre mais próximo de onde está.
- Não é possível construir sobre um tile com tropa (parada, andando ou com o tile reservado como destino).
- Na formação, as tropas maiores escolhem o lugar primeiro. Depois, as mais próximas do destino.
- `ESC` com tropas ou construção selecionadas limpa a seleção; sem seleção, volta ao menu.
- Tipos atuais (velocidade em tiles/s, alcance em tiles):

| Tropa | Vida | Dano | Alcance | Velocidade |
|---|---|---|---|---|
| Soldado | 100 | 10 | 1 | 2.0 |
| Morcego | 60 | 8 | 5 | 1.6 |
| Bode | 140 | 14 | 1 | 3.5 |

- Unidades mantêm vida, dano e alcance. Uma ordem de ataque dispara após um segundo de cooldown e repete a cada segundo enquanto o alvo permanecer dentro do alcance.

### Cruzador e genealogia de tropas
- O jogador seleciona exatamente duas tropas próprias e clica com o botão direito sobre um Cruzador próprio. As duas caminham até a construção; quando ambas chegam, são consumidas e a descendente aparece no espaço livre mais próximo.
- Se não houver espaço para a descendente, as tropas aguardam junto ao Cruzador até existir um tile livre. Uma nova ordem de movimento cancela a entrega.
- As combinações e descendentes são registradas em [`UNIT_GENEALOGY.md`](UNIT_GENEALOGY.md). Combinações ainda não cadastradas usam provisoriamente o resultado legado `Tropa Combinada`, com vida e dano somados, maior alcance, velocidade média ponderada pela quantidade de unidades representadas e tamanho calculado pelo poder.
- O jogador recebe a descendente selecionada. A quantidade de unidades representada pelas tropas de origem é mantida.
- **TBD**: receitas e tipos descendentes, se a ordem dos pais importa, atributos próprios das descendentes e custo/tempo de cruzamento fora do sandbox.
- Tropas maiores que 1x1 ocupam e reservam o quadrado inteiro: nada é construído em cima e o caminho só passa por onde o quadrado inteiro cabe.
- **TBD**: custo e tempo de recrutamento fora do sandbox, limite de tropas, colisão entre tropas.

## Vitória / derrota
- Sandbox: sem condição de vitória ou derrota.
- Missões têm um objetivo configurável por progresso inteiro; ao atingir o alvo, ficam em estado `Won`. Falha explícita enquanto ativas leva a `Lost`. Estados terminais não mudam mais e somente missões vencidas pagam recompensa.
- Recompensas MVP são XP, creditados uma única vez ao jogador associado ao resgate.

## Progressão e tecnologia
- Cada jogador de progressão começa no nível 1 com 0 XP. O custo para passar do nível N para N+1 é `100 × N` XP (acumulado: nível 2 em 100 XP, nível 3 em 300 XP).
- Tecnologias são definidas por ID, nível mínimo e IDs de pré-requisitos. Só podem ser desbloqueadas quando o jogador existe, atende ao nível mínimo e já desbloqueou todos os pré-requisitos. O desbloqueio é permanente durante a vida do estado de progressão.
- A integração de tecnologias com construções/unidades e persistência entre partidas é **TBD**.

## Multiplayer (planejado)
- Rede via ENet.
- Regras de PvE/PvP, sincronização e autoridade (servidor/cliente): **TBD**.
- Desde já, as entidades devem guardar o jogador dono para não impedir o multiplayer depois.
