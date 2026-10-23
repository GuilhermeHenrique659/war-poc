# AGENTS.md

Jogo de estratégia com construção de base e elementos de tower defense. C++ + raylib 5.5.

## O jogo
- Perspectiva **isométrica** (2D, tiles em losango). O mundo é modelado em coordenadas de grid (lógicas) e a conversão grid ↔ tela (e tela ↔ grid, para picking do mouse) é responsabilidade da View. O Model nunca conhece pixels nem a projeção isométrica.
- Modo atual: **single player sandbox**. Multiplayer (PvE cooperativo e PvP via ENet) está planejado, mas fora do escopo por enquanto. Não acople a lógica a um único jogador local de um jeito que impeça o multiplayer depois (ex.: componentes guardam o dono/jogador).
- Regras de negócio e mecânicas ficam em [`docs/GAME_DESIGN.md`](docs/GAME_DESIGN.md). Consulte antes de implementar gameplay e atualize o documento quando uma regra for definida ou mudar. Itens marcados como **TBD** ainda não foram decididos: não invente a regra, pergunte.

## Assets / recursos
- Todos os assets ficam em `resource/` na raiz do repo, separados por tipo:
  - `resource/textures/`: sprites, tilesets, UI.
  - `resource/audio/`: efeitos (`sfx/`) e músicas (`music/`).
  - `resource/fonts/`, `resource/maps/` e outras subpastas conforme necessário.
- Use nomes em minúsculas, sem espaços nem acentos (`snake_case`), por exemplo `resource/textures/tiles/grass_01.png`.
- Carregamento de assets é coisa da View (ou de um gerenciador de recursos usado por ela), sempre com RAII. O Model referencia assets só por ID/nome, nunca por `Texture2D`/`Sound`.
- Monte os caminhos a partir do diretório do executável (`GetApplicationDirectory()`), não do diretório de trabalho. Se for preciso, adicione ao CMake a cópia de `resource/` para `build/` ao lado do `.exe`.

## Build / rodar
- Configurar: `cmake --preset default` (Ninja + g++, saída em `build/`). Compilar: `cmake --build --preset default`. Rodar: `.\build\war_poc.exe`.
- Para compartilhar uma versão de teste no Windows: `cmake --preset release`, `cmake --build --preset release --target package`. O ZIP `build/release/war_poc-windows-x64.zip` contém o executável e, se existir, a pasta `resource/`; quem testar só precisa extrair o ZIP e abrir `war_poc.exe`, sem instalar compilador ou bibliotecas do projeto.
- Execute os testes com `ctest --test-dir build --output-on-failure`. O alvo `war_poc_tests` usa GoogleTest, não depende de raylib e não abre uma janela. A primeira configuração baixa o GoogleTest via CMake `FetchContent`.
- Para verificar uma mudança, compile sem warnings (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion` estão ligados) e execute os testes relevantes.
- Os `.cpp` são achados por `GLOB_RECURSE` em `src/`. Não precisa listar arquivos novos no CMake.
- O executável é linkado com `-static` (raylib estática) e não precisa de DLLs. Hoje só a raylib está linkada. ENet e tmxlite ainda não estão no CMake.
- O caminho do repo tem acento (`Área de Trabalho`): o `make` do MSYS2 quebra e o link via `cmd.exe` corrompe caminhos absolutos. Por isso o build usa Ninja e as libs são passadas por `-L` com caminho **relativo** (veja `CMakeLists.txt`). Mantenha esse padrão ao adicionar libs.

## Toolchain / dependências
- Compilador local: MSYS2 UCRT64 `g++` 15 e `ninja` (`C:\msys64\ucrt64\bin`). As libs em `lib/` são `.a`/`.dll.a` no formato MinGW, então não use MSVC (`cl`).
- Libs vendorizadas: raylib (`libraylib.a` estática ou `raylib.dll` + `libraylibdll.a`), ENet (`libenet.dll.a` + `libenet-7.dll`), tmxlite (`libtmxlite.a` estática ou `.dll`).
- Raylib estática no Windows também precisa de `-lopengl32 -lgdi32 -lwinmm`. ENet precisa de `-lws2_32 -lwinmm`.
- Ao linkar tmxlite estática (`libtmxlite.a`), defina `TMXLITE_STATIC`. Sem ele, os headers usam `__declspec(dllexport)`.
- As DLLs usadas (`libenet-7.dll`, `raylib.dll`, `libtmxlite.dll`) precisam estar ao lado do executável.
- `windows.h` (necessário para o ENet) conflita com raylib (`CloseWindow`, `Rectangle`, `DrawText`...). Inclua `include/raylib_win.h` **antes** de `raylib.h`/`enet.h` em qualquer arquivo que use os dois. Melhor ainda: isole o código de rede em seus próprios arquivos `.cpp`.
- O caminho do repo tem espaços e acentos. Use aspas nos caminhos em comandos de shell.

## Arquitetura (obrigatória)
- MVC: Model, View e Controller não se chamam diretamente. Eles se comunicam só por um **event bus** (`src/core/EventBus.hpp`, síncrono: o handler roda dentro do `publish`). Todos os eventos ficam em `src/core/Events.hpp`.
- Fluxo: o Controller (`src/controller/`) lê a entrada da raylib e publica *comandos* (`...Requested`). O `model::GameModel` reage, muda o estado e publica *notificações* (`SceneChanged`, `MenuStateChanged`). A View (`src/view/`) e o Controller guardam uma cópia local do estado recebido por evento. Não leem o model direto.
- O Model usa **ECS** (`src/model/ecs/Registry.hpp`: entidades = IDs, componentes = structs de dados puros, sistemas = lógica). O mundo da partida é o `world_` em `GameModel`. A lógica de jogo não deve chamar raylib. Só a View desenha.
- `src/core/Game.*` é a raiz de composição. A ordem de declaração dos membros importa: o bus deve viver mais que quem se inscreve nele, e a janela mais que quem usa o contexto GL. `GameModel::initialize()` publica o estado inicial e só deve rodar depois que todos se inscreveram.
- A geometria compartilhada entre View e Controller (hit-test do mouse) fica em `src/view/MenuLayout.hpp`.
- ESC está desligado como tecla de saída da raylib (`SetExitKey(KEY_NULL)`). Para sair, publique `events::QuitRequested`.
- Todo o código do projeto fica em `src/`. Não edite `include/` nem `lib/`, porque são de terceiros.
- Ao implementar ou alterar a camada de apresentação, siga também as regras de [`docs/VIEW_RULES.md`](docs/VIEW_RULES.md).

## Regras de código
- Sempre use smart pointers (`std::unique_ptr` por padrão, `std::shared_ptr` só quando a posse for de fato compartilhada). Nada de `new`/`delete` direto nem ponteiro cru com posse.
- Nenhum arquivo pode passar de 500 linhas. Divida antes de chegar no limite.
- Prefira early return / guard clauses em vez de `if` aninhados.
- Siga boas práticas de C++ moderno: RAII (inclusive para recursos da raylib como `Texture2D` e `Sound`), const-correctness, `enum class`, nada de `using namespace` em headers.

## Testes e TDD (obrigatório)
- Implemente mudanças de comportamento com **TDD**: primeiro escreva um teste que falha, implemente o mínimo para fazê-lo passar e só então refatore, mantendo a suíte verde.
- Estruture cada cenário pela filosofia **Given / When / Then**: deixe claro o estado inicial (Given), execute uma única ação observada (When) e verifique o resultado público (Then). Isso é uma forma de pensar e organizar o teste, não uma exigência de macros específicas; comentários curtos podem separar as etapas quando ajudarem a leitura.
- Os testes ficam em `tests/`, em arquivos terminados em `Tests.cpp`; o CMake os descobre automaticamente. Use GoogleTest (`TEST`, `TEST_F`, `EXPECT_*` e `ASSERT_*`) para escrevê-los.
- Priorize testes unitários determinísticos da lógica do Model e dos sistemas ECS. A View/raylib não deve ser necessária para testar regras de jogo.
