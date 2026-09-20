"""Authored faceted armor profiles; centimetres, centred around the origin."""
from pathlib import Path
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
