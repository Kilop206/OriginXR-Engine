# Changelog

## v0.3.0 — Milestone 3 (Editor Visual)

- Added standalone `originxr_editor` using pinned Dear ImGui v1.91.9b, GLFW and OpenGL 3.3.
- Added hierarchy tree with drag/drop parenting, a properties inspector, scene viewport and diagnostics.
- Added mouse-ray picking against oriented primitive bounds and an XYZ translation gizmo.
- Added snapshots-based Undo/Redo; create, duplicate, delete, load, save and play/stop.
- Scene serialization v3 preserves IDs and parents while retaining v1/v2 import.
- Renderer now uses hierarchical world matrices for entities.
- Added dedicated editor tests (3 CTest suites total) and a v3 sample scene.
- Known omissions: glTF, PBR and native OpenXR are not implemented; GUI still needs real-device validation.

## v0.2.0 — Milestone 2

- Added CPU asset cache, OBJ and PPM loaders, indexed rendering, GPU instancing and stats.
- Added XR camera mathematics and input abstractions.
- Scene serialization v2, backward compatible with v1.
