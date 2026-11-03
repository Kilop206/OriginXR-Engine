# OriginXR Engine v0.1 (C++17)

**Uma engine gráfica 3D educacional, criada em C++ do zero e sem Unity, Unreal ou Godot.**
Possui um núcleo independente da GPU, cenas editáveis em arquivo, entidades/componentes, renderização OpenGL 3.3, entrada, câmera FPS e modo estereoscópico **de pré-visualização**.

> **Estado real:** a versão 0.1 ainda NÃO é RA nem RV imersiva. O modo estéreo é lado a lado numa janela de desktop; não lê sensores de headset, não usa OpenXR e não mostra câmeras reais para RA. Isso é uma *base de engine*, não uma engine XR pronta para dispositivos. O passo seguinte é implementar um backend OpenXR completo e, depois, uma pipeline RA.

## Recursos implementados

- `originxr_core`: matemáticas 3D, câmera, entidades, transformações, componente de malha (cubo), componente de rotação e cenas versionadas `.oxscene`.
- `originxr_graphics`: janela GLFW, contexto OpenGL **3.3 Core**, shaders GLSL, VAO/VBO, profundidade, grade espacial e projeções mono/estéreo.
- `originxr_sandbox`: ambiente de demonstração com cena pré-criada, movimentação livre, objetos animados e criação de cubos em execução.
- `originxr_tests`: testes do núcleo sem OpenGL e sem GLFW; executados via CTest.
- Integração no GitHub Actions para compilação e testes headless.

A engine é dividida em duas bibliotecas (`OriginXR::Core` e `OriginXR::Graphics`) para permitir reutilização sem depender de janela ou runtime gráfico no núcleo.

## Estrutura

```text
originxr-engine/
├── CMakeLists.txt
├── include/originxr/
│   ├── Math.hpp             # Vec3, Mat4, projeções
│   ├── Camera.hpp           # câmera, visão estereoscópica
│   ├── Scene.hpp            # Entity, Transform, Mesh, Spin, Scene
│   ├── SceneIO.hpp          # leitura/escrita .oxscene
│   ├── Renderer.hpp         # API do renderizador OpenGL
│   └── Engine.hpp           # loop de aplicação e callbacks
├── src/                     # implementação dos módulos
├── apps/sandbox.cpp         # aplicação de exemplo
├── assets/scenes/demo.oxscene
├── tests/test_core.cpp
├── docs/roadmap.md
└── .github/workflows/ci.yml
```

## Compilar no Windows com MinGW-w64

**Pré-requisitos:** CMake 3.20+, Git, MinGW-w64/G++ com C++17, Ninja ou mingw32-make, driver com suporte a OpenGL 3.3. É necessário acesso à internet para baixar GLFW 3.4 na primeira configuração **se GLFW não estiver instalado**.

No terminal PowerShell, dentro da pasta extraída:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/mingw64/bin/g++.exe
cmake --build build --parallel
.\build\originxr_sandbox.exe
```

Ajuste os caminhos de compilador. Se Ninja não estiver instalado, use o gerador alternativo **em um diretório diferente**:

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/mingw64/bin/g++.exe
cmake --build build-mingw --parallel
.\build-mingw\originxr_sandbox.exe
```

Se ocorrer erro de link indicando funções de entrada do MinGW, confira se está usando um único toolchain MinGW-w64 consistentemente (GCC, `mingw32-make`, `ninja`, GLFW compilado por ele). Não misture as bibliotecas compiladas para MSVC.

Se quiser iniciar no modo estéreo:

```powershell
.\build\originxr_sandbox.exe --stereo
```

O CMake copia a cena padrão para `build/scenes/demo.oxscene`. **F6 salva nessa cópia**, preservando o arquivo original em `assets/scenes`.

## Compilação sem gráficos + testes

Útil em CI ou em máquinas sem GLFW, OpenGL ou interface gráfica:

```powershell
cmake -S . -B build-tests -DORIGINXR_BUILD_GRAPHICS=OFF -DORIGINXR_BUILD_TESTS=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

## Controles da sandbox

| Tecla | Ação |
|---|---|
| `W A S D` | Andar no plano horizontal |
| `Mouse` | Girar a câmera |
| `Q / E` | Descer / subir |
| `Shift esquerdo` | Acelerar |
| `F1` | Alternar mono / estéreo lado a lado |
| `F2` | Alternar wireframe |
| `F5` | Recarregar cena do arquivo |
| `F6` | Salvar cena no arquivo |
| `F7` | Criar um cubo à frente da câmera |
| `F8` | Remover a última entidade |
| `P` | Pausar/retomar atualização de cena |
| `R` | Redefinir câmera |
| `Esc` | Capturar/liberar o mouse |
| `F10` | Fechar aplicação |

## Adicionando uma entidade via C++

O trecho abaixo pode ser adicionado em `apps/sandbox.cpp` **depois de criar `engine` e antes de `engine.run()`**. Atenção: a cena de disco é carregada durante `run()` e substitui as entidades criadas antes; para adicionar dinamicamente ao iniciar, use o callback e uma flag local:

```cpp
bool created = false;
engine.onUpdate([created](originxr::Scene& scene, float dt) mutable {
    if (!created) {
        auto& box = scene.createEntity("Minha entidade");
        box.transform.position = {0.0f, 1.0f, -4.0f};
        box.transform.scale = {0.5f, 0.5f, 0.5f};
        box.mesh.color = {0.95f, 0.2f, 0.35f};
        box.spin.degreesPerSecond = {0.f, 45.f, 0.f};
        created = true;
    }
});
```

No exemplo fornecido, `sandbox.cpp` já demonstra `onUpdate()` animando o movimento vertical do satélite.

## Editando cenas sem recompilar

Abra `build/scenes/demo.oxscene` após compilar. Cada linha `entity` contém:

```text
entity "Nome" posX posY posZ rotX rotY rotZ scaleX scaleY scaleZ red green blue spinX spinY spinZ visible
```

- Posição/escala em **metros** e rotação em **graus**.
- `red green blue`: componentes RGB entre 0 e 1.
- `spinX/Y/Z`: graus por segundo.
- `visible`: `1` para renderizar, `0` para ocultar.
- `F5` recarrega sem precisar reiniciar.
- `F6` sobrescreve a cena carregada com o estado atual.

Também é possível abrir um arquivo de cena próprio:

```powershell
.\build\originxr_sandbox.exe "C:\Projetos\minha-cena.oxscene" --stereo
```

## Limitações e próximos passos

O OpenGL 3.3 é usado somente para desktop. Não há importação de modelos, animação esquelética, física, áudio, editor visual, gerenciamento completo de recursos ou Android. As entidades são armazenadas num `std::vector` simples: adequado para começar, mas ainda não é um ECS orientado a dados completo.

Para **RV real**, será necessário usar OpenXR para instância, sistema, sessão, ação dos controladores, projeções fornecidas pelo dispositivo, swapchains e submissão dos frames. Para **RA**, também será necessário lidar com transparência/passthrough ou rastreamento de câmera/marcadores, conforme o hardware. Ver `docs/roadmap.md`.

### Referências

- GLFW: https://www.glfw.org/docs/latest/
- OpenXR: https://www.khronos.org/openxr/
- OpenXR Registry: https://registry.khronos.org/OpenXR/
- OpenGL: https://www.khronos.org/opengl/

## Licença

MIT — ver [LICENSE](LICENSE).
