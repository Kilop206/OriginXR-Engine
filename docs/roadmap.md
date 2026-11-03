# OriginXR roadmap — versão 0.2

## Arquitetura

```text
                         Sandbox / jogo C++
                                |
                            Engine
               ________________|________________
              |                                 |
         originxr_core                   originxr_graphics
     Scene / SceneIO(v1,v2)               OpenGL 3.3
     Mesh, Material, Spin                  shaders + UV
     AssetManager (CPU)             cache GPU + instancing
     Camera / Math / XRMath           RenderView / metrics
     InputState (agnóstico)       GLFW window / input adapter
              |                                 |
         testes CTest                      desktop / stereo
```

## Status do Milestone 2 — v0.2

Implementado:
- `MeshData` indexed + `Vertex` com normal e UV, built-ins cubo e plano.
- Importador Wavefront OBJ para polígonos simples; triangulação em leque.
- Texturas PPM P3/P6; material com cor, textura e `unlit`.
- Caches de recursos CPU e GPU, validando IDs/formatos.
- Agrupamento de entidades por malha + material, usando `glDrawElementsInstanced` no OpenGL 3.3.
- Métricas de draw calls, triângulos, instâncias e tempo CPU.
- Entrada separada do GLFW (`InputState`), `RenderView` e funções de matrizes XR.
- Formato `.oxscene 2`, mantendo leitura de `.oxscene 1`.
- CTest headless e CI.

Pendente no escopo expandido de renderização:
- glTF 2.0 completo e materiais PBR.
- Texturas comprimidas e PNG/JPEG, streaming e asset unloading.
- Render targets configuráveis (necessários para swapchains OpenXR), sombras, gamma/sRGB.
- Backends além de OpenGL e camada de input de controladores.

## Milestone 3 — Editor e ferramentas

- ImGui: hierarquia, seleção, inspetor e perfis.
- Gizmos de transform, undo/redo e picking.
- Importação glTF 2.0 por loader validado; conversor de materiais/recursos.
- Pipeline de testes gráficos em máquinas com GPU e verificação em Windows MinGW.

## Milestone 4 — OpenXR / RV real

- Descoberta de runtime OpenXR e suporte `XR_KHR_opengl_enable`.
- `xrCreateInstance`, `xrGetSystem`, sessão, reference space, state/events.
- Negociação de formato/quantidade de imagens de swapchain por olho; FBO por imagem.
- Loop de `xrWaitFrame`, `xrBeginFrame`, `xrLocateViews`, `xrEndFrame`.
- Transformação de poses/FOV para cada olho. **Nunca reutilizar** o IPD de desktop no headset.
- OpenXR Actions: controladores, poses, interação e haptics.
- Testes reais em runtime e headset.

## Milestone 5 — RA

- Escolher plataforma e método (OpenCV + ArUco com webcam, ou plataforma XR com passthrough/scene-understanding).
- Calibração de câmera, âncoras, tracking, permissões, composição e oclusão.

## Limitações explicitadas

O OpenXR não está integrado à v0.2. OpenGL/GLFW não fornece rastreamento espacial nem um compositor VR. A renderização estéreo lado a lado **não substitui** a submissão de camadas ao runtime. Os testes atuais são principalmente CPU/headless e a execução real de GL em GPU precisa ser validada na máquina de destino.
