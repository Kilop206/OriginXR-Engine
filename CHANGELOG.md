# Changelog

## v0.2.0 — Milestone 2

- Novo `originxr::AssetManager` com cache de recursos CPU independentes de GPU.
- Tipos `MeshData`, `ImageData`, `Vertex`; malhas cubo/plano e OBJ.
- Texturização com PPM, materiais tint/unlit.
- Cache de VAO/VBO/EBO/texturas em GPU, renderização por lotes e instancing OpenGL 3.3.
- `RenderView`, `RenderStats`, `XRMath`, input independente de GLFW.
- Formato de cena v2 com leitura retrocompatível de v1.
- Testes adicionais e cena demonstrativa com objetos instanciados, modelo OBJ e plano texturizado.

**Ainda não:** glTF 2.0, PNG/JPEG, sessão OpenXR, renderização num headset, RA real.

## v0.1.0

- Core com entidades, cena, matemática 3D, câmera, persistência v1 e renderização OpenGL 3.3 desktop.
