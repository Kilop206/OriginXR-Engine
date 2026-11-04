# OriginXR roadmap

## Milestone 1 — v0.1

- Base `Scene`, `Camera`, `Engine`, `Renderer`, CMake, 3D desktop and stereo preview.

## Milestone 2 — v0.2

- Assets CPU/GPU, `MeshComponent`, `MaterialComponent`, OBJ, PPM, GPU instancing and `RenderView`.

## Milestone 3 — v0.3 (current)

Implemented in source:
- Dear ImGui editor, separate from sandbox.
- Hierarchy with parent-child transforms and drag/drop reparenting; scene v3 persistence.
- Inspector for transform, material, visibility, spin; CPU picking and translation gizmo.
- Undo/redo snapshots, play/stop, asset/render stats; headless regression tests.

Pending:
- Real GUI/OpenGL compilation and interaction testing on Windows/MinGW + GPU.
- Better editor viewport through off-screen framebuffer (FBO), resize, and docking.
- Full translate/rotate/scale gizmo (only translate is in viewport; other transforms in inspector).
- glTF 2.0 loader, validated material mapping, native dialogs and filesystem explorer.

## Milestone 4 — XR runtime integration

- OpenXR loader + runtime discovery, instance/session/reference spaces.
- OpenGL swapchains and FBO render targets; frame lifecycle and tracked per-eye poses.
- Controller action maps and interaction tests on compatible hardware.

## Milestone 5 — AR

- Begin with calibrated webcam and fiducial marker tracking, then platform passthrough/anchors.
- Occlusion and environment interaction where platform allows it.
