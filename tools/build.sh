#!/usr/bin/env bash
# Stage the Molotov mod for packing: copies Molotov/ to build/Molotov/ with every .p3d
# binarized (ODOL, model.cfg baked in). DayZMissions' tools/build_mod.py packs and signs
# build/Molotov as @Molotov (~/Projects/DayZMissions/mods/Molotov is a symlink to it).
#
#   blender --background --python blender/build_molotov.py   # after changing the model
#   blender --background --python blender/build_ruins.py     # after changing the ruins
#   tools/build.sh
#
# Binarize runs under GE-Proton in its own prefix (shared with HandCart's img2paa.sh; never
# the DayZ Tools prefix in compatdata/). It scans its whole drive for addons, so the source
# gets a drive of its own: P: is a scratch copy of Molotov/, O: the output.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STEAM_ROOT="${STEAM_ROOT:-$HOME/.steam/debian-installation}"
GE="$(ls -d "$STEAM_ROOT"/compatibilitytools.d/GE-Proton* | sort -V | tail -n1)"
BINARIZE="$STEAM_ROOT/steamapps/common/DayZ Tools/Bin/Binarize/binarize.exe"
export WINEPREFIX="${IMG2PAA_PREFIX:-$HOME/.local/share/dayz-img2paa-prefix}/pfx"
export WINEDEBUG=-all

WORK="$ROOT/build/.binarize"
rm -rf "$WORK"
mkdir -p "$WORK/P" "$WORK/O"
cp -r "$ROOT/Molotov" "$WORK/P/"
ln -sfn "$WORK/P" "$WINEPREFIX/dosdevices/p:"
ln -sfn "$WORK/O" "$WINEPREFIX/dosdevices/o:"

cd "$WORK/P"
timeout 300 "$GE/files/bin/wine64" "$BINARIZE" -targetBonesInterval=56 -maxProcesses=1 -always \
	"-textures=O:\\tex" "P:\\Molotov" "O:\\Molotov" >"$WORK/binarize.log" 2>&1 || {
	echo "binarize failed, see $WORK/binarize.log" >&2
	exit 1
}

STAGE="$ROOT/build/Molotov"
# Keep a Workshop id recorded in the staged folder (workshop_publish.sh writes it there)
if [ -f "$STAGE/workshop.id" ] && [ ! -f "$ROOT/Molotov/workshop.id" ]; then
	cp "$STAGE/workshop.id" "$ROOT/Molotov/workshop.id"
fi
rm -rf "$STAGE"
cp -r "$ROOT/Molotov" "$STAGE"
find "$STAGE" -name model.cfg -delete   # baked into the binarized models
while IFS= read -r -d '' src; do
	rel="${src#"$ROOT/Molotov/"}"
	out="$WORK/O/Molotov/$rel"
	if [ "$(head -c 4 "$out" 2>/dev/null)" != ODOL ]; then
		echo "binarize produced no ODOL for $rel, see $WORK/binarize.log" >&2
		exit 1
	fi
	cp "$out" "$STAGE/$rel"
done < <(find "$ROOT/Molotov" -name '*.p3d' -print0)

echo "staged $STAGE ($(find "$STAGE" -name '*.p3d' | wc -l) models binarized)"
