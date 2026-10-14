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
2. **Partida**: o mundo é criado vazio ao iniciar. `ESC` volta ao menu e descarta a partida atual.

## Mapa
- Grid isométrico. Toda posição de jogo é uma coordenada de tile (lógica), independente da projeção na tela.
- Origem do mapa (Tiled `.tmx` ou procedural), tamanho e tipos de terreno: **TBD**.

## Economia / recursos
- **TBD**: tipos de recurso, como são obtidos, custos.
- No sandbox, decidir se os recursos são infinitos ou limitados: **TBD**.

## Construção
- **TBD**: tipos de estrutura, regras de posicionamento (ocupação de tiles, terreno válido), custo, tempo de construção, demolição/reembolso.

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
