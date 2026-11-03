# OriginXR — arquitetura e roadmap

## Fluxo da aplicação

```text
            sandbox.cpp / projeto do usuário
                        |
                     Engine
               _________|___________
              /                     \
        Core (sem GLFW)         Graphics (GLFW)
      Scene / Entities          OpenGL Renderer
      Transform / Spin          shaders + VAO/VBO
      Camera + Math             stereo preview
      SceneIO (.oxscene)         keyboard + mouse
```

## Sistemas atuais

1. **Scene**: cria/remove entidades, centraliza a atualização de rotação dos objetos e expõe os componentes via API C++.
2. **SceneIO**: cenas de texto versionadas; parser transacional (não altera a cena aberta em caso de falha).
3. **Camera**: calcula matrizes `view` e `projection`. O preview estéreo desloca os olhos com IPD simulado e usa frustum assimétrico; não possui distorção óptica nem pose de headset.
4. **Renderer**: backend OpenGL 3.3; pipeline de shaders, depth test, viewport/scissor, cubos e grid.
5. **Engine**: inicialização, loop de frames, entrada e callback de lógica `onUpdate()`.

## Próximos marcos

### 0.2 — Renderização extensível

- Componentes Mesh e Material com cache de recursos.
- Carregamento `glTF 2.0`, texturas, instancing e tratamento de cores.
- Backends de entrada e renderização desacoplados de GLFW.
- Perfis de frame e logs de diagnóstico.

### 0.3 — Editor

- ImGui para hierarquia de cenas e inspetor de componentes.
- Gizmos para mover/girar/alterar escala, salvar e desfazer ações.
- Picking por raycasting e IDs de entidades.

### 0.4 — Runtime OpenXR (RV real)

- Vincular **OpenXR loader** e negociar extensões com o runtime instalado.
- Implementar `xrCreateInstance`, `xrGetSystem`, sessão gráfica e swapchains.
- No loop: processar eventos, esperar frame, localizar vistas/poses e submeter camadas (`xrWaitFrame`, `xrBeginFrame`, `xrLocateViews`, `xrEndFrame`).
- Usar resoluções, FOV, poses e texturas **fornecidas pelo runtime**. Não reutilizar o IPD/convergência arbitrários da prévia desktop.
- Actions, controladores, bindings e vibração.
- Necessário headset/runtime compatível para validação real.

### 0.5 — RA

- Escolher hardware/plataforma. Em PC, protótipo de marcador ArUco usando OpenCV + webcam; em headset de RA, passthrough/ambient integration via capacidades específicas do runtime.
- Estimativa de pose, calibração intrínseca da câmera, composição e oclusão.
- Evitar alegar SLAM universal: implementações e permissões dependem do dispositivo.

### 1.0 — Produto de engine

- Pipeline de recursos, profiler, render graph, execução de scripts, editor, plugins e empacotamento.
- Testes end-to-end em dispositivos compatíveis e suporte multiplataforma validado.

## Notas de projeto

- O núcleo não precisa de renderização; por isso executa testes em CI sem GPU.
- `Scene::createEntity` retorna uma referência que pode ser invalidada caso a cena crie mais entidades. Para persistência, use `EntityId` e `find`.
- O callback `onUpdate` na versão 0.1 aceita apenas um sistema do usuário por vez; refinar para registrar múltiplos sistemas.
- OpenGL 3.3 **não** é OpenXR. A prévia estéreo não transforma uma janela em conteúdo pronto para um headset.
