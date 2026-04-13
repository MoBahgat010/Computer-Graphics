# Modular SciFi MegaKit Standard - Content Overview

## Folder location in this workspace

The pack is installed under a nested path:

- Modular SciFi MegaKit[Standard]/Modular SciFi MegaKit[Standard]

## Main content groups

This pack provides the same model library in multiple export formats:

- OBJ

Each format contains these categories:

- Aliens
- Columns
- Decals
- Platforms
- Props
- Walls

## Textures included

The Textures folder contains shared material maps used by the models, including:

- BaseColor maps
- Normal maps
- ORM maps (Occlusion/Roughness/Metallic packed)
- Decal and emissive variants
- Detail masks

Example floor texture set:

- T_Trim_02_BaseColor.png
- T_Trim_02_Normal.png
- T_Trim_02_ORM.png

## Best assets to build your ground (floor)

Use the Platforms category for map ground pieces. It includes tiles and transition pieces such as:

- Platform_Squares
- Platform_Metal
- Platform_DarkPlates
- Platform_Simple and Platform_Simple2
- Platform_Ramp_2, Platform_Ramp_4, Platform_Ramp_4Wide
- Platform_Stairs_2, Platform_Stairs_4, Platform_Stairs_4Wide
- Curved/round variants for corners and turns

This makes it easy to build modular floors, ramps, and level transitions.

## Which format to use in this project

For this current game project, OBJ is the practical choice because the current mesh pipeline loads OBJ files.

Recommended import source:

- Modular SciFi MegaKit[Standard]/Modular SciFi MegaKit[Standard]/OBJ/Platforms

## Material note

One sampled platform material uses the floor material MI_Trim_02_Floor and references:

- T_Trim_02_BaseColor.png
- T_Trim_02_Normal.png
- T_Trim_02_ORM.png

If your renderer uses separate texture slots, map them as:

- BaseColor -> Albedo slot
- Normal -> Normal slot
- ORM -> split/packed channel handling in your shader

## License

The included license states this Standard pack content is under CC0 (Public Domain Dedication), from Quaternius.
