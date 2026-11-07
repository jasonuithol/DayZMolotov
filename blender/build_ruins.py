"""Procedurally builds the burnt-out remains left where a big wooden thing burned down:
the wooden sheds, the hunter's shack, the big straw stack and the tall deer stands. (The wooden houses use
Livonia's ruins; small things leave a heap of ash - see MolotovGone.c.)

Run headless (from the project root):
    blender --background --python blender/build_ruins.py

Outputs:
    Molotov/ruins/<model>_burnt.p3d   - MLOD P3D per ruin (needs the Arma 3 Object Builder add-on)
    Molotov/ruins/model.cfg
    models/ruins.blend                - source scene
    models/ruins_preview.png          - all of them side by side, textured if build/vanilla_png
                                        holds log01burned_co.png and stoneground_co.png

Each ruin covers the footprint of what it replaces (the collision boxes measured in game,
FOOTPRINTS below), with its bottom at the model's origin height and its footprint centred on
the origin: MolotovGone.LeaveWreck lines the bottom and footprint centre up with the
original's. Only vanilla textures, by their in-game paths: charred wood is the burnt
fireplace log (dz\\gear\\cooking\\data\\log01burned), the ash bed the fireplace's ash
(dz\\gear\\cooking\\data\\stoneground, round with soft see-through edges).
Axes: Blender X/Y/Z = game X/Z/Y (Z up here, Y up in game).
"""

import math
import os
import random

import addon_utils
import bmesh
import bpy
from mathutils import Euler, Matrix, Vector

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "models")
RUIN_DIR = os.path.join(ROOT, "Molotov", "ruins")
PREVIEW_TEX = os.path.join(ROOT, "build", "vanilla_png")

# Each ruin comes twice: <model>_hot glowing (the burning fireplace's log and ash materials,
# embers in the cracks), <model>_burnt cold charcoal (their no-glow twins). The game
# swaps hot for burnt a few minutes after the fire goes out (MolotovGone).
CHAR_TEX = ("dz\\gear\\cooking\\data\\log01burned_co.paa", "dz\\gear\\cooking\\data\\log01burnednoemit.rvmat")
ASH_TEX = ("dz\\gear\\cooking\\data\\stoneground_co.paa", "dz\\gear\\cooking\\data\\stonegroundnoemit.rvmat")
# the hot ones: the vanilla glowing materials with the glow turned up to show in daylight
CHAR_HOT_TEX = (CHAR_TEX[0], "Molotov\\data\\ruin_char_hot.rvmat")
ASH_HOT_TEX = (ASH_TEX[0], "Molotov\\data\\ruin_ash_hot.rvmat")
CHAR_U = 0.82          # the bark runs along u 0..0.82 of the log texture; end grain beyond

A3OB_MODULE = "bl_ext.user_default.Arma3ObjectBuilder"
LOD_VISUAL, LOD_GEOMETRY, LOD_FIRE_GEOMETRY = "0", "6", "15"
MASS_KG = 2000

# What each ruin replaces: footprint (game X, game Z) in metres, from GetCollisionBox
FOOTPRINTS = {
    "shed_w1": ("shed", 5.48, 2.79),
    "shed_w2": ("shed", 4.67, 5.94),
    "shed_w3": ("shed", 5.55, 3.41),
    "shed_w5": ("shed", 5.79, 3.34),
    "shed_w6": ("shed", 6.48, 9.49),
    "misc_feedshack": ("shed", 7.74, 4.50),      # the hunter's shack
    "farm_strawstack": ("straw", 20.17, 8.74),
    "misc_deerstand1": ("stand", 2.50, 2.73),
    "misc_deerstand2": ("stand", 3.91, 3.28),
}
# Burnt trees, chosen by the burnt tree's height (MolotovGone.BurntTree): a big charred
# stump with a ragged top. name: (height, trunk radius)
TREES = {
    "tree_small": (2.8, 0.10),
    "tree_medium": (3.4, 0.14),
    "tree_large": (4.0, 0.19),
}


class Mesh:
    """Boxes and grids in one bmesh, UV'd per face corner, a material per face. Boxes
    also go to a list of convex collision components when asked."""

    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new("UVMap")

    def quad(self, verts, uvs, mat):
        f = self.bm.faces.new(verts)
        f.material_index = mat
        for loop, uv in zip(f.loops, uvs):
            loop[self.uv].uv = uv
        return f

    def box(self, centre, size, rot, mat=0, u0=None):
        """A charred plank/post/beam: size (x, y, z), rot Euler; bark along its longest side"""
        sx, sy, sz = size
        m = Matrix.Translation(centre) @ rot.to_matrix().to_4x4()
        corners = [m @ Vector((x * sx / 2, y * sy / 2, z * sz / 2))
                   for x in (-1, 1) for y in (-1, 1) for z in (-1, 1)]
        v = [self.bm.verts.new(c) for c in corners]
        if u0 is None:
            u0 = random.uniform(0, CHAR_U * 0.5)
        length = max(size)
        u1 = min(CHAR_U, u0 + length * 0.35)
        vv = random.uniform(0, 0.7)
        uvs = ((u0, vv), (u1, vv), (u1, vv + 0.25), (u0, vv + 0.25))
        idx = (0, 1, 2, 3, 4, 5, 6, 7)
        for face in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
            self.quad([v[i] for i in face], uvs, mat)
        return corners

    def to_object(self, name, materials):
        # P3D faces are triangles or quads only: an end cap with more corners would have
        # the exporter drop the whole visual LOD (the tree stumps came out empty)
        ngons = [f for f in self.bm.faces if len(f.verts) > 4]
        if ngons:
            bmesh.ops.triangulate(self.bm, faces=ngons)
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces[:])
        # the ash bed is an open sheet: make sure it faces up
        flip = [f for f in self.bm.faces if f.material_index == 1 and f.normal.z < 0]
        if flip:
            bmesh.ops.reverse_faces(self.bm, faces=flip)
        # every closed part must face out: its signed volume is positive
        inward = 0
        for island in islands(self.bm):
            if any(e.is_boundary for f in island for e in f.edges):
                continue
            vol = sum(f.calc_center_median().dot(f.normal) * f.calc_area() for f in island) / 3
            if vol < 0:
                bmesh.ops.reverse_faces(self.bm, faces=island)
                inward += 1
        if inward:
            print(f"[ruins] {name}: turned {inward} inside-out parts the right way out")
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        obj = bpy.data.objects.new(name, mesh)
        for mat in materials:
            mesh.materials.append(mat)
        return obj


def islands(bm):
    """The connected parts of a mesh, as lists of faces"""
    seen = set()
    parts = []
    for f in bm.faces:
        if f.index in seen:
            continue
        stack, part = [f], []
        seen.add(f.index)
        while stack:
            g = stack.pop()
            part.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in seen:
                        seen.add(h.index)
                        stack.append(h)
        parts.append(part)
    return parts


def ash_bed(mesh, w, d, height_of, mat, n=24):
    """The ash: a grid over w x d with the round ash texture stretched over it once, so its
    soft edges fade into the ground. height_of(u, v) for u, v in -1..1."""
    rows = []
    for j in range(n + 1):
        row = []
        for i in range(n + 1):
            u, v = i / n * 2 - 1, j / n * 2 - 1
            row.append(mesh.bm.verts.new((u * w / 2, v * d / 2, height_of(u, v))))
        rows.append(row)
    for j in range(n):
        for i in range(n):
            q = (rows[j][i], rows[j][i + 1], rows[j + 1][i + 1], rows[j + 1][i])
            uvs = ((i / n, j / n), ((i + 1) / n, j / n), ((i + 1) / n, (j + 1) / n), (i / n, (j + 1) / n))
            f = mesh.quad(q, uvs, mat)
            f.smooth = True


def plank_litter(mesh, w, d, count, mat, z0=0.04):
    """Charred planks lying about, mostly within the footprint"""
    for _ in range(count):
        length = random.uniform(0.7, 2.4)
        c = Vector((random.uniform(-w / 2, w / 2) * 0.9, random.uniform(-d / 2, d / 2) * 0.9, z0 + 0.02))
        rot = Euler((random.uniform(-0.06, 0.06), random.uniform(-0.12, 0.12), random.uniform(0, math.pi)))
        mesh.box(c, (length, random.uniform(0.10, 0.18), 0.03), rot, mat)


def post(mesh, geo, x, y, height, thick, mat, tilt=0.12):
    """An upright charred stump, standing on the ground; a collision box in geo"""
    rot = Euler((random.uniform(-tilt, tilt), random.uniform(-tilt, tilt), random.uniform(-0.3, 0.3)))
    c = Vector((x, y, height / 2))
    mesh.box(c, (thick, thick, height), rot, mat)
    geo.append((c, (thick, thick, height), rot))


def build_shed(w, d):
    vis, geo = Mesh(), []
    ash_bed(vis, w * 1.35, d * 1.35, lambda u, v: 0.02 + 0.05 * random.random() * max(0, 1 - u * u - v * v), 1)
    ix, iy = w / 2 - 0.1, d / 2 - 0.1
    for sx in (-1, 1):
        for sy in (-1, 1):
            post(vis, geo, sx * ix, sy * iy, random.uniform(0.3, 1.7), 0.14, 0)
    # what's left of two walls: a run of upright plank stubs along each
    for side in random.sample(range(4), 2):
        along_x = side < 2
        span = w if along_x else d
        fixed = (iy if side == 0 else -iy) if along_x else (ix if side == 2 else -ix)
        a = -span / 2 + 0.2
        while a < span / 2 - 0.2:
            if random.random() < 0.7:
                h = random.uniform(0.15, 1.1)
                x, y = (a, fixed) if along_x else (fixed, a)
                rot = Euler((random.uniform(-0.1, 0.1), random.uniform(-0.1, 0.1), 0 if along_x else math.pi / 2))
                vis.box(Vector((x, y, h / 2)), (0.16, 0.03, h), rot, 0)
            a += 0.18
    # two charred beams fallen across
    for _ in range(2):
        length = max(w, d) * random.uniform(0.6, 0.85)
        c = Vector((random.uniform(-w, w) * 0.15, random.uniform(-d, d) * 0.15, 0.09))
        rot = Euler((0, random.uniform(-0.05, 0.05), random.uniform(0, math.pi)))
        vis.box(c, (length, 0.13, 0.13), rot, 0)
        geo.append((c, (length, 0.13, 0.13), rot))
    plank_litter(vis, w, d, int(w * d * 0.9), 0)
    return vis, geo


def build_straw(w, d):
    """A long low mound of grey ash with black, charred straw showing through, and a few
    charred stakes from the frame it was stacked on"""
    vis, geo = Mesh(), []
    noise = [[random.uniform(-1, 1) for _ in range(41)] for _ in range(41)]

    def height(u, v):
        core = max(0.0, 1 - abs(u) ** 3) * max(0.0, 1 - v * v)
        i, j = int((u + 1) * 20), int((v + 1) * 20)
        return 0.02 + 0.55 * core + 0.08 * core * noise[j][i]

    ash_bed(vis, w * 1.12, d * 1.2, height, 1, n=40)
    for _ in range(10):
        x, y = random.uniform(-w / 2, w / 2) * 0.85, random.uniform(-d / 2, d / 2) * 0.7
        u, v = x / (w * 0.56), y / (d * 0.6)
        h = random.uniform(0.4, 1.0)
        rot = Euler((random.uniform(-0.25, 0.25), random.uniform(-0.25, 0.25), 0))
        vis.box(Vector((x, y, height(u, v) + h / 2 - 0.2)), (0.07, 0.07, h), rot, 0)
    # collision: the mound as a few low convex slabs, so it's knee-high to walk over, not a wall
    for k in range(5):
        x0 = -w / 2 + w * (k + 0.5) / 5
        u = x0 / (w * 0.56)
        h = max(0.1, 0.55 * (1 - abs(u) ** 3) * 0.8)
        geo.append((Vector((x0, 0, h / 2)), (w / 5, d * 0.6, h), Euler((0, 0, 0))))
    return vis, geo


def build_stand(w, d):
    """A deer stand burnt through: three leg stumps and the fourth leg fallen, the platform
    and its cabin come down on the ground, the ladder lying beside"""
    vis, geo = Mesh(), []
    ash_bed(vis, w * 1.6, d * 1.6, lambda u, v: 0.02 + 0.04 * random.random() * max(0, 1 - u * u - v * v), 1)
    ix, iy = w / 2 - 0.15, d / 2 - 0.15
    corners = [(sx * ix, sy * iy) for sx in (-1, 1) for sy in (-1, 1)]
    fallen = random.randrange(4)
    for k, (x, y) in enumerate(corners):
        if k == fallen:
            post(vis, geo, x, y, random.uniform(0.2, 0.4), 0.16, 0)
            # the rest of it, fallen outwards
            length = random.uniform(3.5, 5.5)
            out = Vector((x, y, 0)).normalized()
            ang = math.atan2(out.y, out.x) + random.uniform(-0.4, 0.4)
            c = Vector((x, y, 0.1)) + Vector((math.cos(ang), math.sin(ang), 0)) * (length / 2 + 0.2)
            vis.box(c, (length, 0.16, 0.16), Euler((0, 0, ang)), 0)
        else:
            post(vis, geo, x, y, random.uniform(0.6, 2.4), 0.16, 0, tilt=0.18)
    # the platform, come down flat-ish with some planks of the cabin still on it
    pw, pd = w * 0.95, d * 0.95
    rot = Euler((random.uniform(-0.15, 0.15), random.uniform(-0.15, 0.15), random.uniform(-0.3, 0.3)))
    pc = Vector((random.uniform(-0.3, 0.3), random.uniform(-0.3, 0.3), 0.25))
    for k in range(int(pw / 0.17)):
        off = rot.to_matrix() @ Vector((-pw / 2 + 0.085 + k * 0.17, 0, 0))
        vis.box(pc + off, (0.16, pd, 0.04), rot, 0)
    geo.append((pc, (pw, pd, 0.08), rot))
    for _ in range(6):
        h = random.uniform(0.2, 0.8)
        x, y = random.uniform(-pw, pw) * 0.4, random.uniform(-pd, pd) * 0.4
        vis.box(pc + Vector((x, y, h / 2)), (0.16, 0.03, h), Euler((random.uniform(-0.3, 0.3), 0, random.uniform(0, math.pi))), 0)
    # the ladder, lying beside
    ang = random.uniform(0, math.pi)
    dirv = Vector((math.cos(ang), math.sin(ang), 0))
    side = Vector((-dirv.y, dirv.x, 0))
    base = Vector((ix + 0.8, 0, 0.06)) if random.random() < 0.5 else Vector((0, iy + 0.8, 0.06))
    for s in (-0.25, 0.25):
        vis.box(base + side * s, (3.2, 0.08, 0.08), Euler((0, 0, ang)), 0)
    for r in range(8):
        if random.random() < 0.7:
            vis.box(base + dirv * (-1.4 + r * 0.4), (0.05, 0.5, 0.04), Euler((0, 0, ang)), 0)
    plank_litter(vis, w * 1.3, d * 1.3, 10, 0)
    return vis, geo


def tapered_limb(mesh, base, direction, length, r0, r1, sides=7, jag=0.0):
    """A charred trunk or branch: a tapering prism from base along direction, its far end
    broken off unevenly (jag: how ragged, in metres). Bark runs along it."""
    direction = direction.normalized()
    side = direction.cross(Vector((0, 0, 1)))
    if side.length < 1e-3:
        side = Vector((1, 0, 0))
    side.normalize()
    up = side.cross(direction)
    rings = []
    steps = max(2, int(length / 1.2))
    for i in range(steps + 1):
        t = i / steps
        r = r0 + (r1 - r0) * t
        ring = []
        for k in range(sides):
            a = 2 * math.pi * k / sides
            wob = 1 + random.uniform(-0.12, 0.12)
            along = t * length
            if i == steps:
                along -= random.uniform(0, jag)
            p = base + direction * along + (side * math.cos(a) + up * math.sin(a)) * r * wob
            ring.append(mesh.bm.verts.new(p))
        rings.append(ring)
    u_len = min(CHAR_U, length * 0.3)
    for i in range(steps):
        for k in range(sides):
            k2 = (k + 1) % sides
            u0, u1 = u_len * i / steps, u_len * (i + 1) / steps
            v0, v1 = k / sides, (k + 1) / sides
            mesh.quad((rings[i][k], rings[i][k2], rings[i + 1][k2], rings[i + 1][k]),
                      ((u0, v0), (u0, v1), (u1, v1), (u1, v0)), 0).smooth = True
    # the broken end: end grain; and the base closed too, so the whole limb is a closed
    # solid (an open tube can come out facing inwards - invisible in game)
    cap = list(reversed(rings[-1]))
    mesh.quad(cap, [(0.9 + 0.06 * math.cos(2 * math.pi * k / sides), 0.5 + 0.12 * math.sin(2 * math.pi * k / sides)) for k in range(sides)], 0)
    mesh.quad(list(rings[0]), [(0.9 + 0.06 * math.cos(2 * math.pi * k / sides), 0.5 + 0.12 * math.sin(2 * math.pi * k / sides)) for k in range(sides)], 0)


def build_tree(height, radius):
    """What a tree burns down to: a big black stump, its top burnt ragged and split, roots
    flaring into the ground, a stub or two of a limb near the top"""
    vis, geo = Mesh(), []
    # no ash ring (a flat disc looks wrong on a hillside) - but it's still built and thrown
    # away, so the random numbers the stump is shaped from stay as they were
    scrap = Mesh()
    ash_bed(scrap, 2.2 + radius * 5, 2.2 + radius * 5, lambda u, v: 0.02 + 0.04 * random.random() * max(0, 1 - u * u - v * v), 1, n=12)
    scrap.bm.free()
    lean = Vector((random.uniform(-0.05, 0.05), random.uniform(-0.05, 0.05), 1)).normalized()
    tapered_limb(vis, Vector((0, 0, -0.3)), lean, height + 0.3, radius * 1.3, radius * 0.85, sides=11, jag=height * 0.35)
    # a splinter or two standing proud of the ragged top
    for _ in range(random.randint(1, 3)):
        a = random.uniform(0, 2 * math.pi)
        base = lean * (height * 0.75) + Vector((math.cos(a), math.sin(a), 0)) * radius * 0.55
        tapered_limb(vis, base, lean + Vector((math.cos(a), math.sin(a), 0)) * 0.15, height * random.uniform(0.25, 0.5), radius * 0.28, radius * 0.05, sides=5, jag=0.05)
    # roots flaring into the ground
    for k in range(5):
        a = 2 * math.pi * k / 5 + random.uniform(-0.3, 0.3)
        out = Vector((math.cos(a), math.sin(a), -0.35))
        tapered_limb(vis, Vector((math.cos(a), math.sin(a), 0)) * radius * 0.7 + Vector((0, 0, 0.12)), out, radius * random.uniform(2.0, 3.0), radius * 0.45, radius * 0.12, sides=5, jag=0.02)
    # a limb stub near the top
    for _ in range(random.randint(1, 2)):
        a = random.uniform(0, 2 * math.pi)
        out = Vector((math.cos(a), math.sin(a), random.uniform(0.3, 0.8)))
        root = lean * height * random.uniform(0.55, 0.8) + Vector((math.cos(a), math.sin(a), 0)) * radius * 0.7
        tapered_limb(vis, root, out, random.uniform(0.6, 1.2), radius * 0.35, radius * 0.15, sides=6, jag=0.1)
    geo.append((Vector((0, 0, height / 2)), (radius * 2.3, radius * 2.3, height), Euler((0, 0, 0))))
    return vis, geo


def geometry_object(name, parts):
    """Convex collision components from (centre, size, rot) boxes"""
    bm = bmesh.new()
    groups = []
    for c, (sx, sy, sz), rot in parts:
        m = Matrix.Translation(c) @ rot.to_matrix().to_4x4() @ Matrix.Diagonal((sx, sy, sz, 1))
        res = bmesh.ops.create_cube(bm, size=1.0, matrix=m)
        groups.append(res["verts"])
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bm.verts.index_update()
    idx = [[v.index for v in g] for g in groups]
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    for i, g in enumerate(idx):
        obj.vertex_groups.new(name=f"Component{i + 1:02d}").add(g, 1.0, "REPLACE")
    return obj


def set_lod(obj, lod, resolution=0, properties=()):
    props = obj.a3ob_properties_object
    props.is_a3_lod = True
    props.lod = lod
    props.resolution = resolution
    for name, value in properties:
        prop = props.properties.add()
        prop.name = name
        prop.value = value


def set_uniform_mass(obj, total):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    layer = bm.verts.layers.float.new("a3ob_mass")
    for v in bm.verts:
        v[layer] = total / len(bm.verts)
    bm.to_mesh(obj.data)
    bm.free()


def make_material(name, color, tex, preview_png):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.9
    png = os.path.join(PREVIEW_TEX, preview_png)
    if os.path.exists(png):
        img = nodes.new("ShaderNodeTexImage")
        img.image = bpy.data.images.load(png)
        mat.node_tree.links.new(img.outputs["Color"], bsdf.inputs["Base Color"])
        mat.node_tree.links.new(img.outputs["Alpha"], bsdf.inputs["Alpha"])
        mat.blend_method = "HASHED"
    props = mat.a3ob_properties_material
    props.texture_type = "CUSTOM"
    props.color_raw = tex[0]
    props.material_path = tex[1]
    return mat


def reset_scene():
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.materials, bpy.data.images,
                 bpy.data.cameras, bpy.data.lights, bpy.data.collections):
        for block in list(coll):
            coll.remove(block)


def render_preview(objects, path):
    """Every ruin in a row, seen from above at an angle"""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x, scene.render.resolution_y = 1800, 700
    scene.world = bpy.data.worlds.new("sky")
    scene.world.color = (0.55, 0.6, 0.65)
    ground = bpy.data.meshes.new("ground")
    ground.from_pydata([(-200, -200, -0.01), (200, -200, -0.01), (200, 200, -0.01), (-200, 200, -0.01)], [], [(0, 1, 2, 3)])
    g = bpy.data.objects.new("ground", ground)
    gm = bpy.data.materials.new("sand")
    gm.use_nodes = True
    gm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.55, 0.48, 0.36, 1)
    ground.materials.append(gm)
    scene.collection.objects.link(g)
    x = 0.0
    for o in objects:
        w = o.dimensions.x
        o.location.x = x + w / 2
        x += w + 3
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 4
    sun.rotation_euler = (0.7, 0.2, 0.6)
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = x + 2
    cam.location = (x / 2, -40, 30)
    cam.rotation_euler = (math.radians(55), 0, 0)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def main():
    addon_utils.enable(A3OB_MODULE, default_set=False)
    if not hasattr(bpy.ops.a3ob, "export_p3d"):
        raise RuntimeError(f"Arma 3 Object Builder add-on ({A3OB_MODULE}) is not installed")
    reset_scene()
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(RUIN_DIR, exist_ok=True)
    char = make_material("charred", (0.08, 0.07, 0.06), CHAR_TEX, "log01burned_co.png")
    ash = make_material("ash", (0.45, 0.45, 0.45), ASH_TEX, "stoneground_co.png")
    char_hot = make_material("charred_hot", (0.08, 0.07, 0.06), CHAR_HOT_TEX, "log01burned_co.png")
    ash_hot = make_material("ash_hot", (0.45, 0.45, 0.45), ASH_HOT_TEX, "stoneground_co.png")
    builders = {"shed": build_shed, "straw": build_straw, "stand": build_stand}

    cfg = ["class CfgSkeletons", "{", "\tclass Default", "\t{", "\t\tisDiscrete = 1;",
           "\t\tskeletonInherit = \"\";", "\t\tskeletonBones[] = {};", "\t};", "};",
           "class CfgModels", "{", "\tclass Default", "\t{", "\t\tsectionsInherit = \"\";",
           "\t\tsections[] = {};", "\t\tskeletonName = \"\";", "\t};"]
    visuals = []
    jobs = [(f"{m}_burnt", kind, w, d) for m, (kind, w, d) in FOOTPRINTS.items()]
    jobs += [(f"{m}_burnt", "tree", h, r) for m, (h, r) in TREES.items()]
    builders["tree"] = build_tree
    for name, kind, w, d in jobs:
        random.seed(name[:-len("_burnt")])   # the original model's name: keeps each ruin as it was
        vis_mesh, parts = builders[kind](w, d)
        coll = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(coll)
        visual = vis_mesh.to_object(f"{name}_LOD0", [char, ash])
        set_lod(visual, LOD_VISUAL, resolution=1)
        geometry = geometry_object(f"{name}_Geometry", parts)
        set_uniform_mass(geometry, MASS_KG)
        set_lod(geometry, LOD_GEOMETRY, properties=(("autocenter", "0"),))
        fire = geometry_object(f"{name}_FireGeometry", parts)
        set_lod(fire, LOD_FIRE_GEOMETRY)
        for o in (visual, geometry, fire):
            coll.objects.link(o)
        geometry.display_type = fire.display_type = "WIRE"
        # export just this ruin's LODs
        for o in bpy.context.scene.objects:
            o.select_set(o in (visual, geometry, fire))
        for suffix, mats in (("_burnt", (char, ash)), ("_hot", (char_hot, ash_hot))):
            visual.data.materials[0] = mats[0]
            visual.data.materials[1] = mats[1]
            bpy.ops.a3ob.export_p3d(
                filepath=os.path.join(RUIN_DIR, name[:-len("_burnt")] + suffix + ".p3d"),
                use_selection=True,
                validate_lods=True,
                validate_lods_warning_errors=False,
                lod_collisions="FAIL",
            )
        visual.data.materials[0] = char
        visual.data.materials[1] = ash
        tris = sum(len(p.vertices) - 2 for p in visual.data.polygons)
        print(f"[ruins] {name}: {w} x {d} m, {tris} tris, {len(parts)} collision parts")
        cfg.append(f"\tclass {name}: Default {{}};")
        cfg.append(f"\tclass {name[:-len('_burnt')]}_hot: Default {{}};")
        geometry.hide_render = fire.hide_render = True
        visuals.append(visual)
    cfg.append("};")
    with open(os.path.join(RUIN_DIR, "model.cfg"), "w") as f:
        f.write("\n".join(cfg) + "\n")

    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, "ruins.blend"))
    render_preview(visuals, os.path.join(OUT_DIR, "ruins_preview.png"))


main()
