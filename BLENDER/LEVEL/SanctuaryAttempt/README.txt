SANCTUARY OF THE FIRST LIGHT

Environment interpretation for RGB. Player moves toward +X. Camera-side is -Y.

00_PROTECTED_Unreal_Greybox contains every Combined_* mesh in its ORIGINAL saved transform.
All 44 original objects retain original geometry, transforms, UVs, parent links and material slots.
Route objects are locked against accidental selection/transforms. Toggle collection selectability to edit.
The supplied FBX is a merged earlier route with a different Z offset: it is for identification only,
not a replacement for the saved Blender meshes.

02_TEMPLATE_Previous_Terrain retains the existing terrains/rocks unchanged, disabled by default.
Turn the collection's viewport/render switches on to compare with your previous version.
Original mechanical details remain visible in 01_TEMPLATE_Mechanical_Details.

05_VISUAL_Skins_and_Wall_Relief contains optional cosmetic facades over existing metal side faces.
Disable it to see original gameplay materials. No landings or tread geometry are edited.
Gameplay-color slots are not replaced. Facades are cosmetic, not collision meshes.

10_BG_NEAR: gate, threshold, outcrops, trees and waystones; positive-Y scenery.
20_BG_MIDDLE: aqueduct, sanctuary, domes, colonnades, watchtowers.
30_BG_FAR: three mountain ranges and distant citadel silhouettes.
40_FOREGROUND: low framing rocks below the route.
80_LIGHTING: review lighting and atmospheric sky.
90_REVIEW_Cameras: whole-route composition, four side-on perspective views, depth overview.
The source file contains no camera. These are review cameras, not an export of the gameplay camera.
Choose a camera in Scene Properties > Camera, then press Numpad 0.

Most stone pieces have editable bevel modifiers. All new meshes have initial UV coordinates.
Materials are procedural Blender previews; FBX does NOT transfer these node networks to Unreal.
Rebuild materials in Unreal or bake maps when committing to the art direction.
Export only SAN_ geometry, layer by layer. Keep NoCollision on cosmetic background/foreground.
Do not reimport the protected greybox as new collision. Exclude review cameras and lights.
No gameplay code, Unreal assets or map settings were changed by this attempt.
