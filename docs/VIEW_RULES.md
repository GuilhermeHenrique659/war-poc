# Regras para a camada de View

Este documento descreve como implementar e integrar a apresentação visual do jogo. Consulte-o antes de alterar `src/view/`.

## Responsabilidades e comunicação

- A View apresenta o estado recebido e encaminha comandos de interface pelo `core::EventBus`; não implementa regras de jogo nem altera o Model diretamente.
- Model, View e Controller não fazem chamadas diretas entre si. A View se inscreve nos eventos relevantes, atualiza seu estado local e desenha a partir dessa cópia.
- Use notificações existentes em `src/core/Events.hpp`. Se faltar um dado ou evento, adicione-o ao fluxo apropriado em vez de consultar o Model ou duplicar regras na View.
- Quando a representação visual de uma entidade depender do seu estado (por exemplo, um sprite diferente quando está carregada), o Model deve publicar uma notificação pelo bus sempre que esse estado mudar, e a View deve atualizar a cópia local da entidade ao recebê-la. Informe também o estado inicial na criação da entidade para que a View comece sincronizada. A View seleciona a representação correspondente ao estado recebido; não consulta o Model nem deduz regras de gameplay a partir de componentes ou inventários.
- A View pode usar raylib para desenhar e para operações visuais. Não deixe `Texture2D`, coordenadas de tela ou chamadas raylib vazarem para o Model.
- Separe responsabilidades em componentes pequenos: a View de cena coordena seus painéis e elementos; ViewModels específicos desenham objetos ou grupos de dados. Siga a composição existente em `src/view/Renderer.cpp` e `src/view/gameplay/PlayingView.cpp`.

## Coordenadas e desenho isométrico

- O Model e os eventos de gameplay usam coordenadas lógicas do grid. Não grave pixels nem valores projetados no estado de gameplay da View.
- Converta entre grid e mundo/tela na View, usando os helpers de `src/view/IsoProjection.hpp` (`gridToWorld`, `spriteDestinationFromNorth`, `spriteDestinationFromSouth` e `screenToGrid`). Não replique as fórmulas de projeção em ViewModels.
- Para sprites de entidades com footprint, escolha o ponto de ancoragem que corresponde à arte: use `spriteDestinationFromSouth` quando a base da imagem deve coincidir com o canto sul do footprint; use `spriteDestinationFromNorth` quando a arte parte do canto norte. Considere o tamanho do frame, não o tamanho total da spritesheet.
- Desenhe elementos do mundo dentro do modo de câmera (`BeginMode2D`/`EndMode2D`); desenhe HUD e painéis de tela fora dele. Preserve o fluxo em `src/view/world/WorldView.cpp` e `src/view/gameplay/PlayingView.cpp`.
- Objetos do mundo compartilham ordenação de profundidade. Ao adicionar uma categoria de entidade, integre-a à ordenação em `WorldObjectsViewModel`, em vez de desenhá-la fora de ordem.
- Use os layouts compartilhados existentes para geometria visual que também participa de interação ou hit-test. Não mantenha cálculos equivalentes divergentes entre View e Controller.

## Texturas, sprites e ciclo de vida

- Mantenha os assets em `resource/`, organizados por tipo, com nomes minúsculos em `snake_case`. Monte caminhos relativos à raiz do executável, como nos ViewModels existentes; nunca dependa do diretório de trabalho.
- Carregue texturas pela infraestrutura existente (`TextureManager`, `SpriteSheet`, `StaticSpriteViewModel` ou `AnimatedSpriteViewModel`). Não carregue texturas repetidamente durante `draw()` e não faça cópia de posse de recursos GPU sem gestão explícita.
- Use uma chave estável e única no `TextureManager`, normalmente no formato `categoria/nome` (por exemplo, `units/drone`). O gerenciador é responsável por liberar as texturas; mantenha a ordem de vida do contexto gráfico e de seus consumidores, conforme a composição em `src/core/Game.*`.
- Para uma spritesheet, informe quantidade de quadros, dimensões de cada quadro e FPS reais ao criar `SpriteSheet`. Confirme que a grade e a contagem cabem na imagem; dimensões fracionárias são válidas quando a imagem e os quadros assim exigirem.
- Componha `AnimatedSpriteViewModel` no ViewModel específico e use `iso::spriteDestinationFromNorth` ou `iso::spriteDestinationFromSouth` para posicioná-lo. `AnimatedSpriteViewModel::drawAt` recebe um destino em pixels e um tint; não desenhe spritesheets manualmente se o helper já resolve a seleção do quadro.
- Para arte estática, use `StaticSpriteViewModel` com o recorte e as dimensões de destino apropriados. Não crie uma implementação paralela de carregamento ou um ViewModel animado de um único frame sem necessidade.
- Passe o alpha como tint quando a mesma arte precisar ser exibida translúcida, por exemplo na prévia de construção. Mantenha a textura original e o estado compartilhado do renderer intactos.
- Falhas ao carregar ou validar um asset devem continuar explícitas. Não substitua erros por textura vazia, sprite invisível ou fallback de sucesso.

## ViewModels e integração

- ViewModels recebem dados necessários para apresentar uma entidade e oferecem métodos de desenho sem consultar o Model. Mantenha a interface mínima e `const` quando possível.
- Conecte cada ViewModel à criação, atualização e remoção da entidade pelos eventos correspondentes. Limpe a cópia local no reset do mundo e ignore atualizações de entidades que não estão mais na View, seguindo os padrões de `src/view/world/WorldObjectsViewModel.cpp`.
- Se um sprite também aparece como prévia, use o mesmo ViewModel ou a mesma rotina de desenho da entidade colocada; varie somente os dados visuais necessários, como alpha.
- Preserve a ordem de declaração dos membros quando houver dependências entre ViewModels: objetos referenciados por outros devem viver por mais tempo que seus consumidores.
- Não acrescente código específico a uma entidade dentro de um ViewModel genérico quando ela puder ter seu ViewModel especializado. Siga os exemplos em `src/view/build/` e `src/view/units/`.

## Testes e validação

- Cubra comportamento visual determinístico sem abrir uma janela: os testes existentes substituem funções pontuais da raylib e verificam frame, textura, destino e tint em `tests/SpriteViewModelTests.cpp` e `tests/TextureManagerTests.cpp`.
- Para animações, teste pelo menos a seleção de quadro em um instante conhecido e a geometria projetada do destino. Inclua alpha quando o desenho tiver suporte a prévia/transparência.
- Para alterações orientadas a eventos, verifique que a View reage às notificações públicas e mantém o estado local consistente, inclusive em reset e remoção.
- Siga o TDD do projeto para mudanças de comportamento. Depois da alteração, compile com os warnings habilitados e execute os testes relevantes; use `ctest --test-dir build --output-on-failure` quando a integração puder afetar mais de um componente.
