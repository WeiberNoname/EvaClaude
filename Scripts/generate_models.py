"""Authored faceted armor profiles; centimetres, centred around the origin."""
from pathlib import Path
import math
out=Path(__file__).parent/'GeneratedModels';out.mkdir(exist_ok=True)
# Eight corners produce broad panels with small bevels rather than cylinders.
corners=[(1,.55),(.55,1),(-.55,1),(-1,.55),(-1,-.55),(-.55,-1),(.55,-1),(1,-.55)]
def profile(name,rings):
    vertices=[(cx+x*w,cy+y*d,z) for z,w,d,cx,cy in rings for x,y in corners]
    faces=[]
    for j in range(len(rings)-1):
        for i in range(8):
            a=j*8+i;b=j*8+(i+1)%8;c=b+8;d=a+8
            faces.extend([(a,b,c),(a,c,d)])
    for i in range(1,7):faces.extend([(0,i+1,i),((len(rings)-1)*8,(len(rings)-1)*8+i,(len(rings)-1)*8+i+1)])
    text=['# EVA authored armor','o '+name,'s off']
    text += [f'v {x:.4f} {y:.4f} {z:.4f}' for x,y,z in vertices]
    text += ['vt 0.5 1', 'vt 0 0', 'vt 1 0']
    text += ['f '+' '.join(f'{v+1}/{i+1}' for i,v in enumerate(face)) for face in faces]
    (out/(name+'.obj')).write_text('\n'.join(text)+'\n')
profile('ArmorTorso',[(-50,23,30,-5,0),(-18,34,44,0,0),(25,42,50,0,0),(43,30,43,0,0),(50,15,22,0,0)])
profile('ArmorPlate',[(-50,25,32,0,0),(-25,46,42,0,0),(28,50,50,0,0),(50,28,34,-6,0)])
profile('ArmorShin',[(-50,23,33,8,0),(-32,30,36,4,0),(28,40,45,0,0),(50,30,36,0,0)])
profile('ArmorHead',[(-50,14,15,12,0),(-25,33,29,14,0),(5,45,44,0,0),(36,28,39,-5,0),(50,14,25,-9,0)])
profile('ArmorPylon',[(-50,33,32,0,0),(-15,38,50,0,0),(28,28,43,-5,0),(50,5,12,-18,0)])
profile('ArmorHorn',[(-50,34,33,0,0),(-10,22,20,8,0),(50,.7,.7,28,0)])
profile('ArmorFoot',[(-50,48,42,0,0),(-20,50,43,0,0),(25,33,34,-17,0),(50,15,27,-24,0)])
profile('AngelMask',[(-50,11,5,0,0),(-20,38,13,0,0),(10,50,18,0,0),(39,36,14,0,0),(50,14,7,0,0)])
# Ramiel's canonical octahedral silhouette, split into eight flat triangular faces.
points=[(0,0,70),(50,0,0),(0,50,0),(-50,0,0),(0,-50,0),(0,0,-70)]
triangles=[(1,2,3),(1,3,4),(1,4,5),(1,5,2),(6,3,2),(6,4,3),(6,5,4),(6,2,5)]
(out/'RamielCrystal.obj').write_text('o RamielCrystal\ns off\n'+'\n'.join(f'v {x} {y} {z}' for x,y,z in points)+'\nvt 0.5 1\nvt 0 0\nvt 1 0\n'+'\n'.join('f '+' '.join(f'{v}/{i+1}' for i,v in enumerate(t)) for t in triangles)+'\n')
print('Generated eight armor meshes and the Ramiel crystal.')

# Dedicated anatomical armor sections; front is +X, lateral is Y, height is Z.
profile('ArmorChest08',[(-50,12,18,8,0),(-20,30,37,0,0),(15,32,50,0,0),(42,23,46,-3,0),(50,12,30,-8,0)])
profile('ArmorWaist08',[(-50,23,32,0,0),(-15,30,30,-4,0),(20,30,35,-6,0),(50,22,42,-6,0)])
profile('ArmorPelvis08',[(-50,14,16,12,0),(-20,30,37,0,0),(22,35,50,0,0),(50,24,42,0,0)])
profile('ArmorThigh08',[(-50,24,26,4,0),(-30,30,30,0,0),(10,37,43,-4,0),(35,32,45,-6,0),(50,24,30,0,0)])
profile('ArmorCalf08',[(-50,15,22,2,0),(-22,21,25,-9,0),(13,33,35,-14,0),(36,30,29,0,0),(50,23,28,7,0)])
profile('ArmorForearm08',[(-50,18,24,3,0),(-25,24,25,4,0),(12,33,32,-3,0),(35,29,29,-5,0),(50,19,22,0,0)])
profile('ArmorBoot08',[(-50,48,43,18,0),(-25,50,45,20,0),(0,44,37,10,0),(25,25,30,-10,0),(50,18,23,-15,0)])
profile('ArmorPylon08',[(-50,30,33,3,0),(-30,38,49,0,0),(5,23,39,-12,0),(50,17,31,-28,0)])
profile('ArmorHelmet01',[(-50,17,18,19,0),(-25,35,31,17,0),(0,43,37,3,0),(25,37,38,-8,0),(43,24,28,-14,0),(50,7,10,-15,0)])
profile('ArmorHelmet02',[(-50,16,20,14,0),(-28,35,32,16,0),(0,40,43,5,0),(30,34,38,-6,0),(50,17,20,-8,0)])
profile('ArmorShoulder02',[(-50,9,24,8,0),(-15,42,40,0,0),(25,50,50,-8,0),(50,34,43,-4,0)])
print('Generated eleven reference-shaped Unit-01 / Unit-02 armor sections.')

# An authored ridgeline replaces the former row of spheres behind the western district.
nx, ny = 16, 100
ridge = ['o TerrainRidge', 's 1']
for iy in range(ny + 1):
    v = iy / ny
    for ix in range(nx + 1):
        u = ix / nx
        peak = 17 + 14 * math.sin(v * 19 + .5)**2 + 12 * math.sin(v * 47)**2
        z = math.sin(math.pi*u)**.85 * peak
        z += math.sin(math.pi*u) * math.sin(v*101+u*31) * 2.8
        ridge.append(f'v {(u-.5)*100:.4f} {(v-.5)*100:.4f} {z:.4f}')
for iy in range(ny + 1):
    for ix in range(nx + 1):
        ridge.append(f'vt {ix/nx:.5f} {iy/ny:.5f}')
for iy in range(ny):
    for ix in range(nx):
        a = iy*(nx+1)+ix+1
        b, c, d = a+1, a+nx+2, a+nx+1
        ridge.extend([f'f {a}/{a} {b}/{b} {c}/{c}', f'f {a}/{a} {c}/{c} {d}/{d}'])
(out/'TerrainRidge.obj').write_text('\n'.join(ridge)+'\n')
print('Generated the western terrain ridgeline.')

# City kit and destruction meshes. Unit scale matches the engine cube (±50), so C++ scales read as metres/100.
import random

def write_mesh(name, vertices, faces, reference, smooth=False):
    """Writes counter-clockwise faces; reference(centroid) gives the outward direction for each face."""
    oriented = []
    for face in faces:
        a, b, c = (vertices[i] for i in face[:3])
        u = [b[k]-a[k] for k in range(3)]
        w = [c[k]-a[k] for k in range(3)]
        normal = (u[1]*w[2]-u[2]*w[1], u[2]*w[0]-u[0]*w[2], u[0]*w[1]-u[1]*w[0])
        centre = [sum(vertices[i][k] for i in face)/len(face) for k in range(3)]
        outward = reference(centre)
        oriented.append(face if sum(normal[k]*outward[k] for k in range(3)) >= 0 else face[::-1])
    text = ['# EVA city kit', 'o '+name, 's 1' if smooth else 's off']
    text += [f'v {x:.4f} {y:.4f} {z:.4f}' for x, y, z in vertices]
    text += ['vt 0.5 1', 'vt 0 0', 'vt 1 0']
    text += ['f '+' '.join(f'{v+1}/{i%3+1}' for i, v in enumerate(face)) for face in oriented]
    (out/(name+'.obj')).write_text('\n'.join(text)+'\n')

def radial(centre):
    return centre

city = random.Random(3017)

# Irregular concrete chunk: a perturbed icosahedron, flattened so piles settle naturally.
phi = (1+5**.5)/2
ico = [(-1,phi,0),(1,phi,0),(-1,-phi,0),(1,-phi,0),(0,-1,phi),(0,1,phi),(0,-1,-phi),(0,1,-phi),(phi,0,-1),(phi,0,1),(-phi,0,-1),(-phi,0,1)]
ico_faces = [(0,11,5),(0,5,1),(0,1,7),(0,7,10),(0,10,11),(1,5,9),(5,11,4),(11,10,2),(10,7,6),(7,1,8),(3,9,4),(3,4,2),(3,2,6),(3,6,8),(3,8,9),(4,9,5),(2,4,11),(6,2,10),(8,6,7),(9,8,1)]
chunk = []
for x, y, z in ico:
    length = (x*x+y*y+z*z)**.5
    r = 50*city.uniform(.62, 1.0)/length
    chunk.append((x*r, y*r*city.uniform(.75, 1.0), z*r*.72))
write_mesh('DebrisChunk', chunk, ico_faces, radial)

# Broken floor slab: an irregular star-shaped outline extruded to a thin plate.
outline = []
for i in range(8):
    angle = i/8*2*math.pi + city.uniform(-.22, .22)
    r = 50*city.uniform(.55, 1.0)
    outline.append((math.cos(angle)*r, math.sin(angle)*r))
slab = [(0, 0, 9), (0, 0, -9)] + [(x, y, 9) for x, y in outline] + [(x, y, -9) for x, y in outline]
slab_faces = []
for i in range(8):
    j = (i+1) % 8
    slab_faces += [(0, 2+i, 2+j), (1, 10+j, 10+i), (2+i, 10+i, 10+j, 2+j)]
write_mesh('SlabShard', slab, slab_faces, lambda c: (c[0], c[1], c[2]*20))

# Pitched roof prism (ridge along Y) and a sawtooth prism with its glazed face toward +X.
def prism(name, profile2d):
    vertices = [(x, -50, z) for x, z in profile2d] + [(x, 50, z) for x, z in profile2d]
    n = len(profile2d)
    faces = [tuple(range(n)), tuple(range(n, 2*n))]
    faces += [(i, (i+1) % n, n+(i+1) % n, n+i) for i in range(n)]
    cx = sum(x for x, _ in profile2d)/n
    cz = sum(z for _, z in profile2d)/n
    write_mesh(name, vertices, faces, lambda c: (c[0]-cx, c[1], c[2]-cz))
prism('RoofGable', [(-50, -50), (50, -50), (0, 50)])
prism('RoofSaw', [(-50, -50), (50, -50), (50, 50)])

# Jagged stump left standing after a collapse: a box whose top is torn into uneven spikes.
n = 4
top = {}
stump = []
for ix in range(n+1):
    for iy in range(n+1):
        edge = ix in (0, n) or iy in (0, n)
        height = city.uniform(-20, 50) if not edge else city.uniform(-5, 50)
        top[ix, iy] = len(stump)
        stump.append((-50+100*ix/n, -50+100*iy/n, height))
faces = []
for ix in range(n):
    for iy in range(n):
        faces.append((top[ix, iy], top[ix+1, iy], top[ix+1, iy+1], top[ix, iy+1]))
perimeter = [(i, 0) for i in range(n)] + [(n, i) for i in range(n)] + [(n-i, n) for i in range(n)] + [(0, n-i) for i in range(n)]
bottom = {}
for key in perimeter:
    bottom[key] = len(stump)
    x, y, _ = stump[top[key]]
    stump.append((x, y, -50))
for k, key in enumerate(perimeter):
    nxt = perimeter[(k+1) % len(perimeter)]
    faces.append((bottom[key], bottom[nxt], top[nxt], top[key]))
faces.append(tuple(bottom[key] for key in perimeter))
def stump_reference(c):
    if c[2] > -49 and abs(c[0]) < 49.9 and abs(c[1]) < 49.9:
        return (0, 0, 1)
    if c[2] <= -49.9:
        return (0, 0, -1)
    return (c[0], c[1], 0)
# Split the torn top into triangles: the random heights make quads non-planar.
tris = []
for face in faces:
    if len(face) == 4 and all(stump[i][2] > -50 for i in face):
        tris += [(face[0], face[1], face[2]), (face[0], face[2], face[3])]
    else:
        tris.append(face)
write_mesh('StumpJagged', stump, tris, stump_reference)

# Hyperboloid cooling tower shell with an inner wall and top lip, open at the base.
segments, rings = 28, 12
def tower_radius(z):
    # Base radius 50 (unit footprint), waist 30 at z=20, lip about 35.
    return 30*math.sqrt(1+((z-20)/52.5)**2)
tower = []
for inner in (0, 1):
    for r in range(rings+1):
        z = -50+100*r/rings
        radius = tower_radius(z) - (2.2 if inner else 0)
        for s in range(segments):
            a = 2*math.pi*s/segments
            tower.append((math.cos(a)*radius, math.sin(a)*radius, z))
def tower_index(inner, r, s):
    return inner*(rings+1)*segments + r*segments + s % segments
faces, references = [], []
for inner in (0, 1):
    for r in range(rings):
        for s in range(segments):
            faces.append((tower_index(inner, r, s), tower_index(inner, r, s+1), tower_index(inner, r+1, s+1), tower_index(inner, r+1, s)))
for s in range(segments):
    faces.append((tower_index(0, rings, s), tower_index(0, rings, s+1), tower_index(1, rings, s+1), tower_index(1, rings, s)))
def tower_reference(c):
    radius = math.hypot(c[0], c[1])
    if c[2] > 49.9:
        return (0, 0, 1)
    shell = tower_radius(c[2]) - 1.1
    sign = 1 if radius > shell else -1
    return (c[0]*sign, c[1]*sign, 0)
write_mesh('CoolingTower', tower, faces, tower_reference, smooth=True)

# Cargo ship hull: flat deck, raked bow, square stern; length on X, beam on Y.
stations = [(-50, 1.0, 1.0), (-30, 1.0, 1.0), (25, 1.0, 1.0), (38, .82, .55), (46, .45, .18), (50, .04, .02)]
hull = []
for x, deck, keel in stations:
    hull += [(x, -50*deck, 50), (x, 50*deck, 50), (x, 36*keel, -50), (x, -36*keel, -50)]
faces = [(0, 1, 2, 3)]
last = (len(stations)-1)*4
faces.append((last, last+1, last+2, last+3))
for i in range(len(stations)-1):
    a, b = i*4, i*4+4
    for k in range(4):
        faces.append((a+k, a+(k+1) % 4, b+(k+1) % 4, b+k))
write_mesh('ShipHull', hull, faces, lambda c: (c[0]*.2, c[1], c[2]))
print('Generated seven city kit and destruction meshes.')
