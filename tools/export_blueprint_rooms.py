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
TAPESTRY_X = np.linspace(-.325,.325,17)
TAPESTRY_Z = .475 + .008*np.cos(np.arange(17)*math.pi/2)
TAPESTRY_U = np.concatenate(([0],np.cumsum(np.hypot(np.diff(TAPESTRY_X),np.diff(TAPESTRY_Z)))))


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

    # Everything below is authored directly in final native cell units.
    m = Model('work-surface'); models.append(m)
    for z in (.07,.19,.31,.43):
        m.box((0,.30875,z),(.97,.0225,.115)) # Top surface .32.
        m.box((0,.0725,z),(.97,.0175,.115),'darkwood')
    for x in (-.435,.435):
        for z in (.07,.43):
            m.box((x,.1525,z),(.055,.255,.055))
            m.box((x,.0125,z),(.07,.025,.07),'darkwood')
    for z in (.055,.445):
        m.box((0,.28,z),(.81,.035,.0325))
    m.box((-.24,.3275,.21),(.225,.0115,.195),'endwood')
    m.box((-.16,.336,.22),(.14,.006,.0225),'iron')
    m.vessel((.175,.321,.30),.08,.0375)
    m.vessel((.35,.321,.19),.05,.0375,'wood')
    m.box((.025,.3275,.13),(.145,.0125,.14),'cloth')
    for x in (-.235,-.04,.16,.325):
        m.vessel((x,.082,.30),.065,.06)

    m = Model('barrel'); models.append(m)
    radius=.24*.57; h=radius*3; z=.495-radius
    profile=[(radius*.80,0),(radius*.96,h*.20),(radius,h*.50),
             (radius*.96,h*.80),(radius*.80,h)]
    m.lathe((0,0,z),profile,'wood',sides=20)
    m.disk((0,h,z),radius*.80,'endwood',sides=20)
    # Closed underside, outward-facing.
    m.face([(radius*.80*math.cos(math.tau*i/20),0,z+radius*.80*math.sin(math.tau*i/20))
            for i in range(20)],'darkwood')
    for level in (.055,h-.075):
        r0=np.interp(level,[p[1] for p in profile],[p[0] for p in profile])
        r1=np.interp(level+.02,[p[1] for p in profile],[p[0] for p in profile])
        m.lathe((0,0,z),[(r0,level),(r0+.003,level),(r1+.003,level+.02),(r1,level+.02)],'iron',sides=20)

    for name,w,d,h in [('crate-large',.87*.57,.51*.57,.37*.57),
                        ('crate-small',.66*.57,.34*.57,.22*.57)]:
        m=Model(name);models.append(m);z=.49-d/2
        m.box((0,h/2,z),(w-.017,h,d-.017),'darkwood')
        for x in (-w/2+.017,w/2-.017):
            for zz in (z-d/2+.017,z+d/2-.017):
                m.box((x,h/2,zz),(.034,h,.034))
        for y in (.017,h-.020):
            for zz in (z-d/2,z+d/2):
                m.box((0,y,zz),(w-.068,.034,.014))
        for j in range(3):
            m.box((-w/2+(j+.5)*w/3,h+.00456,z),(w/3-.00456,.00912,d))

    m=Model('grain-sack');models.append(m);scale=.57;z=.49-.18*scale
    m.lathe((0,0,z),[(r*scale,h*scale) for r,h in
            [(.14,0),(.18,.08),(.14,.19),(.055,.25),(.075,.28)]],'cloth',sides=16)
    m.disk((0,.28*scale,z),.075*scale,'cloth',sides=16)
    m.lathe((0,0,z),[(.06*scale,.23*scale),(.06*scale,.255*scale)],'rope',sides=16)

    m = Model('table-and-benches'); models.append(m)
    for x in (-.165,0,.165):
        m.box((x,.26625,.5),(.16,.0275,1.47)) # Top surface .28.
    for z in (-.09,1.09):
        m.box((0,.0175,z),(.97,.035,.095),'darkwood')
        m.box((0,.1325,z),(.11,.195,.115))
        m.box((0,.24,z),(.49,.025,.14))
        for x in (-.4,.4):
            m.box((x,.0956,z),(.06,.12125,.07))
    m.box((0,.1,.5),(.04,.04,1.125),'darkwood')
    for x in (-.4,.4):
        m.box((x,.15625,.5),(.16,.0275,1.47)) # Seat surface .17.
    for z in (-.025,.925):
        for x in (-.145,.145):
            m.box((x,.28125,z),(.009,.0025,.009),'iron')
    for z in (.1,.7):
        m.vessel((0,.2805,z),.06,.024,'wood')
    for x,z in ((-.135,.3),(.135,.95)):
        m.vessel((x,.2805,z),.0225,.025,'wood')

    m=Model('tapestry');models.append(m)
    # Continuous UVs follow the cloth's unfolded arc length across shallow folds.
    for i in range(len(TAPESTRY_X)-1):
        x0,x1=TAPESTRY_X[i:i+2];z0,z1=TAPESTRY_Z[i:i+2]
        front=[(x0,.65,z0),(x0,1.55,z0),(x1,1.55,z1),(x1,.65,z1)]
        m.face(front,'heraldry')
        back=[(x,y,z+.004) for x,y,z in front]
        m.face(back[::-1],'cloth')
        for j in range(4):
            k=(j+1)%4
            m.face([front[j],back[j],back[k],front[k]],'cloth')
    m.bevel_box((0,1.58,.465),(.75,.018,.022),'iron',.004)
    for x in (-.28,.28):
        m.box((x,1.557,.463),(.027,.04,.025),'rope')

    m=Model('brazier');models.append(m);z=.25
    # Turned bowl with visible inner wall, rolled rim and a coal bed.
    m.lathe((0,0,z),[(.055,.265),(.11,.29),(.15,.34),(.15,.36),(.135,.36),
                    (.132,.34),(.095,.30),(.05,.285)],'iron',sides=24)
    m.disk((0,.285,z),.05,'ash',sides=24)
    m.face([(.055*math.cos(math.tau*i/24),.265,z+.055*math.sin(math.tau*i/24))
            for i in range(24)],'iron')
    # Three splayed, fitted legs and feet; each strut is a closed eight-sided prism.
    for i in range(3):
        angle=math.tau*i/3
        a=np.array([.12*math.cos(angle),.018,z+.12*math.sin(angle)])
        b=np.array([.06*math.cos(angle),.273,z+.06*math.sin(angle)])
        axis=(b-a)/np.linalg.norm(b-a)
        u=np.cross(axis,[0,1,0]);u/=np.linalg.norm(u);v=np.cross(axis,u)
        lo=[a+.012*(math.cos(math.tau*j/8)*u+math.sin(math.tau*j/8)*v) for j in range(8)]
        hi=[p+(b-a) for p in lo]
        m.oriented(lo,'iron',-axis);m.oriented(hi,'iron',axis)
        for j in range(8):
            k=(j+1)%8
            m.oriented([lo[j],lo[k],hi[k],hi[j]],'iron',lo[j]-a)
        m.bevel_box((a[0],.012,a[2]),(.06,.024,.06),'iron',.004)
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
    def load(path, size=None):
        image = bpy.data.images.load(str(path))
        if size: image.scale(*size)
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
    fields['heraldry'] = load(ROOT/'assets/packs/underkeep/source/rooms/heraldic-tapestry-master.png',
                              (math.ceil(TAPESTRY_U[-1]*DENSITY),math.ceil(.9*DENSITY)))
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
    # The accepted hearth remains exactly as exported in the preceding review.
    models[0].faces = [(p*.5+np.array([-.25,0,.25]),n,mat) for p,n,mat in models[0].faces]
    entries = {}; faces = {}
    for m in models:
        faces[m.name] = []
        for points,normal,material in m.faces:
            if material == 'heraldry':
                uv=np.column_stack((np.interp(points[:,0],TAPESTRY_X,TAPESTRY_U),1.55-points[:,1]))*DENSITY
                size=np.array([math.ceil(TAPESTRY_U[-1]*DENSITY),math.ceil(.9*DENSITY)])
            else:
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
