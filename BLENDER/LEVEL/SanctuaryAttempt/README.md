# Sanctuary of the First Light

A first editable environment interpretation of RGB's intro route. The player
proceeds along **positive X**, with the viewing side at **negative Y**.

Open `LEVEL_INTRO_Sanctuary.blend` in Blender 5.2 and press **Numpad 0** to see
the overview. The original `LEVEL_INTRO.blend` and `Greybox.FBX` are unchanged.

## Art direction

Warm sandstone, oxidized bronze, faded saffron banners and broad mountain
silhouettes. The composition progresses from an entrance gate, through an open
valley with a broken aqueduct, toward a domed sanctuary behind the vertical
climb. Carved facades visually connect the route to the architecture. Cooler,
lower-contrast mountains give the background depth.

## Collections

| Collection | Contents |
| --- | --- |
| `00_PROTECTED_Unreal_Greybox` | All ten original `Combined_*` objects; locked against accidental selection and transformation. |
| `01_TEMPLATE_Mechanical_Details` | Existing mechanical detail, retained visibly. |
| `02_TEMPLATE_Previous_Terrain` | Existing rocks/terrain retained at their original transforms; hidden by default for comparison. |
| `05_VISUAL_Skins_and_Wall_Relief` | Optional cosmetic side-face skins, masonry, inset arches and friezes; switch off to see the original materials. |
| `10_BG_NEAR` | Gate, outcrops, threshold, trees and waystones; Y approximately 15–63. |
| `15_VALLEY_FLOOR` | A continuous decorative canyon floor behind the route. |
| `20_BG_MIDDLE` | Aqueduct, sanctuary and watchtowers; Y approximately 72–188. |
| `30_BG_FAR` | Mountain ranges and distant silhouettes; Y approximately 190–630. |
| `40_FOREGROUND` | Sparse low rocks on the camera side, below the main route. |
| `80_LIGHTING_and_Atmosphere` | Review lighting and sky backdrop. |
| `90_REVIEW_Cameras` | Overview, entrance, aqueduct, threshold, sanctuary and an oblique view showing depth. |

The source contains **no camera object**. The new cameras are composition
references, not replicas of the Unreal gameplay camera. Select another camera
in **Scene Properties → Camera**, then press Numpad 0. The overview uses
2000 × 900; use 1600 × 900 for the four perspective cameras to match the saved
sectional previews. The oblique depth preview uses 1800 × 1100.

## Previews

- [Whole route](sanctuary_overview.png)
- [Entrance](01_entrance.png)
- [Aqueduct valley](02_aqueduct.png)
- [Climb threshold](03_threshold.png)
- [Sanctuary](04_sanctuary.png)
- [Depth layers](05_depth_layers.png)

## Preservation and verification

Reopened the source and saved result in Blender 5.2.2 LTS. Checked all **44
original objects**: geometry, transforms, UV coordinates, parent links, material
slots, polygon material assignments, smooth flags and modifier names/types
match. No original object is missing. Both source-file SHA-256 hashes remain
identical, and all eight referenced file textures are packed in the result.

The FBX contains a merged route plus collision reference, with different Z
bounds from the saved Blender scene. The saved Blender scene is the authority
for placement; the FBX was imported only for inspection, not added to the result.

Commands executed from the RGB workspace:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --disable-autoexec --python-exit-code 1 --python 'BLENDER\LEVEL\SanctuaryAttempt\build_sanctuary.py'
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --disable-autoexec --python-exit-code 1 --python 'BLENDER\LEVEL\SanctuaryAttempt\verify_saved_scene.py'
```

Both completed with exit code 0. Six Cycles previews were rendered; overview,
entrance, valley, climb, sanctuary and depth views were visually inspected.
See `verification_saved.json` for the preservation checks and depth bounds.
Git attributes report LFS coverage for the new `.blend` and preview PNGs.

## Bringing the art into Unreal

This is a first environment art pass, not a verified game build. Original
landing surfaces are unchanged; added side-face masonry is cosmetic. All new
scenery carries an `Unreal_collision` note specifying **NoCollision**, but this
metadata does not automatically configure Unreal's collision settings.

Export new `SAN_` geometry in manageable groups from each collection. Exclude
the protected greybox, prior terrain, review cameras and lights. Set cosmetic
meshes to **NoCollision** in Unreal and check framing with the actual gameplay
camera. Blender's procedural materials do not transfer through FBX; recreate
them in Unreal or bake textures. Basic UV coordinates exist on the new meshes,
but they are not finished lightmap UVs or baked texture atlases. Repeated pieces
remain separate and editable; consolidate or instance them for the final game.

No Unreal map, C++ gameplay file or input setting was changed. Unreal import,
performance and Play In Editor verification have not been run for this artwork.
