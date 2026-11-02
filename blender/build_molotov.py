"""Procedurally builds the molotov cocktail model: a vodka bottle with a rag knotted round
its neck, plugging the mouth, its tail hanging over the shoulder.

Run headless (from the project root):
    blender --background --python blender/build_molotov.py

Outputs:
    models/molotov.blend         - source scene, one collection per DayZ LOD
    Molotov/molotov.p3d          - MLOD P3D (needs the Arma 3 Object Builder add-on)
    Molotov/model.cfg
    models/molotov_preview.png   - textured only if MOLOTOV_PREVIEW_TEX points at a folder
                                   holding vodkabottles_ca.png and molotov_rag_co.png
                                   (tools/make_textures.py leaves them in build/vanilla_png)

Only vanilla textures and materials are used, referenced by their in-game paths:
the bottle is UV-mapped onto dz\\gear\\drinks\\data\\vodkabottles_ca.paa (main label on
the body, plain glass on the shoulder and neck, the base circle underneath), the rag onto
our petrol-soaked copy of the vanilla rag texture, and the fuel inside is a procedural colour.

Size and origin copy the vanilla GlassBottle (measured in game: 0.28 m tall from -0.138
to +0.142 about its origin, body radius 0.036), so it sits in hands and the inventory
like one. Axes: Blender X/Y/Z = game X/Z/Y (Z up here, Y up in game).
"""

import math
import os
import random

import addon_utils
import bmesh
import bpy
from mathutils import Vector

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "models")
ADDON_DIR = os.path.join(ROOT, "Molotov")    # becomes the Molotov PBO (prefix "Molotov")
PREVIEW_TEX = os.environ.get("MOLOTOV_PREVIEW_TEX", "")

GLASS_TEX = ("dz\\gear\\drinks\\data\\vodkabottles_ca.paa", "dz\\gear\\drinks\\data\\vodkabottles.rvmat")
# The rag: a petrol-soaked version of the vanilla rag (tools/make_textures.py)
RAG_TEX = ("Molotov\\data\\molotov_rag_co.paa", "Molotov\\data\\molotov_rag.rvmat")
# The fuel inside: a procedural petrol-amber colour, shiny like the glass. The vanilla
# glass is semi-transparent (alpha ~90 of 255), so it shows through.
FUEL_TEX = ("#(argb,8,8,3)color(0.72,0.38,0.04,1,CO)", GLASS_TEX[1])
FILL = -0.055            # fuel level: a bit under half of the body

SEGMENTS = 20
BOTTOM, TOP = -0.1376, 0.142
BODY_R = 0.036
BODY_TOP = 0.03          # where the shoulder starts
NECK_R = 0.0130
MASS_KG = 0.9

# Bottle profile, bottom to top: (radius, height, uv section)
BOTTLE = [
    (0.000, BOTTOM, "base"),
    (0.031, BOTTOM, "base"),
    (0.035, BOTTOM + 0.0025, "edge"),
    (BODY_R, BOTTOM + 0.006, "body"),
    (BODY_R, BODY_TOP, "body"),
    (0.0345, BODY_TOP + 0.012, "neck"),
    (0.029, BODY_TOP + 0.026, "neck"),
    (0.020, BODY_TOP + 0.040, "neck"),
    (0.0150, BODY_TOP + 0.052, "neck"),
    (NECK_R, BODY_TOP + 0.064, "neck"),
    (NECK_R, TOP - 0.006, "neck"),
    (0.0148, TOP - 0.004, "neck"),
    (0.0148, TOP, "neck"),
    (0.000, TOP, "neck"),
]

# Texture regions on vodkabottles_ca (Blender UV: u right, v up from the bottom)
BODY_U = (0.31, 0.94)            # plain glass then the main label, once round the body
BODY_V = (0.005, 0.595)
NECK_U = (0.02, 0.48)            # plain glass, top-left of the sheet
NECK_V = (0.62, 0.98)
BASE_C, BASE_RADIUS = (0.625, 0.707), 0.10
# The main label (u ~0.78) faces the inventory camera (vanilla invView, from -X +Y here)
LABEL_ANGLE = math.atan2(0.296, -0.289)
ANGLE0 = LABEL_ANGLE - 2 * math.pi * (0.72 - BODY_U[0]) / (BODY_U[1] - BODY_U[0])
TAIL_ANGLE = LABEL_ANGLE - math.radians(75)

# Rag texture regions on rag_co
RAG_BAND_UV = ((0.02, 0.60), (0.42, 0.78))   # (u range, v range): the long horizontal folds
RAG_PLUG_UV = ((0.05, 0.45), (0.05, 0.38))   # folds, bottom left
RAG_TAIL_UV = ((0.745, 0.865), (0.03, 0.75)) # the long vertical strip

A3OB_MODULE = "bl_ext.user_default.Arma3ObjectBuilder"
LOD_VISUAL, LOD_GEOMETRY, LOD_MEMORY, LOD_FIRE_GEOMETRY = "0", "6", "9", "15"

MODEL_CFG = '''class CfgSkeletons
{
	class Default
	{
		isDiscrete = 1;
		skeletonInherit = "";
		skeletonBones[] = {};
	};
};
class CfgModels
{
	class Default
	{
		sectionsInherit = "";
		sections[] = {};
		skeletonName = "";
	};
	class molotov: Default {};
};
'''


def reset_scene():
    # Clear by hand: read_factory_settings would also unload the Object Builder add-on
    for coll in (bpy.data.objects, bpy.data.meshes, bpy.data.materials, bpy.data.images,
                 bpy.data.cameras, bpy.data.lights, bpy.data.collections):
        for block in list(coll):
            coll.remove(block)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0


def make_material(name, color, roughness, tex, preview_png, see_through=False):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    png = os.path.join(PREVIEW_TEX, preview_png) if PREVIEW_TEX and preview_png else ""
    if png and os.path.exists(png):
        img = nodes.new("ShaderNodeTexImage")
        img.image = bpy.data.images.load(png)
        mat.node_tree.links.new(img.outputs["Color"], bsdf.inputs["Base Color"])
        if see_through:   # preview only: let the fuel show through the glass
            mat.node_tree.links.new(img.outputs["Alpha"], bsdf.inputs["Alpha"])
            mat.blend_method = "BLEND"
    props = mat.a3ob_properties_material
    props.texture_type = "CUSTOM"   # raw in-game path, written to the P3D as-is
    props.color_raw = tex[0]
    props.material_path = tex[1]
    return mat


def lerp(a, b, t):
    return a + (b - a) * t


class Lathe:
    """Surfaces of revolution and strips in one bmesh, UV'd face corner by face corner
    (so the seam needs no duplicate vertices), tagged with a material per face."""

    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new("UVMap")

    def face(self, verts, uvs, mat):
        f = self.bm.faces.new(verts)
        f.material_index = mat
        f.smooth = True
        for loop, uv in zip(f.loops, uvs):
            loop[self.uv].uv = uv
        return f

    def revolve(self, profile, mat, uv_of, angle0=0.0, wobble=None, segments=SEGMENTS):
        """profile: [(radius, z), ...] bottom to top; radius 0 closes a pole.
        uv_of(ring_index, k_fraction) -> (u, v); wobble(ring, k) scales the radius."""
        rings = []
        for j, (r, z) in enumerate(profile):
            if r == 0:
                rings.append(self.bm.verts.new((0, 0, z)))
                continue
            ring = []
            for k in range(segments):
                a = angle0 + 2 * math.pi * k / segments
                rr = r * (wobble(j, k) if wobble else 1.0)
                ring.append(self.bm.verts.new((rr * math.cos(a), rr * math.sin(a), z)))
            rings.append(ring)
        for j in range(len(rings) - 1):
            lo, hi = rings[j], rings[j + 1]
            for k in range(segments):
                k2 = (k + 1) % segments
                f0, f1 = k / segments, (k + 1) / segments
                if isinstance(lo, list) and isinstance(hi, list):
                    self.face((lo[k], lo[k2], hi[k2], hi[k]),
                              (uv_of(j, f0), uv_of(j, f1), uv_of(j + 1, f1), uv_of(j + 1, f0)), mat)
                elif isinstance(hi, list):      # bottom pole: fan up to the ring
                    self.face((lo, hi[k2], hi[k]), (uv_of(j, (f0 + f1) / 2), uv_of(j + 1, f1), uv_of(j + 1, f0)), mat)
                else:                           # top pole
                    self.face((lo[k], lo[k2], hi), (uv_of(j, f0), uv_of(j, f1), uv_of(j + 1, (f0 + f1) / 2)), mat)

    def strip(self, path, width, thickness, mat, uv_rect):
        """A flat strip of cloth along path [(Vector centre, Vector across, Vector out)],
        given some thickness so it reads from both sides."""
        n = len(path)
        (u0, u1), (v0, v1) = uv_rect
        layers = []
        for side in (+0.5, -0.5):
            row = []
            for i, (c, across, out, w) in enumerate(path):
                base = c + out * (side * thickness)
                row.append((self.bm.verts.new(base - across * (width * w / 2)),
                            self.bm.verts.new(base + across * (width * w / 2))))
            layers.append(row)
        front, back = layers
        for i in range(n - 1):
            va, vb = lerp(v1, v0, i / (n - 1)), lerp(v1, v0, (i + 1) / (n - 1))
            self.face((front[i][0], front[i][1], front[i + 1][1], front[i + 1][0]),
                      ((u0, va), (u1, va), (u1, vb), (u0, vb)), mat)
            self.face((back[i][1], back[i][0], back[i + 1][0], back[i + 1][1]),
                      ((u0, va), (u1, va), (u1, vb), (u0, vb)), mat)
            for e in (0, 1):   # the two thin edges
                a, b = (0, 1) if e == 0 else (1, 0)
                self.face((front[i][e], back[i][e], back[i + 1][e], front[i + 1][e]),
                          ((u0, va), (u0 + 0.01, va), (u0 + 0.01, vb), (u0, vb)), mat)
        # end caps
        for i, flip in ((0, False), (n - 1, True)):
            q = (front[i][0], front[i][1], back[i][1], back[i][0])
            if flip:
                q = tuple(reversed(q))
            self.face(q, ((u0, v0), (u1, v0), (u1, v0 + 0.01), (u0, v0 + 0.01)), mat)

    def to_object(self, name, materials):
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces[:])
        mesh = bpy.data.meshes.new(name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        obj = bpy.data.objects.new(name, mesh)
        for m in materials:
            mesh.materials.append(m)
        return obj


def bottle_uv(j, f):
    r, z, section = BOTTLE[j]
    if section == "base":
        a = ANGLE0 + 2 * math.pi * f
        rr = BASE_RADIUS * (r / 0.031)
        return (BASE_C[0] + rr * math.cos(a), BASE_C[1] + rr * math.sin(a))
    if section == "body" or (section == "edge"):
        t = (z - (BOTTOM + 0.006)) / (BODY_TOP - (BOTTOM + 0.006))
        return (lerp(*BODY_U, f), lerp(*BODY_V, max(0.0, t)))
    t = (z - BODY_TOP) / (TOP - BODY_TOP)
    return (lerp(*NECK_U, f), lerp(*NECK_V, t))


def build_visual(glass, rag, fuel):
    rnd = random.Random(7)
    b = Lathe()
    b.revolve([(r, z) for r, z, _ in BOTTLE], 0, bottle_uv, angle0=ANGLE0)

    # The fuel, just inside the glass up to the fill line
    inner = BODY_R - 0.0022
    fuel_profile = [(0.0, BOTTOM + 0.004), (inner - 0.003, BOTTOM + 0.004), (inner, BOTTOM + 0.008),
                    (inner, FILL), (0.0, FILL)]
    b.revolve(fuel_profile, 2, lambda j, f: (lerp(*NECK_U, f), lerp(*NECK_V, j / 4)), angle0=ANGLE0)

    # The band: lumpy cloth knotted round the neck
    band = [(NECK_R - 0.0002, 0.094), (0.0175, 0.098), (0.0200, 0.106), (0.0205, 0.114),
            (0.0185, 0.121), (NECK_R - 0.0002, 0.126)]
    lumps = [[rnd.uniform(0.9, 1.12) for _ in range(SEGMENTS)] for _ in band]
    (bu0, bu1), (bv0, bv1) = RAG_BAND_UV
    b.revolve(band, 1, lambda j, f: (lerp(bu0, bu1, f), lerp(bv0, bv1, j / (len(band) - 1))),
              angle0=0.3, wobble=lambda j, k: 1.0 if j in (0, len(band) - 1) else lumps[j][k])

    # The plug: a crumpled wad stuffed in the mouth, poking out of the top
    plug = [(0.0120, TOP - 0.010), (0.0140, TOP + 0.004), (0.0155, TOP + 0.014),
            (0.0135, TOP + 0.024), (0.0085, TOP + 0.031), (0.0, TOP + 0.034)]
    plumps = [[rnd.uniform(0.9, 1.1) for _ in range(SEGMENTS)] for _ in plug]
    (pu0, pu1), (pv0, pv1) = RAG_PLUG_UV
    b.revolve(plug, 1, lambda j, f: (lerp(pu0, pu1, f), lerp(pv0, pv1, j / (len(plug) - 1))),
              angle0=1.1, wobble=lambda j, k: plumps[j][k])

    # The tail: hangs from the knot over the shoulder and down the body
    radial = Vector((math.cos(TAIL_ANGLE), math.sin(TAIL_ANGLE), 0))
    tangent = Vector((-radial.y, radial.x, 0))
    pts = [(0.0215, 0.112, 0.8), (0.0225, 0.098, 1.0), (0.0255, 0.084, 1.05), (0.0315, 0.070, 1.05),
           (0.0380, 0.054, 1.0), (0.0398, 0.038, 0.95), (0.0402, 0.020, 0.9), (0.0402, 0.004, 0.85),
           (0.0405, -0.010, 0.75), (0.0425, -0.020, 0.6)]
    path = []
    for i, (r, z, w) in enumerate(pts):
        sway = 0.004 * math.sin(i * 1.3)
        centre = radial * r + tangent * sway + Vector((0, 0, z))
        path.append((centre, tangent, radial, w))
    b.strip(path, 0.030, 0.0025, 1, RAG_TAIL_UV)
    return b.to_object("Molotov_LOD0", [glass, rag, fuel])


def build_geometry(name):
    """Convex collision: the body, then the shoulder and neck up to the plug."""
    hull = [((BODY_R, BOTTOM), (BODY_R, BODY_TOP)), ((BODY_R, BODY_TOP), (0.016, TOP + 0.034))]
    bm = bmesh.new()
    groups = []
    for (r0, z0), (r1, z1) in hull:
        res = bmesh.ops.create_cone(bm, cap_ends=True, segments=8, radius1=r0, radius2=r1,
                                    depth=z1 - z0, matrix=__import__("mathutils").Matrix.Translation((0, 0, (z0 + z1) / 2)))
        groups.append(res["verts"])
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bm.verts.index_update()
    idx = [[v.index for v in g] for g in groups]
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    for i, g in enumerate(idx):
        obj.vertex_groups.new(name=f"Component0{i + 1}").add(g, 1.0, "REPLACE")
    return obj


def build_memory():
    points = {
        "invView": (-0.289, 0.296, 0.1195),   # the vanilla bottle's inventory camera
        # What the inventory icon frames: snug round the bottle, rag and tail. Without
        # these the icon frames every point in the model, invView included, and the
        # bottle comes out tiny.
        "boundingbox_min": (-0.046, -0.046, BOTTOM - 0.002),
        "boundingbox_max": (0.046, 0.046, TOP + 0.036),
        "ce_center": (0, 0, 0),
        "ce_radius": (0, 0, 0.15),
        "rag_tip": (0, 0, TOP + 0.030),        # where the flame burns
    }
    mesh = bpy.data.meshes.new("Molotov_Memory")
    mesh.from_pydata(list(points.values()), [], [])
    obj = bpy.data.objects.new("Molotov_Memory", mesh)
    for i, name in enumerate(points):
        obj.vertex_groups.new(name=name).add([i], 1.0, "REPLACE")
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


def new_collection(name):
    coll = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(coll)
    return coll


def render_preview(path):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 700
    scene.render.resolution_y = 1000
    world = bpy.data.worlds.new("World")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.55, 0.6, 0.65, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.9
    scene.world = world
    preview = new_collection("Preview")
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(50), 0, math.radians(-40))
    preview.objects.link(sun)
    cam = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
    cam.location = Vector((-0.289, 0.296, 0.1195)) * 1.45   # where the inventory looks from
    direction = Vector((0, 0, 0.02)) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = 60
    preview.objects.link(cam)
    scene.camera = cam
    for c in bpy.data.collections:
        if c.name not in ("LOD0_Visual", "Preview"):
            c.hide_render = True
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def render_workshop(path, visual):
    """The Workshop preview: the bottle leaning over, the rag alight (a stand-in flame
    for the picture only; in game it's the vanilla torch flame), against a dusky sky."""
    scene = bpy.context.scene
    scene.render.resolution_x = 960
    scene.render.resolution_y = 600
    scene.view_settings.view_transform = "Standard"   # keep the flame orange, not washed out
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.10, 0.11, 0.16, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    preview = bpy.data.collections["Preview"]
    # Lean the bottle across the picture: roll it about the line the camera looks along
    view = Vector((-0.289, 0.296, 0)).normalized()
    visual.rotation_mode = "AXIS_ANGLE"
    visual.rotation_axis_angle = (math.radians(-24), view.x, view.y, view.z)
    bpy.context.view_layer.update()
    tip = visual.matrix_world @ Vector((0, 0, TOP + 0.032))

    # Flames rise: a few tapering tongues straight up from the rag, orange outside, a
    # yellow core, a glowing base
    Matrix = __import__("mathutils").Matrix
    tongues = [((0.0, 0.0), 0.017, 0.085, (1.0, 0.28, 0.02), 1.6),
               ((0.006, 0.004), 0.010, 0.060, (1.0, 0.40, 0.04), 1.8),
               ((-0.006, -0.003), 0.009, 0.050, (1.0, 0.36, 0.03), 1.8),
               ((0.0, 0.0), 0.008, 0.045, (1.0, 0.75, 0.25), 2.2)]
    for i, ((dx, dy), r, h, color, strength) in enumerate(tongues):
        fmesh = bpy.data.meshes.new(f"Flame{i}")
        fbm = bmesh.new()
        bmesh.ops.create_cone(fbm, cap_ends=True, segments=16, radius1=r, radius2=0.0,
                              depth=h, matrix=Matrix.Translation((dx, dy, h / 2)))
        bmesh.ops.create_uvsphere(fbm, u_segments=12, v_segments=8, radius=r * 0.98,
                                  matrix=Matrix.Translation((dx, dy, 0)))
        fbm.to_mesh(fmesh)
        fbm.free()
        flame = bpy.data.objects.new(f"Flame{i}", fmesh)
        flame.location = tip
        fmat = bpy.data.materials.new(f"flame{i}")
        fmat.use_nodes = True
        em = fmat.node_tree.nodes.new("ShaderNodeEmission")
        em.inputs["Color"].default_value = (*color, 1)
        em.inputs["Strength"].default_value = strength
        fmat.node_tree.links.new(em.outputs["Emission"], fmat.node_tree.nodes["Material Output"].inputs["Surface"])
        fmesh.materials.append(fmat)
        preview.objects.link(flame)
    glow = bpy.data.objects.new("Glow", bpy.data.lights.new("Glow", "POINT"))
    glow.data.energy = 6
    glow.data.color = (1.0, 0.55, 0.2)
    glow.location = tip + Vector((0, 0, 0.04))
    preview.objects.link(glow)

    cam = scene.camera
    centre = visual.matrix_world @ Vector((0, 0, 0.06))
    cam.location = centre + view * 1.0 + Vector((0, 0, 0.07))
    direction = centre - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = 55
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def main():
    addon_utils.enable(A3OB_MODULE, default_set=False)
    if not hasattr(bpy.ops.a3ob, "export_p3d"):
        raise RuntimeError(f"Arma 3 Object Builder add-on ({A3OB_MODULE}) is not installed")
    reset_scene()
    os.makedirs(OUT_DIR, exist_ok=True)

    glass = make_material("glass", (0.8, 0.85, 0.85), 0.15, GLASS_TEX, "vodkabottles_ca.png", see_through=True)
    rag = make_material("rag", (0.45, 0.33, 0.18), 0.55, RAG_TEX, "molotov_rag_co.png")
    fuel = make_material("fuel", (0.72, 0.38, 0.04), 0.1, FUEL_TEX, "")

    lod0 = new_collection("LOD0_Visual")
    visual = build_visual(glass, rag, fuel)
    lod0.objects.link(visual)
    set_lod(visual, LOD_VISUAL, resolution=1)

    geo = new_collection("Geometry")
    geometry = build_geometry("Molotov_Geometry")
    geo.objects.link(geometry)
    set_uniform_mass(geometry, MASS_KG)
    set_lod(geometry, LOD_GEOMETRY, properties=(("autocenter", "0"),))
    fire = new_collection("FireGeometry")
    fire_geo = build_geometry("Molotov_FireGeometry")
    fire.objects.link(fire_geo)
    set_lod(fire_geo, LOD_FIRE_GEOMETRY)
    mem = new_collection("Memory")
    memory = build_memory()
    mem.objects.link(memory)
    set_lod(memory, LOD_MEMORY)
    for c in (geo, fire):
        for o in c.objects:
            o.display_type = "WIRE"

    tris = sum(len(p.vertices) - 2 for p in visual.data.polygons)
    print(f"[molotov] visual LOD: {len(visual.data.vertices)} verts, {tris} tris")

    os.makedirs(ADDON_DIR, exist_ok=True)
    with open(os.path.join(ADDON_DIR, "model.cfg"), "w") as f:
        f.write(MODEL_CFG)
    bpy.ops.a3ob.export_p3d(
        filepath=os.path.join(ADDON_DIR, "molotov.p3d"),
        use_selection=False,
        validate_lods=True,
        validate_lods_warning_errors=False,
        lod_collisions="FAIL",
    )

    bpy.context.preferences.filepaths.save_version = 0   # no .blend1 backups
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, "molotov.blend"))
    render_preview(os.path.join(OUT_DIR, "molotov_preview.png"))
    # Workshop preview (preview.png beside the mod, kept out of the PBO by build_mod.py)
    render_workshop(os.path.join(ADDON_DIR, "preview.png"), visual)


main()
