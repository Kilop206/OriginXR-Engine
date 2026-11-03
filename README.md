# OriginXR Engine v0.2.0 — Milestone 2

**Engine 3D educacional em C++17, sem Unity, Unreal ou Godot.** O Milestone 2 evolui o renderizador fixo da v0.1 para **malhas importadas, materiais, texturas, cache e instancing**, com APIs independentes do backend gráfico e primitivas matemáticas preparadas para OpenXR.

> **Estado:** este projeto não implementa uma sessão OpenXR, não rastreia headsets e ainda não produz RA/RV imersiva. A opção `--stereo` é apenas pré-visualização lado a lado numa janela. O XR real precisa de OpenXR loader, runtime, swapchains, composição e dispositivos compatíveis; ver `docs/roadmap.md`.
>
> **Validação:** o núcleo compilou com GCC 14/C++17 e passou no CTest; o código gráfico passou por checagem sintática com Clang usando interfaces GLFW/OpenGL simuladas. A execução gráfica real precisa ser validada em uma máquina com driver OpenGL 3.3 e GLFW.

## O que mudou desde a v0.1

| Recurso | v0.1 | v0.2 |
|---|---|---|
| Geometria | Cubo fixo | `MeshData`, cubo/plano e importador OBJ (triangulação) |
| Texturas | Nenhuma | Texturas PPM P3/P6, shader UV e RGB |
| Materiais | Cor no MeshComponent | `MaterialComponent` com tint, textura e unlit |
| Recursos | VBO de cubo | `AssetManager` com cache CPU + upload/cache GPU |
| Desenho | Cada entidade = um draw | Batching por malha/material e `glDrawElementsInstanced` |
| Perfis | Contagem FPS | Draw calls, triângulos, instâncias e tempo CPU por frame |
| Câmera | Desktop mono e estéreo | `RenderView` aceita projeções/visões externas |
| Entrada | GLFW acoplado à movimentação | `InputState` e `moveFreeCamera` independentes do GLFW |
| Cenas | `.oxscene` v1 | v2 com malha/material e leitura retrocompatível v1 |
| XR | Simulação estéreo | Matemática de pose/FOV XR, **sem runtime XR** |

O projeto continua intencionalmente pequeno e didático. **Não suporta glTF 2.0, PNG/JPEG, animação esquelética, editor visual nem backends Vulkan/Metal.** Essas funcionalidades estão listadas como futuros incrementos; não são anunciadas como implementadas.

## Estrutura

```text
originxr-engine-v0.2/
├── CMakeLists.txt
├── include/originxr/
│   ├── Assets.hpp       AssetManager, MeshData, ImageData
│   ├── Input.hpp        estados de entrada independentes do dispositivo
│   ├── RenderView.hpp   vistas externas, métricas de renderização
│   ├── XRMath.hpp       pose, FOV e matrizes em coordenadas XR
│   ├── Camera.hpp, Math.hpp, Scene.hpp, SceneIO.hpp
│   └── Engine.hpp, Renderer.hpp
├── src/                 implementação C++
├── assets/
│   ├── meshes/pyramid.obj
│   ├── textures/checker.ppm
│   └── scenes/demo.oxscene, legacy-v1.oxscene
├── apps/sandbox.cpp
├── tests/                testes core + assets + XR math + input
├── docs/roadmap.md
└── .github/workflows/ci.yml
```

## Começando no Windows / MinGW-w64

Pré-requisitos: CMake 3.20+, Ninja, Git, GCC/MinGW-w64 (C++17), driver com **OpenGL 3.3 Core**. Se não houver GLFW instalado, o CMake tentará obtê-lo da tag `3.4` do repositório oficial (requer acesso à internet).

Abra o PowerShell na pasta extraída e execute:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/mingw64/bin/g++.exe
cmake --build build --parallel
.\build\originxr_sandbox.exe
```

Ajuste o caminho do MinGW conforme sua instalação. Para testar o modo estéreo (pré-visualização, não headset):

```powershell
.\build\originxr_sandbox.exe --stereo
```

Na primeira execução a cena contém **16 cubos com o mesmo material**, renderizados em um batch instanciado, mais um modelo OBJ piramidal, uma plataforma com textura quadriculada e um satélite animado. Os recursos são copiados para `build/scenes`, `build/meshes` e `build/textures` na compilação.

### Controles

| Entrada | Comportamento |
|---|---|
| WASD / Mouse | Movimento / olhar |
| Q / E / Shift | Descer / subir / acelerar |
| F1 / F2 | Prévia estéreo / wireframe |
| F5 / F6 | Recarregar / salvar cena |
| F7 / F8 | Criar cubo / apagar última entidade |
| P / R | Pausar animação / redefinir câmera |
| Esc / F10 | Liberar mouse / fechar |

O terminal exibe uma linha de diagnóstico aproximadamente a cada segundo, com **batches/draw calls, número de instâncias, triângulos e custo CPU** do renderizador. A grade conta como um draw call separado.

## Compilar e testar sem gráficos

Se faltar GPU, GLFW ou bibliotecas de desenvolvimento OpenGL:

```powershell
cmake -S . -B build-tests -G Ninja -DORIGINXR_BUILD_GRAPHICS=OFF -DORIGINXR_BUILD_TESTS=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

Esses testes exercitam cálculo vetorial, matriz de projeção, cenas v1/v2, OBJ, PPM, cache de recursos, entrada e transformação XR. Não validam o framebuffer ou hardware.

## API básica — adicionando um objeto texturizado

Use `engine.onUpdate` em `apps/sandbox.cpp`, **depois de criar `engine`** e antes de `engine.run()`. Como `Engine::run()` carrega uma cena do disco, entidades criadas antes de `run()` seriam substituídas:

```cpp
bool added = false;
engine.onUpdate([added](originxr::Scene& scene, float dt) mutable {
    (void)dt;
    if (added) return;
    auto& e = scene.createEntity("Meu objeto");
    e.transform.position = {0.f, 1.f, -3.f};
    e.mesh.asset = "../meshes/pyramid.obj";
    e.material.tint = {1.f, 1.f, 1.f};
    e.material.texture = "../textures/checker.ppm";
    added = true;
});
```

`e.mesh.asset` aceita `builtin:cube`, `builtin:plane` ou um caminho para OBJ. `e.material.texture` aceita um caminho para PPM ou string vazia. Caminhos relativos são resolvidos relativamente à **pasta da cena** aberta. OBJ suporta faces `f`, posições `v`, normais `vn`, UV `vt`, índices negativos e triangulação de polígonos simples; não interpreta automaticamente `.mtl` nem materiais PBR. Se não houver normais nos vértices, calcula uma normal plana por triângulo. Para modelos com polígonos côncavos, triangule antes em um editor de modelos.

Também é possível registrar em memória um recurso `MeshData` ou `ImageData` com ID único via `engine.assets().registerMesh(...)` / `registerImage(...)`. Registre antes de `engine.run()`; recursos registrados são imutáveis para preservar a consistência do cache GPU. Cenas não armazenam os dados binários, apenas IDs/caminhos.

## Arquivos `.oxscene` v2

Uma entidade v2 é representada por:

```text
originxr_scene 2
entity "Meu objeto" 0 1 -3 0 0 0 1 1 1 1 1 1 0 30 0 1 "../meshes/pyramid.obj" "../textures/checker.ppm" 0
```

Campos: nome, posição XYZ, rotação XYZ em graus, escala XYZ, cor RGB, velocidade angular XYZ em graus/s, visibilidade, ID da malha, ID da textura e flag `unlit` (0/1). As cenas **v1 continuam sendo lidas**, assumindo cubo e cor convertida para `MaterialComponent::tint`. Ao salvar, o arquivo usa **v2**.

> Migração de código: a cor da v0.1 era `e.mesh.color`; na v0.2 é `e.material.tint`. A visibilidade continua sendo `e.mesh.visible`.

## Preparação para OpenXR

O arquivo `XRMath.hpp` define `XRPose` e `XRFov`, juntamente com `makeXRView` e `makeXRProjection`. O `Renderer::renderViews` aceita matrizes de view/projection e viewports, ao invés de criar um IPD fixo. No futuro, o backend OpenXR substituirá as projeções de desktop por poses e FOV obtidos do runtime e renderizará em swapchains próprias. **A criação de sessões/swapchains e o uso de headsets ainda não existem nesta versão.**

## Próximos passos

1. Completar importação glTF 2.0 (accessors, índices, materiais, múltiplas primitivas), PNG/JPEG e gerenciamento/descarga de assets.
2. Extrair a interface de renderização de framebuffer, para permitir OpenXR com swapchains e render targets específicos por olho.
3. Introduzir inspeção de cenas, editor ImGui e picking.
4. Integrar OpenXR runtime + sessão + frame loop + head tracking + inputs de controladores; validar em headset real.
5. Escolher backend RA conforme a plataforma, incluindo câmera/passthrough e permissões apropriadas.

Fontes técnicas: [GLFW](https://www.glfw.org/docs/latest/), [OpenXR Registry](https://registry.khronos.org/OpenXR/), [glTF 2.0 Specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html).

## Licença

MIT — consultar `LICENSE`.
