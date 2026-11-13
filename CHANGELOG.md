# Changelog

## 0.6 - 2026-11-13

- The vehicle explosions moved out into their own mod,
  [Better Explosions](https://github.com/jasonuithol/DayZBetterExplosions), which Molotov
  now requires (load it before Molotov). Nothing changes in game: a fuelled vehicle or
  pump caught in the fire still goes up 3 s later - wreck, sinking boat, flying motorbike,
  the blast that hurts - and burns for a minute where it stood.
- Vanilla explosives now set fuelled vehicles off too (Better Explosions: landmines, the
  M79's 40 mm round, C4, grenades right against a vehicle), and they burn with Molotov's
  fire, which spreads, like any vehicle that goes up.
- For modders: `MolotovWreck.Add` / `Fling` are now `BetterExplosionsWreck.Add` / `Fling`,
  and the flying bits are `BetterExplosionsDebris` classes.

## 0.5 - 2026-11-10

- Other mods can give more vehicles a wreck and burnt bits that fly off it, on the server
  before anything burns: `MolotovWreck.Add("CarClass", "WreckClass", turn)`, then
  `MolotovWreck.Fling("CarClass", "PieceClass", onEdge)` for each piece. The new Molotov -
  Expansion Vehicles add-on uses it for DayZ Expansion's tractor, bus, UAZ, Land Rover and
  UH-1H.
- A burnt helicopter rotor blade, snapped off near the hub, for the add-on to fling.
- A wreck now lies centred where the vehicle stood, rather than on the vehicle's origin.
- The flying wheel and hood go a little further or shorter each time.

## 0.4.1 - 2026-11-09

- Molotovs can also be made from Nasdara's glass bottle (the game's `WineBottle`, also
  called "Glass Bottle" in game); the recipe only took the vodka-style one.

## 0.4 - 2026-11-08

- Exploding cars leave wrecks: each vanilla car (Niva, Golf, Olga, Sarka, Humvee, M3S) is
  swapped for vanilla's abandoned wreck of the same model and colour. What was in its
  cargo is thrown out on the ground round it (its own parts - wheels, doors, battery,
  plugs, radiator, headlights - go with it). A burnt wheel and the hood fly off at 45
  degrees and land some 8-10 m away. Wrecks and the flying bits are gone at restart.
- Exploding boats: the outboard motor is blown off the stern at 45 degrees, splashes down
  and sinks; the hull, burnt and burst, sinks where the boat was; the fire burns on the
  water. Anything in the boat goes in the water.
- Exploding motorbikes fly apart: their burnt wheels, fenders, exhaust and (on the
  Motorbike_02) windshield and side shields are flung every which way, 7-13 m, leaving
  nothing but the fire. The bits are gone at restart.
- The blast hurts: up to 60 health and shock within 2 m of a vehicle or fuel pump going
  up, falling to nothing at 8 m (infected and animals take 2.5x), with the flinch, cry and
  red flash. The vanilla explosion behind it barely touched players.
- Fires on the sea burn on its surface (they could end up on the seabed).
- The burnt sheds and the hunter's shack leave a proper heap of ash, about 45 cm high in
  the middle, with the fallen beams and planks lying on it; the flat layer of ash
  disappeared into bumpy ground and grass. Our ruins sit on the ground's average height
  over the footprint.

## 0.3 - 2026-11-07

- Things burn. Hay and straw (bales, haystacks, the big straw stacks), pallets, woodpiles,
  lumber piles, wooden sheds, outhouses, wooden guard sheds, the hunters' feeding racks,
  feed shacks and deer stands, the wooden houses, trees and shrubs all catch from a
  molotov's fire and burn as big fires for a minute, with no explosion. Big ones burn
  along their whole length and up over the top: standing on or beside one is deadly.
- Burnt down until the server restarts. When the fire is out the object is gone, on the
  server and for every player (including ones who join later), and leaves:
  - wooden houses: the matching Livonia house ruin (loose loot inside burns with it);
  - the big wooden sheds, hunter's shack, straw stack and deer stands: their own burnt
    ruin (charred posts, wall stubs, fallen beams, planks, ash);
  - trees: a tall charred stump; shrubs and small things: a heap of ash;
  - lumber piles (the loot event) burn away.
  Fresh ruins, stumps and ash glow with embers and smoke for 4 minutes, then go cold.
- Fire spreads. Anything close to a burning object may catch from it a few seconds later:
  80% if it's touching the flames, down to 35% at the edge of their reach. Light rain
  halves that, heavy rain stops it. At most 24 things lit by spreading burn at once, so a
  forest fire creeps along rather than taking the whole map.
- Every vehicle with fuel in it now goes up like a car: boats and motorbikes too. An
  exploding vehicle wrecks what's in it: every item in its cargo loses 75% of its health.
  Empty boats and bikes in the fire take engine damage like empty cars.
- A lit molotov that hits an unlit fireplace lights it, as both lighter and kindling (its
  rag goes in if the fireplace has no kindling).
- A hand drill can no longer light a molotov.
- Burning hurts: players in the flames flinch, cry out and see the red damage flash, and
  a fatal burn holds them at the last of their health for a moment so the health display
  has drained before the screen goes black.
- The bottle smashing now has a sound (it was missing).

## 0.2 - 2026-11-05

- Fuel goes up. A car with fuel in the tank caught in a molotov fire blows up 3 s later
  with the fuel pump explosion (its sound, and 50 health of blast damage within 5 m,
  less further out), a fireball and smoke, and is wrecked. A working fuel pump caught in
  the fire explodes the vanilla way.
- What's left burns for 60 s, about 7 m across, and can set off another fuelled car
  parked close by. Empty cars still just take the slow burn.

## 0.1 - 2026-11-03

First release.

- Molotov cocktail: a glass bottle with at least 200 ml of petrol or diesel + a rag.
- "Light the rag" with a lighter or matches from the inventory (or the vanilla way, lighter
  in hands, molotov on the ground). Lit, it can't be stowed and bursts by itself after 30 s.
- Bursts on impact (or when shot) into a 3.5 m pool of fire that burns for 20 s: players
  10 health/s plus shock, infected and animals 25 health/s, straight past clothing; cars'
  engines and fuel tanks 5%/s.
- Own model: a vodka bottle with a petrol-soaked rag knotted round the neck, half full of
  fuel, built from vanilla textures; tilted inventory icon.
- Loot entry for servers in extras/types_molotov.xml (crafted only, never spawns).
