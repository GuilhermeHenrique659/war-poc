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

## Tempo
- **1 tick = 1 segundo** de gameplay. O relógio só avança na cena `Partida`; menus não contam tempo.
- **1 dia = 600 ticks** (10 minutos reais). O contador do dia vai de `00:00` a `09:59` e então vira o dia seguinte em `00:00`.
- **1 semana = 7 dias**. **1 mês = 4 semanas = 28 dias**. Não há ano nesta etapa; depois do dia 28 o calendário volta ao dia 1 da semana 1 do mês seguinte.
- Cada partida começa em **Mês 1, Semana 1, Dia 1, 00:00**. O relógio reinicia ao iniciar uma nova partida e ao descartar a partida atual.
- O canto superior direito mostra a data (`Mês X · Semana Y`), o tempo decorrido do dia (`Dia Z · MM:SS`), o crédito e os juros da semana. O painel de vida do jogador fica logo abaixo; os painéis de logística, comércio e recrutamento ficam abaixo dele.
- O relógio não altera as regras de produção, construção ou combate, que continuam usando segundos. A virada de semana fecha o mercado (recalcula preços e renova as cotas) e cobra os juros da dívida.

## Jogador
- Cada partida cria a entidade ECS de identidade do jogador local, com o `PlayerId` gerado pelo Model, o nome configurado e o componente de dono. Ela ainda não tem posição, vida, combate nem representação visual.
- O avatar só spawna quando o jogador coloca o Centro de Comando: nesse momento recebe `Position`, vida e atributos de combate e passa a ser representado no mundo.
- O avatar tem 10 de vida, causa 1 de dano com pistola, tem alcance de 3 tiles e velocidade de 3,5 tiles/s (a velocidade do Bode).
- WASD controla o avatar nas quatro direções da tela isométrica. O movimento é contínuo em coordenadas lógicas; diagonais mantêm a mesma velocidade.
- O ponto inicial é a casa livre mais próxima, fora do Centro de Comando colocado pelo jogador. Ao chegar a 0 de vida, o avatar reaparece nessa casa com a vida cheia.
- O avatar colide com os footprints das construções, tem raio de colisão de 0,3 tile e desliza pelas paredes ao mover-se na diagonal.
- A câmera acompanha o avatar; o zoom permanece fixo.
- No sandbox atual há somente o jogador local. O `PlayerId` é propagado aos Controllers e Views pelo event bus, permitindo que novos jogadores sejam adicionados depois.

## Modo debug
- `F3` liga e desliga o modo debug em qualquer cena. Quando ativo, o custo em recursos de todas as construções e unidades é zero. A construção ignora a entrega de materiais, mas mantém o tempo nominal; a interface mostra `Gratis`, e o recrutamento de drones também não exige nem desconta recursos. O modo debug também zera o custo em crédito energético e não registra oferta nem demanda no mercado.
- Painel no canto superior esquerdo com FPS e tempo do frame (verde ≥ 55, amarelo ≥ 30, vermelho abaixo disso) e a cena atual. Durante a partida mostra também a quantidade de construções e tropas, a posição e o zoom da câmera, e o tile sob o mouse.
- Em cada entidade da partida aparecem:
  - o contorno da área ocupada no chão (ciano);
  - para tropas, a caixa de clique usada na seleção (amarelo);
  - um ponto na posição (magenta) e o rótulo `#id Nome (x, y)`. Para tropas a posição é contínua. Para construções aparecem a origem e o tamanho.

## Mapa
- Grid isométrico. Toda posição de jogo é uma coordenada de tile (lógica), independente da projeção na tela.
- Tile de 128x64 px na tela (zoom 1).
- Cada partida gera um mapa finito de 1024x1024 tiles a partir de uma seed aleatória, mantida no Model para reproduzir a geração. A origem lógica do mapa é (-512, -512), com o ponto inicial em (0, 0).
- O mapa tem terra transitável e água em regiões conectadas, com alvo aproximado de 30% de tiles de água. Movimento de jogadores e unidades, assim como construção, só é permitido em terra; tiles fora dos limites também são bloqueados.
- O Centro de Comando também precisa ter pelo menos um tile de terra adjacente para permitir o surgimento do avatar.
- Jazidas de ferro, cobre e carvão ocupam áreas de 2x2 tiles em terra. A área inicial contém 2 jazidas de ferro, 1 de cobre e 2 de carvão; fora dela são geradas mais 50 jazidas de cada recurso. Cada tipo de mina só pode ser construído sobre uma jazida do recurso correspondente.
- `BoundedMap` representa o retângulo lógico e seu terreno, sem dependência de raylib. A View recebe o mapa por evento e desenha somente os tiles visíveis.
- Câmera: acompanha o avatar do jogador com zoom fixo de 1,0x.
- Renderização de construções, tropas e avatar usa o algoritmo do pintor: objetos mais distantes no eixo de profundidade do grid são desenhados primeiro.

## Economia / recursos
- O inventário é composto por slots; cada slot armazena um item e sua quantidade atual em unidades inteiras (não há frações de item). Cada pilha comporta até 100 unidades, exceto arma (1 por pilha). O inventário do jogador tem 32 slots, o de cada construção tem 64, o de cada tropa tem 2 e o de cada drone de transporte tem 10. O drone de construção usa 2 slots e carrega no máximo 10 unidades de um único recurso por viagem.
- O jogador começa com 500 unidades de ferro, 10 armas e 1000 munições. Ao colocar o Centro de Comando, os materiais de construção do inventário do jogador (ferro, carvão, aço, cobre, eletrônicos e ferramentas) são transferidos para o inventário do Centro; armas e munição permanecem com o jogador. Os demais inventários de construções começam vazios. Itens definidos: ferro, carvão, aço, cobre, eletrônicos, ferramentas, arma e munição.
- A tecla `I` abre/fecha o inventário do jogador. Ao selecionar uma construção, o painel mostra o inventário do jogador e o da construção lado a lado. Arrastar uma pilha de um painel ao outro transfere o máximo que couber; o restante permanece na origem se não houver espaço. `ESC` ou `X` fecha o painel.
- Produção usa taxas por segundo, mas itens e consumo são sempre unidades inteiras. O progresso fracionário da taxa fica acumulado internamente até completar uma unidade; produção também respeita os itens disponíveis e a capacidade do inventário de saída:
  - Mina de Ferro: produz 1 ferro/s sem consumir entradas.
  - Mina de Carvão: produz 1 carvão/s sem consumir entradas.
  - Siderurgia: consome 1 ferro/s e 1 carvão/s e produz 0,5 aço/s.
  - Mina de Cobre: produz 1 cobre/s sem consumir entradas.
  - Fábrica de Eletrônicos: consome 1 ferro/s e 1 cobre/s e produz 0,5 eletrônico/s.
  - Fábrica de Ferramentas: consome 1 aço/s e 1 eletrônico/s e produz 0,5 ferramenta/s.
  - Fábrica de Armas: consome 1 aço/s, 1 eletrônico/s e 1 ferramenta/s e produz 0,5 arma/s.
  - Fábrica de Munição: consome 1 aço/s e 1 cobre/s e produz 0,5 munição/s.
- Construções têm custo e tempo nominal de construção. A colocação apenas cria um canteiro (verde/vermelho na prévia não depende de recursos) e não desconta nada do jogador: os materiais são entregues ao canteiro por drones de construção e consumidos proporcionalmente ao progresso. Se faltar qualquer material, o progresso pausa até a entrega; não é preciso ter todos os recursos para começar. O Centro de Comando é a exceção e fica pronto instantaneamente. O recrutamento de tropas continua instantâneo, requer Quartel e não tem custo nem limite populacional.
- Cada Centro de Comando colocado concede 1 drone de construção grátis ao lado dele. Mais drones de construção podem ser recrutados no Centro de Comando por 50 de ferro + 50 de eletrônicos, sem limite, pagos do inventário do próprio Centro. O drone de construção é uma unidade logística sem combate: busca o material faltante no Armazém mais próximo que tenha estoque e, se nenhum Armazém tiver, no Centro de Comando mais próximo; entrega no canteiro e repete até a construção terminar. Cada viagem carrega até 10 unidades de um recurso. O tempo nominal é mantido mesmo com vários drones; mais drones só reduzem a pausa por falta de material.
- No modo debug, a construção ignora os materiais, mas continua respeitando o tempo nominal. O modo debug também zera os custos em crédito energético e não registra oferta nem demanda no mercado.
- Casa e Gerador permanecem como estruturas sem efeitos econômicos nesta etapa. Tropas de combate recebem um inventário de 2 slots para arma e munição.
- O Depósito existente passa a ser chamado **Armazém**, mantém sua área 2x2 e inventário de 64 slots, custa 100 de ferro e permite recrutar drones de transporte. Cada drone é recrutado instantaneamente e custa 50 de ferro e 50 eletrônicos, sem limite.
- Cada construção produtora cria rotas de saída para todos os Armazéns do mesmo jogador e para as construções do mesmo jogador que consomem seu produto. Ao criar um Armazém ou uma construção consumidora, também são criadas rotas de produtores existentes para ela. Cada rota tem ID único, origem, destino, item e quantidade de drones atribuídos; sua lista pertence à construção de origem.
- Um drone de transporte é uma unidade logística com dono, sem combate nem ordens de tropa. É recrutado no Armazém e pode ser atribuído ou desatribuído de uma rota pela interface da construção de origem. Ao desatribuir um drone que carrega itens, ele retorna à origem e devolve a carga antes de ficar ocioso. O drone percorre continuamente origem → destino → origem a 2 tiles/s, usando os caminhos do mapa.
- Cada drone tem inventário de 10 slots, com pilhas de até 100 itens. Ele carrega o produto da origem até preencher o inventário ou esgotar o estoque disponível, descarrega no destino o que couber e mantém a carga restante se o destino estiver cheio. Sem produto, aguarda na origem; se não houver espaço no destino, aguarda no destino para tentar descarregar novamente.
- O estado visual do drone é `Unloaded` quando o inventário está vazio e `Loaded` quando contém qualquer item.

## Crédito energético e mercado
- Todo recurso tem um preço em **crédito energético (EC)**, a moeda do jogo. O crédito é **adicional** aos materiais: consumir um recurso desconta o item do inventário e cobra o preço de mercado em EC.
- Todo jogador começa com **1.000 EC**. O saldo pode acumular ou ficar negativo; não existe falência no sandbox.
- O consumo de materiais por produção, construção e recrutamento de drones cobra o preço vigente por unidade. A produção de itens não cobra nada: apenas registra oferta para o mercado. A cobrança da construção é proporcional ao material consumido, não à colocação do canteiro.
- O preço fica fixo durante a semana. Quando a semana fecha:
  - `demanda = consumo interno + vendas externas`;
  - `oferta = produção`;
  - `fator = clamp((demanda + profundidade) / (oferta + profundidade), 0.5, 2.0)`;
  - `preço = preço base × fator`;
  - os contadores de oferta e demanda zeram. Sem fluxos na semana, o preço volta ao valor base.
- Dívida paga **5% de juros por semana**, arredondado para cima e somado ao saldo negativo. Saldo positivo não rende juros.
- O **Centro Comercial** é do jogador e determina a demanda externa: cada centro compra automaticamente o estoque armazenado até a **cota semanal por item**, ao preço da semana. A cota renova a cada semana. Itens podem chegar por transferência manual ou pelas rotas automáticas de drones.
- Centro Comercial: área 3x3, custo de 200 ferro, 12 s, requer Centro de Comando, 500 de vida e inventário de 64 slots. Cada centro tem a própria cota.
- Tabela de preços (EC por unidade):

| Item | Preço base | Profundidade | Cota semanal |
|---|---:|---:|---:|
| Ferro | 1 | 500 | 1000 |
| Carvão | 1 | 500 | 800 |
| Aço | 5 | 300 | 500 |
| Cobre | 1 | 400 | 600 |
| Eletrônicos | 5 | 150 | 250 |
| Ferramentas | 25 | 80 | 100 |
| Arma | 80 | 50 | 500 |
| Munição | 2 | 500 | 3000 |

- A interface mostra o crédito e os juros da semana no painel do jogador, o custo estimado em EC na barra de construção e, ao selecionar um Centro Comercial, preço, cota restante/total e estoque de cada item.
- **TBD**: consumo de munição em combate entrar no mercado como demanda.

## Construção
- Barra de construção no rodapé da tela: clique (ou teclas `1`-`9`) seleciona a estrutura. Clicar de novo na mesma desmarca.
- Com uma estrutura selecionada, a prévia segue o mouse (verde = válido, vermelho = inválido). O tile sob o mouse é o centro da área ocupada.
- Botão esquerdo constrói. Segurar e arrastar continua construindo a cada tile novo (útil para muros).
- Botão direito ou `ESC` cancela a seleção.
- Estruturas ocupam uma área quadrada de tiles e não podem se sobrepor.
- Estruturas atuais:

| Estrutura | Área | Custo | Tempo | Regra |
|---|---|---|---|---|
| Centro de Comando | 5x5 | Grátis | Instantâneo | Um por jogador. Precisa ser a primeira construção. Depois de construído, some da seleção. Concede 1 drone de construção grátis. |
| Muro | 1x1 | 100 de cada recurso | 3 s | Só liberado depois que o jogador tem um Centro de Comando. |
| Boneco de Treinamento | 1x1 | Grátis | 3 s | Requer Centro de Comando; tem 100 de vida e serve de alvo de treino: pode ser atacado por qualquer tropa, inclusive as do próprio jogador. |
| Casa | 2x2 | 100 de cada recurso | 8 s | Requer Centro de Comando; sem efeito econômico nesta etapa. |
| Gerador | 2x2 | 100 de cada recurso | 8 s | Requer Centro de Comando; sem efeito econômico nesta etapa. |
| Armazém | 2x2 | 100 ferro | 8 s | Requer Centro de Comando; inventário de 64 slots; recruta drones de transporte. |
| Quartel | 3x3 | 100 de cada recurso | 12 s | Requer Centro de Comando; libera recrutamento. |
| Cruzador | 3x3 | 100 de cada recurso | 12 s | Requer Centro de Comando; recebe duas tropas para gerar uma descendente. |
| Torre | 2x2 | 100 de cada recurso | 8 s | Requer Centro de Comando. |
| Mina de Ferro | 2x2 | 100 ferro | 8 s | Requer Centro de Comando; inventário de 64 slots; produz 1 ferro/s. |
| Mina de Carvão | 2x2 | 100 ferro | 8 s | Requer Centro de Comando; inventário de 64 slots; produz 1 carvão/s. |
| Siderurgia | 3x3 | 300 ferro | 12 s | Requer Centro de Comando; inventário de 64 slots; consome 1 ferro/s e 1 carvão/s e produz 0,5 aço/s. |
| Mina de Cobre | 2x2 | 100 ferro | 8 s | Requer Centro de Comando; inventário de 64 slots; produz 1 cobre/s. |
| Fábrica de Eletrônicos | 3x3 | 200 ferro, 100 aço | 12 s | Requer Centro de Comando; inventário de 64 slots; consome 1 ferro/s e 1 cobre/s e produz 0,5 eletrônico/s. |
| Fábrica de Ferramentas | 3x3 | 200 aço, 200 ferro, 100 eletrônicos | 12 s | Requer Centro de Comando; inventário de 64 slots; consome 1 aço/s e 1 eletrônico/s e produz 0,5 ferramenta/s. |
| Fábrica de Armas | 3x3 | 200 aço, 200 ferro, 100 eletrônicos | 12 s | Requer Centro de Comando; inventário de 64 slots; consome 1 aço/s, 1 eletrônico/s e 1 ferramenta/s e produz 0,5 arma/s. |
| Fábrica de Munição | 3x3 | 200 aço, 200 ferro, 100 cobre | 12 s | Requer Centro de Comando; inventário de 64 slots; consome 1 aço/s e 1 cobre/s e produz 0,5 munição/s. |
| Centro Comercial | 3x3 | 200 ferro | 12 s | Requer Centro de Comando; inventário de 64 slots; vende o estoque automaticamente até a cota semanal. |

- Clicar numa construção pronta abre seu inventário para consulta e transferência ao jogador.
- Enquanto a construção não termina, o prédio aparece como um fantasma translúcido com uma barra de progresso acima; só ao concluir ele recebe o comportamento funcional (produção, arma, recrutamento etc.) e passa a ser um alvo válido como construção pronta. Um canteiro tem vida e pode ser destruído; ao ser destruído, libera a área e o progresso é perdido.
- O Armazém (antigo Depósito) também abre o painel de logística, onde drones podem ser recrutados. Selecionar uma construção produtora mostra as rotas de saída e permite atribuir drones ociosos.
- **TBD**: demolição/reembolso.

## Defesa (tower defense)
- Torre defensiva: construção 2x2, tem 500 de vida, alcance de 6 tiles e causa 20 de dano a cada segundo com o Canhão (tiro direto, cinético).
- A torre ataca automaticamente a unidade ou avatar inimigo mais próximo já dentro do alcance. Torres e unidades podem receber ordens de ataque por comando do Model; alvos aliados, mortos ou sem vida são inválidos, com exceção do Boneco de Treinamento, que pode ser atacado até pelo próprio dono. Ataques fora de alcance não causam dano.
- O Boneco de Treinamento é removido ao chegar a zero de vida e libera o tile que ocupava.
- Ataques reduzem vida; ao chegar a zero, unidades ou construções são removidas do mundo e liberam a área ocupada.
- Ondas e comportamento/pathfinding de inimigos ainda não fazem parte desta fatia; os alvos inimigos são entidades de jogador distintas criadas pelo jogo ou pelos testes.

## Armas, projéteis e dano
- Cada arma tem um tipo que define balística, tipo de dano, dano, alcance, cooldown, raio de área e velocidade de projétil. A entidade guarda dano e alcance efetivos (a genealogia pode combiná-los), mas o comportamento vem sempre do tipo.
- Armas atuais (alcance e raio em tiles, dano por disparo):

| Arma | Balística | Dano | Dano | Alcance | Cooldown | Área | Usada por |
|---|---|---|---|---|---|---|---|
| Pistola | Direta | Cinético | 1 | 3 | 1 s | — | avatar do jogador |
| Fuzil | Direta | Cinético | 10 | 4 | 1 s | — | Soldado |
| Rifle de Precisão | Direta | Cinético | 8 | 5 | 1 s | — | Morcego |
| Espingarda | Direta | Cinético | 14 | 1 | 1 s | — | Bode |
| Canhão | Direta | Cinético | 20 | 6 | 1 s | — | Torre |
| Morteiro | Parabólica | Explosivo | 15 | 8 | 1 s | 2 | reservada para testes e conteúdo futuro |

- **Balística direta (hitscan)**: o disparo resolve no mesmo instante. Exige linha de visão livre entre atirador e alvo; se estiver bloqueada, a unidade ou torre segura o fogo, não gasta munição e não reinicia o cooldown. Dispara assim que a visão abrir.
- **Linha de visão**: bloqueada pelo footprint de qualquer construção e por tiles `Blocked` do terreno. Água, tropas e projéteis não bloqueiam. Os footprints do atirador e do alvo são ignorados, então quem atira de dentro de uma construção ou mira numa construção não bloqueia o próprio tiro.
- **Balística parabólica**: cria uma entidade de projétil com tempo de voo proporcional à distância e à velocidade da arma. Ignora obstáculos e resolve o dano no impacto. O ponto de impacto é travado no disparo (o projétil não persegue o alvo); se o alvo morrer antes, o projétil ainda cai e explode no ponto travado.
- **Dano cinético**: atinge somente o alvo.
- **Dano explosivo**: atinge com dano cheio todas as entidades com vida dentro do raio, incluindo aliados, construções do próprio dono e o próprio atirador se estiver no raio. Não há queda de dano pela distância.
- O disparo consome 1 munição no momento do disparo (não no impacto). Torres e o avatar não precisam de munição; tropas precisam de arma e munição.
- O cooldown é por arma e o primeiro disparo após uma ordem de ataque espera um cooldown completo.
- Genealogia: a descendente herda o tipo de arma do pai de maior alcance (empate: primeiro pai) e mantém dano somado e maior alcance.
- **TBD**: armas parabólicas jogáveis, alcance mínimo de morteiro, resistência/armadura por tipo de dano, representação visual do arco e sprites de projétil.

## Unidades
- Tropas móveis controladas pelo jogador (estilo RTS). Cada tropa tem dono.
- **Recrutamento**: clicar no próprio Centro de Comando (sem construção selecionada) seleciona o Centro e abre o painel de recrutamento no canto superior direito. O painel fecha com o `X`, `ESC` ou ao selecionar outra coisa / clicar no chão vazio.
  - No MVP sandbox, o recrutamento exige ao menos um Quartel e é instantâneo: a tropa aparece no tile livre mais próximo do Centro. Não há custo nem limite populacional nesta etapa.
- **Seleção**: clique esquerdo numa tropa seleciona só ela. Arrastar com o botão esquerdo desenha uma caixa e seleciona todas as tropas do jogador dentro dela. `Shift` soma à seleção atual. Clique no chão vazio (sem `Shift`) limpa a seleção. Selecionar uma construção tira as tropas da seleção e vice-versa.
- **Inventário das tropas**: selecionar tropas abre o mesmo painel de inventário usado pelas construções, com uma linha por tropa mostrando vida, arma e munição atuais. O painel se atualiza sozinho quando a munição muda (tiros e reabastecimento) e fecha ao limpar a seleção; o `X`/`ESC` também o fecha até a seleção mudar.
- **Ordens**: botão direito sobre uma unidade inimiga, uma construção inimiga ou um Boneco de Treinamento ordena ataque às tropas selecionadas; no chão, manda as tropas para o tile clicado. O clique considera toda a área visível do sprite, então acertar o topo de uma construção também funciona.
  - Formação: cada tropa recebe um tile livre diferente, os mais próximos do destino. A tropa mais próxima de cada tile fica com ele.
  - Caminho por A* em 8 direções, desviando de construções e sem cortar quinas. Tropas não colidem entre si durante o trajeto, mas paradas nunca dividem tile.
  - Se uma construção for colocada no caminho, a tropa recalcula a rota. Se o destino não for alcançável, ela para no tile livre mais próximo de onde está.
- Não é possível construir sobre um tile com tropa (parada, andando ou com o tile reservado como destino).
- Na formação, as tropas maiores escolhem o lugar primeiro. Depois, as mais próximas do destino.
- `ESC` com tropas ou construção selecionadas limpa a seleção; sem seleção, volta ao menu.
- Tipos atuais (velocidade em tiles/s, alcance em tiles):

| Tropa | Vida | Dano | Alcance | Velocidade | Arma |
|---|---|---|---|---|---|
| Soldado | 100 | 10 | 4 | 2.0 | Fuzil |
| Morcego | 60 | 8 | 5 | 1.6 | Rifle de Precisão |
| Bode | 140 | 14 | 1 | 3.5 | Espingarda |

- As tropas usam as armas da tabela de **Armas, projéteis e dano**. Uma ordem de ataque dispara após um cooldown de um segundo e repete enquanto o alvo permanecer dentro do alcance. Tiros diretos exigem linha de visão; sem ela, a tropa segura o fogo.
- Ao receber uma ordem de ataque, a tropa caminha pela rota até o alvo **entrar no alcance da arma e para ali** — não é preciso encostar. O primeiro tile da rota em que o alvo já está no alcance vira o destino; se ela já estiver no alcance, não se desloca. O disparo continua exigindo alvo no alcance e linha de visão; sem caminho até um tile dentro do alcance, ela não atira. Torres e outras entidades sem `Mover` não se deslocam e atacam apenas o que estiver no alcance.
- **Suprimentos**: cada tropa tem inventário de 2 slots (arma e munição) e carrega no máximo 1 arma e 100 munições. A arma é obrigatória para atacar e cada disparo consome 1 munição.
- **Máquina de estados**: a tropa fica `Idle` quando está com arma e munição; entra em `SeekingSupplies` ao nascer ou quando a munição chega a zero; e fica em `WaitingForSupplies` quando não há suprimentos disponíveis ou o depósito encontrado está vazio.
  - Ao nascer, busca 1 arma e 100 munições. Com munição zerada, volta a buscar uma recarga completa automaticamente.
  - Procura primeiro o Armazém mais próximo que tenha o item necessário; se nenhum Armazém tiver estoque, usa o Centro de Comando mais próximo que tenha estoque. Armazém tem preferência sobre o Centro quando ambos podem suprir.
  - Se nenhum Armazém ou Centro de Comando tiver arma ou munição, a tropa para onde está e permanece parada, sem atacar, até aparecer estoque.
  - Ao chegar ao depósito, transfere 1 arma (se faltar) e completa a munição até 100 usando o inventário do prédio.

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

## Multiplayer (planejado)
- Rede via ENet.
- Regras de PvE/PvP, sincronização e autoridade (servidor/cliente): **TBD**.
- Desde já, as entidades devem guardar o jogador dono para não impedir o multiplayer depois.
