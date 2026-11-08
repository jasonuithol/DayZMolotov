# Molotov

A molotov cocktail for DayZ, made from things you already find: a glass bottle, some
petrol and a rag. Light it, throw it, and whatever it hits burns.

![The molotov, lit](Molotov/preview.png)

## In game

- **Making one.** Fill a vanilla glass bottle with at least 200 ml of petrol or diesel
  (pour it from a jerry can), then combine it with a rag in the inventory. One rag from
  the stack goes into the neck.
- **Lighting it.** Hold the molotov with a lighter or matches anywhere in the inventory
  and use **Light the rag**. The vanilla way works too: lighter in hands, molotov on the
  ground.
- **Throwing it.** A lit molotov bursts on whatever it hits (ground, wall, car, person),
  or when it's shot, into a pool of fire about 3.5 m across that burns for 20 s.
- **Holding it too long.** 30 s after lighting it bursts by itself, at your feet if
  you're still holding it. A lit molotov can't go into the inventory.
- **The fire.** Damage goes straight to health, so clothing doesn't help:

  | Who | Per second |
  |---|---|
  | Players | 10 health, 10 shock (they flinch, cry out, and the screen flashes red) |
  | Infected, animals | 25 health |
  | Cars (engine and fuel tank) | 5% of their health |

- **Fuel goes up.** Any vehicle with fuel in it (car, boat, motorbike) caught in the fire
  explodes 3 s later, as does a working fuel pump. The blast does up to 60 health and
  shock within 2 m, falling to nothing at 8 m (infected and animals take 2.5x), and leaves
  a fire about 7 m across burning for 60 s, which can set off the next fuelled vehicle.
  - **Cars** become vanilla's wreck of the same model and colour. Their cargo is thrown
    out round the wreck at a quarter health (their own parts go with them), and a burnt
    wheel and the hood fly off at 45 degrees.
  - **Boats** lose their outboard motor, flung off the stern to splash down and sink;
    the burnt, burst hull sinks where the boat was. Anything in it goes in the water.
  - **Motorbikes** fly apart: wheels, fenders, exhaust and shields everywhere, nothing
    left but the fire.
  - Wrecks and flying bits are gone at restart. Only empty vehicles get the slow 5%/s
    burn.

- **Things burn.** Hay and straw, pallets, woodpiles, lumber piles, wooden sheds,
  outhouses, the hunters' feeding racks, feed shacks and deer stands, wooden houses,
  trees and shrubs catch from the fire and burn for a minute without exploding. Big ones
  burn along their whole length and over the top.
- **Burnt down until restart.** When the fire's out the object is gone for everyone, and
  leaves a Livonia ruin (wooden houses), our own burnt ruin (the big sheds, the hunter's
  shack, the straw stack, the deer stands), a charred stump (trees) or a heap of ash
  (shrubs and small things). Lumber piles burn away. Ruins, stumps and ash glow and smoke
  for 4 minutes, then go cold. Loose loot inside a burning house burns with it.
- **Fire spreads** to anything close: 80% if it's touching the flames, down to 35% at the
  edge of their reach, a few seconds after the fire gets to it; half that in light rain,
  none in heavy rain. At most 24 things lit by spreading burn at once.
- **Fireplaces.** A lit molotov that hits an unlit fireplace lights it, as lighter and
  kindling both.
- **Unlit**, it's just a heavy bottle: dropping or throwing it does nothing. A hand drill
  can't light it.

The model is our own: a vodka bottle with a petrol-soaked rag knotted round the neck and
fuel up to just under half, built from vanilla textures (nothing from other mods). Its
inventory icon leans over a little.

## For server owners

The Workshop description has the step-by-step install. In short: `@Molotov` in the server
folder and on `-mod=`, `keys/tarbaby.bikey` into the server's `keys/`, and the loot entry
in `extras/types_molotov.xml` registered in the mission. The entry never spawns molotovs
(nominal 0, crafted only, like the vanilla torch); it only lets the economy keep crafted
ones.

Burnt map objects come back when the server restarts; ruins, stumps and ash aren't
saved. The numbers live in `MolotovSettings` in
[`Molotov/Scripts/4_World/Molotov/Molotov.c`](Molotov/Scripts/4_World/Molotov/Molotov.c);
there is no settings file yet.

## Layout

| Path | What |
|---|---|
| `Molotov/` | The mod (PBO prefix `Molotov`): config, scripts, model, textures, Workshop files |
| `Molotov/Scripts/4_World/Molotov/` | The molotov and its fire (`Molotov.c`: what burns, spreading, vehicles), the Light the rag action, the recipe, fireplaces, burning players (`MolotovPain.c`), burnt-down objects, ruins and ash (`MolotovGone.c`) |
| `Molotov/Scripts/5_Mission/` | The tilted inventory icon; telling joining players what's burnt down |
| `Molotov/ruins/` | Our burnt ruins and stumps, each hot (glowing) and cold |
| `Molotov/extras/` | Loot entry for server admins (shipped beside `addons/`, not packed) |
| `blender/build_molotov.py` | Builds the model, `models/molotov.blend` and both previews |
| `blender/build_ruins.py` | Builds the burnt ruins and stumps, `models/ruins.blend` and a preview |
| `tools/make_textures.py` | Makes the oily rag texture and material from the installed game's vanilla rag |
| `tools/build.sh` | Binarizes the model and stages the mod in `build/Molotov` |
| `mission/` | Test mission |

## Building

Needs Blender 4.2 with the Arma 3 Object Builder add-on, DayZ Tools (Binarize,
ImageToPAA), GE-Proton and Pillow. Binarize and ImageToPAA run under Proton in their own
prefix (`~/.local/share/dayz-img2paa-prefix`), never DayZ Tools' own.

```sh
tools/make_textures.py      # only when the rag texture or material changes
MOLOTOV_PREVIEW_TEX=$PWD/build/vanilla_png blender --background --python blender/build_molotov.py
blender --background --python blender/build_ruins.py   # only when the ruins change
tools/build.sh
```

Packing and signing is done by DayZMissions' `tools/build_mod.py`
(`~/Projects/DayZMissions/mods/Molotov` is a symlink to `build/Molotov`).

## Testing

[`mission/`](mission) is linked into DayZMissions as `missions/molotov.chernarusplus`.
Players spawn on an empty stretch of the east-coast beach (13480 5700) with ready-made
molotovs, lighters and matches at their feet, and everything that burns laid out along
the beach in three rows: hay, straw, pallets, a lumber pile, the wooden sheds, the
hunters' stands and shack, and four wooden houses.

From DayZMissions, `tools/test.sh molotov` runs it headless with `-missiontest`. It
burns all of that plus, on the Chernogorsk quay, a clothed survivor, an infected, a
fuelled hatchback and boat, two fireplaces and the nearest map pump, hay, shed, lumber
pile and straw stack, and a tree and shrub near the beach. It logs `[MolotovTest]` lines:
the fires, the damage, what went up, fire spreading between two hay bales (chance pinned
to 1), that everything burnt is gone a minute on, and that the ruins cool. Setup and
checks take about 3 minutes. `tools/run_server.sh molotov` and `tools/run_client.sh
molotov` start it for real.

## Releasing

The Workshop keeps only the latest upload, with no version number of its own; the version
comes from a git tag (`workshop_publish.sh` adds it to the description and change note).

1. Date the version's entry in `CHANGELOG.md`, commit, tag and push:

   ```sh
   git tag -a v0.4 -m "Molotov 0.4"
   git push origin main v0.4
   ```

2. Build: `tools/build.sh`.
3. From DayZMissions, upload it yourself (SteamCMD asks for the password and a Steam
   Guard code). The first time, run it twice: the first run creates the item and records
   its id, the second uploads the mod with that id in its `meta.cpp`.

   ```sh
   tools/workshop_publish.sh Molotov <steam login>
   ```

   SteamCMD logging in knocks the desktop Steam client offline: don't publish while
   playing, and click Steam > Go Online afterwards. Then set the tags (the launcher
   ignores items without the Mod tag). Name them: without arguments `workshop_tags.py`
   sets Mod Vehicle Mechanics.

   ```sh
   tools/workshop_tags.py Molotov Mod Weapon Equipment
   ```
4. Tell server admins to update and restart. The key stays `tarbaby.bikey`.
