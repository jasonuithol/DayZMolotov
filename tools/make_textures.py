#!/usr/bin/env python3
"""Make the molotov's own textures from the installed game's vanilla ones (nothing from
the game is kept in this repo; the results are, in Molotov/data/):

  molotov_rag_co.paa   the vanilla rag (dz\\gear\\consumables\\data\\rag_co.paa), darkened,
                       warmed and blotched so it looks soaked in petrol
  molotov_rag.rvmat    the vanilla rag material with a stronger, tighter shine

Run: tools/make_textures.py   (needs Pillow, GE-Proton and DayZ Tools' ImageToPAA)
Also leaves PNGs of the vanilla bottle/rag textures in build/vanilla_png/ for the Blender
preview (MOLOTOV_PREVIEW_TEX=build/vanilla_png).
"""
import os, random, re, struct, subprocess, sys
from PIL import Image, ImageChops, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STEAM = os.environ.get('STEAM_ROOT', os.path.expanduser('~/.steam/debian-installation'))
ADDONS = os.path.join(STEAM, 'steamapps/common/DayZ/Addons')
WORK = os.path.join(ROOT, 'build', 'vanilla_png')
DATA = os.path.join(ROOT, 'Molotov', 'data')
PROTON = os.path.join(STEAM, 'compatibilitytools.d/GE-Proton10-32/proton')
IMAGETOPAA = os.path.join(STEAM, 'steamapps/common/DayZ Tools/Bin/ImageToPAA/ImageToPAA.exe')
PREFIX = os.environ.get('IMG2PAA_PREFIX', os.path.expanduser('~/.local/share/dayz-img2paa-prefix'))


def cstr(f):
    out = b''
    while True:
        c = f.read(1)
        if c in (b'\0', b''):
            return out
        out += c


def pbo_file(pbo, name):
    """One uncompressed file out of a PBO, by its path inside it."""
    with open(pbo, 'rb') as f:
        entries = []
        while True:
            n = cstr(f)
            packing, _, _, _, size = struct.unpack('<IIIII', f.read(20))
            if n == b'' and packing == 0x56657273:
                while cstr(f) != b'':
                    cstr(f)
                continue
            if n == b'':
                break
            entries.append((n.decode('latin1').replace('\\', '/').lower(), size))
        for n, size in entries:
            data = f.read(size)
            if n == name.lower():
                return data
    raise SystemExit(f'{name} not found in {pbo}')


def paa_to_png(paa_bytes, png):
    paa = png[:-4] + '.paa'
    open(paa, 'wb').write(paa_bytes)
    env = dict(os.environ, STEAM_COMPAT_DATA_PATH=PREFIX, STEAM_COMPAT_CLIENT_INSTALL_PATH=STEAM)
    subprocess.run([PROTON, 'run', IMAGETOPAA, 'Z:' + paa, 'Z:' + png], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
    os.remove(paa)
    if not os.path.exists(png):
        raise SystemExit(f'ImageToPAA made no {png}')


def oily(src, dst):
    rag = Image.open(src).convert('RGB')
    w, h = rag.size
    rnd = random.Random(11)
    # Large soft blotches: where the petrol soaked in most
    noise = Image.new('L', (w // 16, h // 16))
    noise.putdata([rnd.randint(0, 255) for _ in range(noise.width * noise.height)])
    stains = noise.resize((w, h), Image.BICUBIC).filter(ImageFilter.GaussianBlur(24))
    stains = stains.point(lambda v: max(0, min(255, (v - 110) * 3)))
    soaked = ImageChops.multiply(rag, Image.new('RGB', (w, h), (150, 112, 62)))    # brown, warm
    dark = ImageChops.multiply(rag, Image.new('RGB', (w, h), (88, 64, 36)))       # dark patches
    out = Image.composite(dark, soaked, stains)
    out.save(dst)


RAG_RVMAT_STAGES = '''class Stage1
{
	texture = "dz\\gear\\consumables\\data\\rag_nohq.paa";
	uvSource = "tex";
	class uvTransform { aside[] = {1, 0, 0}; up[] = {0, 1, 0}; dir[] = {0, 0, 1}; pos[] = {0, 0, 0}; };
};
class Stage2
{
	texture = "#(argb,8,8,3)color(0.5,0.5,0.5,1,DT)";
	uvSource = "tex";
	class uvTransform { aside[] = {8, 0, 0}; up[] = {0, 8, 0}; dir[] = {0, 0, 1}; pos[] = {0, 0, 0}; };
};
class Stage3
{
	texture = "#(argb,8,8,3)color(0,0,0,0,MC)";
	uvSource = "tex";
	class uvTransform { aside[] = {1, 0, 0}; up[] = {0, 1, 0}; dir[] = {0, 0, 1}; pos[] = {0, 0, 0}; };
};
class Stage4
{
	texture = "dz\\gear\\consumables\\data\\rag_as.paa";
	uvSource = "tex";
	class uvTransform { aside[] = {1, 0, 0}; up[] = {0, 1, 0}; dir[] = {0, 0, 1}; pos[] = {0, 0, 0}; };
};
class Stage5
{
	texture = "dz\\gear\\consumables\\data\\rag_smdi.paa";
	uvSource = "tex";
	class uvTransform { aside[] = {1, 0, 0}; up[] = {0, 1, 0}; dir[] = {0, 0, 1}; pos[] = {0, 0, 0}; };
};
class Stage6
{
	texture = "#(ai,16,2,2)fresnel(0.71,0.6)";
	uvSource = "tex";
	class uvTransform { aside[] = {1, 0, 0}; up[] = {0, 1, 0}; dir[] = {0, 0, 1}; pos[] = {0, 0, 0}; };
};
class Stage7
{
	texture = "dz\\data\\data\\env_land_co.paa";
	uvSource = "tex";
	class uvTransform { aside[] = {1, 0, 0}; up[] = {0, 1, 0}; dir[] = {0, 0, 1}; pos[] = {0, 0, 0}; };
};
'''

# The vanilla rag material (rags_bandages.rvmat), but petrol-soaked: shinier (specular
# 0.11 -> 0.45) with tighter highlights (power 50 -> 80)
RAG_RVMAT = '''ambient[] = {1, 1, 1, 1};
diffuse[] = {1, 1, 1, 1};
forcedDiffuse[] = {0, 0, 0, 0};
emmisive[] = {0, 0, 0, 1};
specular[] = {0.45, 0.42, 0.36, 1};
specularPower = 80;
PixelShaderID = "Super";
VertexShaderID = "Super";
''' + RAG_RVMAT_STAGES


def main():
    os.makedirs(WORK, exist_ok=True)
    os.makedirs(DATA, exist_ok=True)
    paa_to_png(pbo_file(os.path.join(ADDONS, 'gear_drinks.pbo'), 'data/vodkabottles_ca.paa'),
               os.path.join(WORK, 'vodkabottles_ca.png'))
    paa_to_png(pbo_file(os.path.join(ADDONS, 'gear_consumables.pbo'), 'data/rag_co.paa'),
               os.path.join(WORK, 'rag_co.png'))
    oily(os.path.join(WORK, 'rag_co.png'), os.path.join(WORK, 'molotov_rag_co.png'))
    subprocess.run([os.path.join(ROOT, 'tools', 'img2paa.sh'), os.path.join(WORK, 'molotov_rag_co.png'), DATA], check=True)
    with open(os.path.join(DATA, 'molotov_rag.rvmat'), 'w') as f:
        f.write(RAG_RVMAT)
    print('textures:', ', '.join(sorted(os.listdir(DATA))))


main()
