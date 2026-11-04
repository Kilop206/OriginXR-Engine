# OriginXR Engine v0.3 — Milestone 3: Editor Visual

Mini-engine 3D **C++17 / OpenGL 3.3 / GLFW / Dear ImGui**, desenvolvida sem Unity, Unreal ou Godot. A v0.3 amplia a v0.2 com um editor visual independente da sandbox. **Ainda não há suporte a headset OpenXR, passthrough RA ou glTF**.

## Como compilar no Windows (MinGW-w64)

Requisitos: CMake >= 3.20, Ninja, GCC/MinGW-w64 com suporte a C++17, Git, drivers OpenGL 3.3 e internet na primeira configuração para baixar GLFW 3.4 e Dear ImGui v1.91.9b (pinned). Execute no PowerShell dentro da pasta do projeto:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/mingw64/bin/g++.exe `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel

.\build\originxr_editor.exe
```

Para abrir um exemplo de hierarquia v3:

```powershell
.\build\originxr_editor.exe .\build\scenes\hierarchy-demo.oxscene
```

Para continuar usando a sandbox da v0.2:

```powershell
.\build\originxr_sandbox.exe
```

Se o Ninja não estiver instalado, use um gerador disponível e configure o CMake novamente em outra pasta. Ajuste os caminhos `C:/mingw64` conforme a instalação do seu compilador. **Não misture compiladores/geradores no mesmo diretório `build`.**

Para compilar sem interface e executar os testes (não requer download de dependências gráficas):

```powershell
cmake -S . -B build-tests -G Ninja -DORIGINXR_BUILD_GRAPHICS=OFF -DORIGINXR_BUILD_TESTS=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

Para desabilitar somente o editor e manter a sandbox:

```powershell
cmake -S . -B build -DORIGINXR_BUILD_EDITOR=OFF
```

## Funcionalidades do editor

| Painel | Funcionalidade |
| --- | --- |
| Hierarchy | Exibe pais/filhos, seleciona entidades, arrasta entidades para alterar o pai, botão Scene Root para desvincular |
| Inspector | Edita nome, posição, rotação, escala, malha, visibilidade, tint, textura PPM, iluminação unlit e velocidade de rotação |
| Viewport | Renderiza cena 3D em região reservada, seleção por ray casting e gizmo translacional XYZ desenhado via ImGui |
| Assets / Diagnostics | Mostra estatísticas de renderização, total de assets e histórico |
| File | Abre e salva cenas `.oxscene` v3, inclusive convertendo arquivos v1/v2 ao salvar |
| Edit | Undo/redo de mudanças, duplicação e exclusão |
| Play/Stop | Executa simulação local temporária e restaura cena editável ao parar |

### Atalhos e manipulação

- Clique esquerdo em um objeto na **Viewport**: selecionar entidade (ray casting CPU contra caixas orientadas); o teste usa uma caixa aproximada para modelos OBJ.
- Arraste um dos eixos coloridos (X vermelho, Y verde, Z azul): **mover** a entidade selecionada no espaço de seu pai.
- Segure **botão direito do mouse** sobre a Viewport e use **W/A/S/D**, **Q/E** (altura), **Shift** (acelerar): navegar a câmera.
- **Ctrl+S**: salvar cena; **Ctrl+Z / Ctrl+Y**: desfazer/refazer; **Delete**: excluir seleção (desde que um widget de texto não esteja ativo).
- **File > Save As**: digite o caminho no popup; **File > Open scene**: informe o caminho da cena.
- **Inspector**: o nome, os IDs de malha e os caminhos de textura são aplicados ao pressionar **Enter**; demais campos são editáveis diretamente.
- **Hierarchy**: arraste o item para outro item para alterar o pai; solte em “Scene Root” para voltar à raiz. A transformação local é preservada no reparenting (a posição no mundo pode mudar).
- **Play** anima `SpinComponent` temporariamente; o botão **Stop** recupera o estado anterior à simulação.

## Estrutura

```text
originxr-engine-v0.3/
├── apps/
│   ├── editor.cpp            # Dear ImGui UI / desktop viewport
│   └── sandbox.cpp           # Sandbox v0.2 (mantida)
├── include/originxr/
│   ├── Scene.hpp             # Entity.parent, setParent, worldMatrix
│   ├── SceneIO.hpp           # Leitura v1/v2/v3; gravação v3
│   └── editor/EditorModel.hpp# UndoHistory e ray picking (sem ImGui)
├── src/
│   ├── Scene.cpp
│   ├── SceneIO.cpp
│   ├── Renderer.cpp           # Transformações acumuladas por hierarquia
│   └── editor/EditorModel.cpp
├── assets/
│   └── scenes/
│       ├── demo.oxscene            # Cena existente, formato v2
│       └── hierarchy-demo.oxscene  # Exemplo de pais/filhos, formato v3
├── tests/
│   ├── test_core.cpp
│   ├── test_assets.cpp
│   └── test_editor.cpp
└── CMakeLists.txt
```

A biblioteca `originxr_core` **não depende de ImGui nem de OpenGL**. A GUI vive em `originxr_editor`. Os testes headless cobrem parsing de cenas v1/v2/v3, reparenting, rejeição de ciclos, transformações hierárquicas, undo/redo e picking CPU.

## `.oxscene` versão 3

Cada linha de entidade inclui `id parent_id` antes do nome:

```text
originxr_scene 3
entity 1 0 "Parent" 0 0 -3 0 0 0 1 1 1 1 1 1 0 0 0 1 "builtin:cube" "" 0
entity 2 1 "Child" 1 0 0 0 0 0 0.5 0.5 0.5 1 0.3 0.2 0 0 0 1 "builtin:cube" "" 0
```

A propriedade `parent_id=0` representa uma entidade raiz. O parser valida referências de pais e ciclos. O editor lê versões v1/v2 e salva v3, preservando IDs da v3. Para proteção do original, **faça uma cópia da cena antes de salvá-la em outro formato**.

## Limitações verificadas e pontos pendentes

- A biblioteca core **foi compilada e passou nos testes CTest com GCC 14 no Linux**.
- O código **GUI/OpenGL não foi compilado ou executado com GLFW/ImGui reais neste ambiente**, portanto a primeira execução gráfica precisa ser verificada no Windows.
- O editor possui **um gizmo de translação**. Rotação e escala podem ser editadas no Inspector, mas ainda não têm manipuladores 3D completos.
- A seleção aproximada de arquivos OBJ assume dimensões locais equivalentes a um cubo unitário; não utiliza a bounding box real da malha. Não há hierarquia de ossos.
- Arquivos `.obj` simples e texturas `.ppm` continuam funcionando; **glTF 2.0, materiais PBR, diálogos nativos de arquivos e thumbnails** permanecem para futuras versões.
- Não há docking nem painel de arquivos, apenas um layout fixo. O viewport renderiza no framebuffer principal (sem render target próprio).
- Undo/redo é baseado em snapshots de `Scene`, limitado a 64 alterações; não inclui modificações feitas durante a simulação.
- Ainda não existe integração nativa com runtime/headset OpenXR; visão estereoscópica da sandbox é apenas prévia desktop.

## Sobre dependências

O projeto usa Dear ImGui sob licença MIT, obtido pelo `FetchContent` durante a compilação, não incluído no ZIP. GLFW também é obtido automaticamente se não estiver instalado. Veja [Dear ImGui](https://github.com/ocornut/imgui) e [GLFW](https://github.com/glfw/glfw).
