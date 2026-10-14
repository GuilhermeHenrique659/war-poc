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
2. **Partida**: o mundo é criado vazio ao iniciar. `ESC` cancela a construção selecionada; sem seleção, volta ao menu e descarta a partida atual.

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
- **TBD**: se existem unidades móveis, se são controláveis (estilo RTS) ou automáticas.

## Vitória / derrota
- Sandbox: sem condição de vitória ou derrota.
- Outros modos: **TBD**.

## Multiplayer (planejado)
- Rede via ENet.
- Regras de PvE/PvP, sincronização e autoridade (servidor/cliente): **TBD**.
- Desde já, as entidades devem guardar o jogador dono para não impedir o multiplayer depois.
