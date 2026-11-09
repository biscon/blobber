"""BR02 pair-stage authoring. Run with background Blender, -t 2, -- --output-root NEW_ROOT.
Writes a staged asset tree; never edits the interactive Blender session or old props.
Geometry is in native cell units, +Y up. Retained masters supply all painted surfaces.
"""
import argparse
import json
import math
import sys
from pathlib import Path
import bpy
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from city_mesh_export import export_collection

DENSITY, ATLAS, GUTTER = 256, 2048, 4


class Model:
    def __init__(self, name):
        self.name, self.faces = name, []

    def face(self, points, material):
        p = np.asarray(points, dtype=float)
        normal = np.cross(p[1] - p[0], p[2] - p[0])
        normal /= np.linalg.norm(normal)
        self.faces.append((p, normal, material))

    def box(self, center, size, material='wood'):
        if material in ('wood', 'darkwood', 'endwood'):
            return self.bevel_box(center, size, material, min(.008, min(size)*.12))
        return self.plain_box(center, size, material)

    def plain_box(self, center, size, material):
        x, y, z = center
        w, h, d = np.asarray(size) / 2
        p = [(x-w,y-h,z-d),(x+w,y-h,z-d),(x+w,y+h,z-d),(x-w,y+h,z-d),
             (x-w,y-h,z+d),(x+w,y-h,z+d),(x+w,y+h,z+d),(x-w,y+h,z+d)]
        for indices in ((0,3,2,1),(4,5,6,7),(0,4,7,3),(1,2,6,5),(3,7,6,2),(0,1,5,4)):
            self.face([p[i] for i in indices], material)

    def oriented(self, points, material, outward):
        p = np.asarray(points, dtype=float)
        if np.dot(np.cross(p[1]-p[0], p[2]-p[0]), outward) < 0:
            p = p[::-1]
        self.face(p, material)

    def bevel_box(self, center, size, material, bevel=.012):
        # Six faces, twelve bevel strips and eight fitted corner triangles.
        half = np.asarray(size)/2
        bevel = min(bevel, float(min(half))*.4)
        def vertex(signs, outer):
            extent = half-bevel
            extent = extent.copy(); extent[outer] = half[outer]
            return np.asarray(center)+np.asarray(signs)*extent
        for axis in range(3):
            other = [a for a in range(3) if a != axis]
            for sign in (-1, 1):
                points = []
                for a,b in ((-1,-1),(-1,1),(1,1),(1,-1)):
                    signs = [0,0,0]; signs[axis]=sign; signs[other[0]]=a; signs[other[1]]=b
                    points.append(vertex(signs, axis))
                normal = np.zeros(3); normal[axis] = sign
                self.oriented(points, material, normal)
        for free in range(3):
            a,b = [i for i in range(3) if i != free]
            for sa in (-1,1):
                for sb in (-1,1):
                    signs=[0,0,0]; signs[a]=sa; signs[b]=sb
                    points=[]
                    for sf,outer in ((-1,a),(1,a),(1,b),(-1,b)):
                        signs[free]=sf; points.append(vertex(signs,outer))
                    normal=np.zeros(3);normal[a]=sa;normal[b]=sb
                    self.oriented(points,material,normal)
        for x in (-1,1):
            for y in (-1,1):
                for z in (-1,1):
                    signs=(x,y,z)
                    self.oriented([vertex(signs,a) for a in range(3)],material,signs)

    def extrude(self, outline, front, back, material):
        area=sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(outline,outline[1:]+outline[:1]))
        if area<0: outline=list(reversed(outline))
        self.face([(x,y,front) for x,y in reversed(outline)],material)
        self.face([(x,y,back) for x,y in outline],material)
        for a,b in zip(outline,outline[1:]+outline[:1]):
            self.face([(*a,front),(*b,front),(*b,back),(*a,back)],material)

    def ring(self, center, radius, thickness, material, segments=16, sides=4):
        # A closed iron handle in the XY plane, with real front/back thickness.
        def point(a,b):
            r=radius+thickness*math.cos(b)
            return (center[0]+r*math.cos(a),center[1]+r*math.sin(a),center[2]+thickness*math.sin(b))
        for i in range(segments):
            a,c=math.tau*i/segments,math.tau*(i+1)/segments
            for j in range(sides):
                b,d=math.tau*j/sides,math.tau*(j+1)/sides
                self.face([point(a,b),point(c,b),point(c,d),point(a,d)],material)

    def lathe(self, center, profile, material, sides=12):
        """Outward side rings, bottom-to-top. Reverse radius profile makes a hollow vessel."""
        x, y, z = center
        for (r0, h0), (r1, h1) in zip(profile, profile[1:]):
            for i in range(sides):
                a, b = math.tau*i/sides, math.tau*(i+1)/sides
                self.face([(x+r0*math.cos(a),y+h0,z+r0*math.sin(a)),
                           (x+r1*math.cos(a),y+h1,z+r1*math.sin(a)),
                           (x+r1*math.cos(b),y+h1,z+r1*math.sin(b)),
                           (x+r0*math.cos(b),y+h0,z+r0*math.sin(b))], material)

    def disk(self, center, radius, material, sides=12):
        x,y,z = center
        self.face([(x+radius*math.cos(-math.tau*i/sides),y,z+radius*math.sin(-math.tau*i/sides))
                   for i in range(sides)], material)

    def vessel(self, center, radius, height, material='clay'):
        self.lathe(center, [(radius*.62,0),(radius,height*.65),(radius*.9,height),
                            (radius*.73,height),(radius*.73,height*.62)], material)
        self.disk((center[0],center[1]+height*.62,center[2]), radius*.73, 'dark')


def author():
    models = []
    m = Model('hearth'); models.append(m)
    # Separate wall-matched masonry courses with chamfered edges and real joints.
    for i in range(4):
        m.bevel_box((-.25+i*.5,.035,0),(.492,.07,.98),'stone'+str(i%3),.009)
    for side,x in enumerate((-.32,1.32)):
        for row in range(3):
            m.bevel_box((x,.16+row*.176,-.045),(.34,.169,.77),'stone'+str((side+row)%3),.012)
    # Segmental arch: continuous inner reveal, individually fitted wedge stones.
    for i in range(9):
        a=math.pi*i/9+.008; b=math.pi*(i+1)/9-.008
        inner=lambda t:(.5+.65*math.cos(t),.60+.20*math.sin(t))
        outer=lambda t:(.5+.88*math.cos(t),.60+.34*math.sin(t))
        m.extrude([outer(a),outer(b),inner(b),inner(a)],-.43,.34,'stone'+str(i%3))
        # Fill the shoulders above the arch without an open or overlapping hood.
        oa,ob=outer(a),outer(b)
        m.extrude([oa,ob,(ob[0],.958),(oa[0],.958)],-.40,.34,'stone'+str((i+1)%3))
    for i in range(3):
        m.bevel_box((-.16+i*.66,.979,-.025),(.652,.042,.95),'stone'+str(i),.007)
    # The lined firebox has a visible stone back and ash floor, not a black void.
    m.plain_box((.5,.43,.405),(1.29,.72,.15),'soot')
    m.plain_box((.5,.082,-.015),(1.29,.024,.67),'ash')
    # Four feet support a connected grate frame; bars meet its front/back rails.
    for x in (.02,.98):
        for z in (-.26,.22):
            m.plain_box((x,.117,z),(.035,.046,.035),'iron')
    for z in (-.26,.22):
        m.plain_box((.5,.15,z),(1.05,.03,.035),'iron')
    for i in range(7):
        m.plain_box((.035+i*.155,.154,-.02),(.025,.026,.48),'iron')
    # Rounded cast-iron pot: curved belly, thick rolled lip, interior and two handles.
    m.lathe((.5,.188,-.02),[(.10,0),(.17,.022),(.215,.08),(.225,.16),(.205,.23),
            (.18,.255),(.18,.27),(.16,.27),(.155,.248),(.178,.22),(.195,.16),
            (.183,.09),(.14,.06)],'iron',sides=24)
    m.disk((.5,.248,-.02),.14,'dark',sides=24)
    for x in (.26,.74):
        m.ring((x,.365,-.02),.06,.012,'iron',segments=12)
    for x,z in ((.38,-.10),(.62,-.10),(.50,.12)):
        m.plain_box((x,.177,z),(.035,.03,.035),'iron')

    m = Model('work-surface'); models.append(m)
    # A lower shelf occupies the full footprint; legs finish at the shelf/top.
    for z in (-.36,-.12,.12,.36):
        m.box((.5,.245,z),(1.94,.045,.23))
        m.box((.5,.065,z),(1.94,.035,.23),'darkwood')
    for x in (-.37,1.37):
        for z in (-.36,.36):
            m.box((x,.1275,z),(.11,.185,.11))
            m.box((x,.025,z),(.14,.05,.14),'darkwood')
    for z in (-.39,.39):
        m.box((.5,.20,z),(1.62,.05,.065))
    m.box((.02,.279,-.08),(.45,.023,.39),'endwood')
    m.box((.18,.296,-.06),(.28,.012,.045),'iron')
    m.vessel((.85,.27,.1),.16,.075)
    m.vessel((1.20,.27,-.12),.10,.075,'wood')
    m.box((.55,.279,-.24),(.29,.025,.28),'cloth')
    for x in (.03,.42,.82,1.15):
        m.vessel((x,.085,.1),.13,.12)

    m = Model('provisions'); models.append(m)
    for z in (-.36,-.12,.12,.36):
        m.box((.5,.025,z),(1.98,.05,.23),'darkwood')
    # Two closed staved barrels, with raised hoops that have no coplanar faces.
    for x,z,h in ((-.18,-.12,.37),(.32,.14,.32)):
        m.lathe((x,.05,z),[(.19,0),(.23,h*.25),(.24,h*.6),(.20,h)],'wood')
        m.disk((x,.05+h,z),.20,'endwood')
        for level,radius in ((.07,.227),(h*.70,.238)):
            m.lathe((x,.05,z),[(radius,level),(radius,level+.025)],'iron')
    # Crates fill the right end with inset panels and distinct fitted corner rails.
    for x,z,w,d,h in ((1.00,.20,.87,.51,.37),(1.08,-.29,.66,.34,.22)):
        m.box((x,.05+h*.5,z),(w-.03,h,d-.03),'darkwood')
        for xx in (x-w*.5+.03,x+w*.5-.03):
            for zz in (z-d*.5+.03,z+d*.5-.03):
                m.box((xx,.05+h*.5,zz),(.06,h,.06))
        for level in (.08,.05+h-.035):
            for zz in (z-d*.5,z+d*.5):
                m.box((x,level,zz),(w-.12,.06,.025))
        for j in range(3):
            m.box((x-w*.5+(j+.5)*w/3,.05+h+.008,z),(w/3-.008,.016,d))
    # Tied grain sack in the remaining front space.
    m.lathe((.40,.05,-.29),[(.14,0),(.18,.08),(.14,.19),(.055,.25),(.075,.28)],'cloth')
    m.disk((.4,.33,-.29),.075,'cloth')
    m.lathe((.4,.05,-.29),[(.06,.23),(.06,.255)],'rope')

    m = Model('table-and-benches'); models.append(m)
    # Three-cell long table. Benches and end cross-feet explain grouped occupancy.
    for x in (.17,.50,.83):
        m.box((x,.32,1.0),(.32,.055,2.94))
    for z in (-.18,2.18):
        m.box((.5,.035,z),(1.94,.07,.19),'darkwood')
        m.box((.5,.17,z),(.22,.20,.23))
        m.box((.5,.275,z),(.98,.035,.28))
        for x in (-.30,1.30):
            m.box((x,.115,z),(.12,.09,.14))
    m.box((.5,.12,1),(.08,.08,2.25),'darkwood')
    for x in (-.30,1.30):
        m.box((x,.18,1),(.32,.055,2.94))
    for z in (-.05,1.85):
        for x in (.21,.79):
            m.box((x,.35,z),(.018,.005,.018),'iron')
    for z in (.2,1.4):
        m.vessel((.50,.348,z),.12,.048,'wood')
    for x,z in ((.23,.6),(.77,1.9)):
        m.vessel((x,.348,z),.045,.05,'wood')
    return models


def mapping(points, normal):
    axis = points[1]-points[0]; axis /= np.linalg.norm(axis)
    cross = np.cross(normal,axis)
    coords = np.column_stack(((points-points[0])@axis,(points-points[0])@cross))
    coords -= coords.min(axis=0)
    extent = coords.max(axis=0)
    if extent[1] > extent[0]:
        coords = coords[:,::-1]; extent = extent[::-1]
    return coords*DENSITY, np.maximum(1,np.ceil(extent*DENSITY-1e-7)).astype(int)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-root',type=Path,required=True)
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    output = args.output_root.resolve()
    if output.exists():
        raise RuntimeError('Use a new staged output root')
    pack = output/'packs/underkeep'; (pack/'models/rooms').mkdir(parents=True)
    (pack/'source/rooms').mkdir(parents=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    def load(path):
        image = bpy.data.images.load(str(path))
        data = np.empty(len(image.pixels),dtype=np.float32); image.pixels.foreach_get(data)
        data = data.reshape(image.size[1],image.size[0],4)[::-1].copy()
        bpy.data.images.remove(image)
        return data
    old = load(ROOT/'assets/packs/underkeep/source/material-master.png')
    fresh = load(ROOT/'assets/packs/underkeep/source/rooms/material-master.png')
    h,w = fresh.shape[:2]
    fields = {'cloth':fresh[24:h//2-24,w//2+24:w-24],
              'clay':fresh[h//2+24:h-24,24:w//2-24], 'iron':fresh[h//2+24:h-24,w//2+24:w-24],
              'wood':old[40:145,40:580], 'endwood':old[40:570,680:1200]}
    fields['darkwood'] = fields['wood'].copy(); fields['darkwood'][:,:,:3] *= .55
    wall = load(ROOT/'assets/textures/wall.png')
    # Actual dungeon-wall paint, sampled at native density, not a different stone style.
    fields['stone0'] = wall[74:102,3:40].copy()
    fields['stone1'] = wall[110:138,95:135].copy()
    fields['stone2'] = wall[4:22,141:208].copy()
    fields['soot'] = wall.copy(); fields['soot'][:,:,:3] *= .58
    fields['ash'] = wall[110:138,95:135].copy()
    ash_gray=fields['ash'][:,:,:3].mean(axis=2,keepdims=True)
    fields['ash'][:,:,:3]=ash_gray*np.array([.48,.46,.43])
    fields['dark'] = fields['iron'].copy(); fields['dark'][:,:,:3] *= .36
    fields['rope'] = fields['cloth'].copy(); fields['rope'][:,:,:3] *= .62
    # Match the desaturated accepted timber treatment, not the raw master's orange.
    for key in ('wood','endwood','darkwood'):
        rgb=fields[key][:,:,:3]; mean=rgb.mean(axis=2,keepdims=True)
        fields[key][:,:,:3]=(rgb*.72+mean*.28)*.78
    models = author()
    # User-requested half-size geometry. Recenter in the smaller occupied cells;
    # back wall pieces also move to the cell's rear boundary rather than floating.
    for model in models:
        offset = np.array([-.25,0,.25 if model.name != 'table-and-benches' else 0])
        model.faces = [(p*.5+offset,n,mat) for p,n,mat in model.faces]
    entries = {}; faces = {}
    for m in models:
        faces[m.name] = []
        for points,normal,material in m.faces:
            uv,size = mapping(points,normal)
            shade = .54 if normal[1]<-.5 else 1.0 if normal[1]>.5 else .83
            key = (int(size[0]),int(size[1]),material,shade)
            entries[key] = None
            faces[m.name].append((points,normal,uv,key))
    atlas = np.ones((ATLAS,ATLAS,4),dtype=np.float32); atlas[:,:,:3] = .08
    x=y=row=0
    for key in sorted(entries,key=lambda k:(-k[1],-k[0],k[2],k[3])):
        w,h,material,shade = key
        if x+w+2*GUTTER>ATLAS: x=0; y+=row; row=0
        if y+h+2*GUTTER>ATLAS: raise RuntimeError('2048 atlas exhausted; never shrink metric UVs')
        entries[key] = (x+GUTTER,y+GUTTER)
        yy,xx = np.mgrid[:h,:w]; source=fields[material]
        # Mirror at sample boundaries; genuine master detail, no low-resolution upscale.
        def mirror(q,n):
            q=q%(2*n); return np.where(q<n,q,2*n-q-1)
        patch=source[mirror(yy,source.shape[0]),mirror(xx,source.shape[1])].copy()
        edge=np.minimum(np.minimum(xx,w-1-xx),np.minimum(yy,h-1-yy))
        painted=(.70+.30*np.minimum(edge/2.5,1))*shade
        patch[:,:,:3]*=painted[:,:,None]
        atlas[y:y+h+8,x:x+w+8]=np.pad(patch,((4,4),(4,4),(0,0)),mode='edge')
        x+=w+8; row=max(row,h+8)
    image=bpy.data.images.new('room-pair-atlas',width=ATLAS,height=ATLAS,alpha=True)
    image.pixels.foreach_set(np.ascontiguousarray(atlas[::-1]).ravel())
    image.filepath_raw=str(pack/'source/rooms/pair-atlas.png'); image.file_format='PNG'; image.save()
    contracts={k['id']:k for k in json.loads((ROOT/'assets/blueprints/underkeep.json').read_text())['kits']}
    records=[]; report={}
    for i,m in enumerate(models):
        vertices=[]
        for points,normal,uv,key in faces[m.name]:
            origin=np.asarray(entries[key]); uv=(uv+origin)/ATLAS
            for j in range(1,len(points)-1):
                ids=(0,j,j+1)
                p=points[list(ids)]; t=uv[list(ids)]
                # Jacobian singular values catch anisotropic distortion, unlike area alone.
                e=np.column_stack((p[1]-p[0],p[2]-p[0]))
                du=np.column_stack((t[1]-t[0],t[2]-t[0]))*ATLAS
                singular=np.linalg.svd(du@np.linalg.pinv(e),compute_uv=False)
                assert np.max(np.abs(singular-DENSITY))<.001, (m.name,'UV stretch',singular)
                for k in ids: vertices.append([round(float(v),8) for v in (*points[k],*normal,*uv[k])])
        contract=contracts[m.name]; arr=np.asarray(vertices); w,d=contract['size']
        assert len(vertices)<=6000 and np.all(np.isfinite(arr))
        assert np.all(arr[:,:3].min(axis=0)>=np.array([-.5,0,-.5])-1e-6)
        assert np.all(arr[:,:3].max(axis=0)<=np.array([w-.5,contract['height'],d-.5])+1e-6)
        record=dict(name='room-'+m.name,texture='packs/underkeep/source/rooms/pair-atlas.png',vertices=vertices)
        (pack/'models/rooms'/f'{m.name}.mesh.json').write_text(json.dumps(record,separators=(',',':'))+'\n')
        records.append((record,((i%2)*3.4,0,(i//2)*4.0)))
        report[m.name]=dict(triangles=len(vertices)//3,bounds=[arr[:,:3].min(axis=0).tolist(),arr[:,:3].max(axis=0).tolist()])
    export_collection(pack,'blueprint-room-pair',records)
    print(json.dumps(dict(kits=report,atlas=[ATLAS,ATLAS],density=DENSITY,packed_rows=y+row),indent=2))


if __name__=='__main__': main()
