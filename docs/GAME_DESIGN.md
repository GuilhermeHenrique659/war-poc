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
1. **Menu principal**: opções `Iniciar` (começa uma partida) e `Sair`. Navegação por teclado (cima/baixo + confirmar) ou mouse (hover + clique).
2. **Partida**: o mundo é criado vazio ao iniciar. `ESC` cancela a construção selecionada ou limpa a seleção de tropas/construção; sem nada selecionado, volta ao menu e descarta a partida atual.

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
- Por enquanto, para testes, o mapa é **infinito** e sem terreno: qualquer tile livre é válido.
- Câmera: WASD/setas ou botão do meio do mouse arrastando para mover. Roda do mouse para zoom.
- Origem do mapa definitivo (Tiled `.tmx` ou procedural), tamanho e tipos de terreno: **TBD**.

## Economia / recursos
- **TBD**: tipos de recurso, como são obtidos, custos.
- No sandbox, decidir se os recursos são infinitos ou limitados: **TBD**.

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

- Custo: grátis no sandbox por enquanto.
- **TBD**: tempo de construção, demolição/reembolso, terreno válido.

## Defesa (tower defense)
- **TBD**: torres, alcance, dano, alvo prioritário.
- **TBD**: inimigos, ondas, rotas/pathfinding.

## Unidades
- Tropas móveis controladas pelo jogador (estilo RTS). Cada tropa tem dono.
- **Recrutamento**: clicar no próprio Centro de Comando (sem construção selecionada) seleciona o Centro e abre o painel de recrutamento no canto superior direito. O painel fecha com o `X`, `ESC` ou ao selecionar outra coisa / clicar no chão vazio.
  - No sandbox o recrutamento é **grátis e instantâneo**: a tropa aparece no tile livre mais próximo do Centro.
- **Seleção**: clique esquerdo numa tropa seleciona só ela. Arrastar com o botão esquerdo desenha uma caixa e seleciona todas as tropas do jogador dentro dela. `Shift` soma à seleção atual. Clique no chão vazio (sem `Shift`) limpa a seleção. Selecionar uma construção tira as tropas da seleção e vice-versa.
- **Movimento**: botão direito no mapa manda as tropas selecionadas para o tile clicado.
  - Formação: cada tropa recebe um tile livre diferente, os mais próximos do destino. A tropa mais próxima de cada tile fica com ele.
  - Caminho por A* em 8 direções, desviando de construções e sem cortar quinas. Tropas não colidem entre si durante o trajeto, mas paradas nunca dividem tile.
  - Se uma construção for colocada no caminho, a tropa recalcula a rota. Se o destino não for alcançável, ela para no tile livre mais próximo de onde está.
- Não é possível construir sobre um tile com tropa (parada, andando ou com o tile reservado como destino).
- Na formação, as tropas maiores escolhem o lugar primeiro. Depois, as mais próximas do destino.
- `ESC` com tropas ou construção selecionadas limpa a seleção; sem seleção, volta ao menu.
- Tipos atuais (velocidade em tiles/s, alcance em tiles):

| Tropa | Vida | Dano | Alcance | Velocidade |
|---|---|---|---|---|
| Infantaria | 100 | 10 | 1 | 2.0 |
| Longa Distância | 60 | 8 | 5 | 1.6 |
| Cavalaria | 140 | 14 | 1 | 3.5 |

- Vida, dano e alcance já são guardados, mas ainda não têm efeito: combate é **TBD** (ataque, alvos, cadência, morte).

### Juntar tropas (merge)
- Com 2 ou mais tropas selecionadas aparece o painel de merge no canto superior direito, mostrando os atributos da tropa que vai nascer. O botão `Juntar` (ou a tecla `M`) junta **todas** as tropas selecionadas em uma **Tropa Combinada** (um mech).
- Qualquer combinação vale: tipos iguais, diferentes e tropas que já são combinadas.
- Atributos da tropa nova:
  - Vida máxima = soma das vidas máximas. Vida atual = soma das vidas atuais.
  - Dano = soma dos danos.
  - Alcance = o maior alcance.
  - Velocidade = média ponderada pela quantidade de tropas recrutadas dentro de cada uma. Juntar em etapas dá o mesmo resultado que juntar tudo de uma vez.
- **Tamanho**: poder = vida máxima + 10 × dano. O lado do quadrado ocupado é `1 + poder / 400` (divisão inteira), de 1x1 até no máximo 5x5. Ex.: 2 Infantarias = poder 400, vira 2x2.
- Requisitos (sem eles o botão fica desativado e mostra o motivo):
  - Todas as tropas a até 3 tiles do centro do grupo ("Muito longe").
  - Um espaço livre do tamanho da tropa nova a até 3 tiles do centro do grupo ("Sem espaco"). Os tiles das tropas que vão se juntar contam como livres.
- O merge é instantâneo. As tropas de origem somem, a nova aparece no espaço livre mais próximo do centro e fica selecionada.
- Tropas maiores que 1x1 ocupam e reservam o quadrado inteiro: nada é construído em cima e o caminho só passa por onde o quadrado inteiro cabe (um mech 2x2 não passa num vão de 1 tile).
- **TBD**: desfazer o merge (separar), custo ou tempo do merge fora do sandbox.
- **TBD**: custo e tempo de recrutamento fora do sandbox, limite de tropas, colisão entre tropas.

## Vitória / derrota
- Sandbox: sem condição de vitória ou derrota.
- Outros modos: **TBD**.

## Multiplayer (planejado)
- Rede via ENet.
- Regras de PvE/PvP, sincronização e autoridade (servidor/cliente): **TBD**.
- Desde já, as entidades devem guardar o jogador dono para não impedir o multiplayer depois.
