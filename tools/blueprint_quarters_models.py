"""Native-scale barracks/armory furniture; shared authoring primitives own export/UVs."""
import math
import numpy as np


def author_quarters(Model):
    models = []

    def chest(m, z=.25):
        m.box((0,.105,z),(.65,.19,.30),'darkwood')
        for x in (-.215,0,.215):
            m.plain_box((x,.211,z),(.207,.026,.31),'wood')
        for x in (-.24,.24):
            m.plain_box((x,.23,z),(.028,.012,.31),'iron-smooth')
            m.plain_box((x,.11,z-.156),(.028,.21,.012),'iron-smooth')
        m.box((0,.15,z-.17),(.055,.075,.023),'iron-smooth')
        m.ring((0,.12,z-.189),.022,.005,'iron-smooth',8,4)

    def cushion(m, center, size, material, bulge=.018):
        # Rounded sewn volumes, triangulated patches preserve metric UVs on folds.
        x,y,z=center; w,h,d=size
        rings=[]
        for sy,shrink in ((-.5,.12),(-.25,0),(.25,0),(.5,.12)):
            ring=[]
            for i in range(16):
                a=math.tau*i/16
                px=math.copysign(abs(math.cos(a))**.38,math.cos(a))*w/2*(1-shrink)
                pz=math.copysign(abs(math.sin(a))**.38,math.sin(a))*d/2*(1-shrink)
                ring.append((x+px,y+sy*h,z+pz))
            rings.append(ring)
        for lo,hi in zip(rings,rings[1:]):
            for i in range(16):
                j=(i+1)%16
                m.face([lo[i],hi[i],hi[j]],material)
                m.face([lo[i],hi[j],lo[j]],material)
        for ring,sign in ((rings[0],-1),(rings[-1],1)):
            for i in range(16):
                m.oriented([(x,y+sign*(h/2+bulge),z),ring[i],ring[(i+1)%16]],material,(0,sign,0))

    def blanket(m, color):
        xs=[-.385,-.367,-.26,-.12,0,.14,.27,.367,.385]
        zs=[.02,.12,.30,.48,.66,.84,1.01]
        grid=[]
        for iz,z in enumerate(zs):
            row=[]
            for ix,x in enumerate(xs):
                drop=.14 if ix in (0,8) else .012 if ix in (1,7) else 0
                fold=.008*math.sin(ix*2.6+iz*.9)+.006*math.cos(iz*2+ix)
                row.append((x,.337-drop+fold,z))
            grid.append(row)
        top_start=len(m.faces)
        for a,b in zip(grid,grid[1:]):
            for j in range(len(xs)-1):
                material='wool-border-smooth' if j in (1,6) else color
                m.oriented([a[j],b[j],b[j+1]],material,(0,1,0))
                m.oriented([a[j],b[j+1],a[j+1]],material,(0,1,0))
        for p,n,mat in list(m.faces[top_start:]):
            m.face(p[::-1]-np.array([0,.009,0]),mat)
        perimeter=grid[0]+[r[-1] for r in grid[1:]]+list(reversed(grid[-1][:-1]))+[r[0] for r in reversed(grid[1:-1])]
        for a,b in zip(perimeter,perimeter[1:]+perimeter[:1]):
            m.face([a,b,(b[0],b[1]-.009,b[2]),(a[0],a[1]-.009,a[2])],color)

    for name,color in [('sleeping-bay','wool-green-smooth'),('sleeping-bay-red','wool-red-smooth')]:
        m=Model(name);models.append(m)
        for x in (-.37,.37):
            for z,h in ((-.04,.35),(1.38,.55)):
                m.box((x,h/2,z),(.065,h,.065),'wood')
            m.box((x,.20,.67),(.055,.10,1.42),'wood')
        for z,h,y in ((1.38,.29,.375),(-.04,.12,.27)):
            for x in (-.24,0,.24):
                m.plain_box((x,y,z),(.232,h,.045),'wood')
            m.box((0,y+h/2,z),(.79,.035,.075),'wood')
        m.plain_box((0,.20,.67),(.70,.04,1.38),'darkwood')
        cushion(m,(0,.262,.67),(.70,.08,1.34),'linen-smooth',.008)
        blanket(m,color)
        cushion(m,(0,.343,1.20),(.52,.075,.25),'linen-smooth',.012)
        m.locker_start=len(m.faces)
        chest(m,-.30)
    m=Model('footlocker');models.append(m);chest(m)

    def sword(m,x,y,z,length=.58):
        # Convex diamond-section blade, crisp ridge instead of a featureless rectangle.
        tip=np.array((x,y,z));base=y+length*.76;w=length*.047
        rings=[[(x-w,h,z),(x,h,z-.012),(x+w,h,z),(x,h,z+.012)]
               for h in (y+length*.16,base)]
        for i in range(4):
            j=(i+1)%4
            outward=((rings[0][i][0]+rings[0][j][0])/2-x,0,(rings[0][i][2]+rings[0][j][2])/2-z)
            m.oriented([tip,rings[0][j],rings[0][i]],'steel-smooth',outward)
            m.oriented([rings[0][i],rings[0][j],rings[1][j],rings[1][i]],'steel-smooth',outward)
        m.oriented(rings[1],'steel-smooth',(0,1,0))
        m.box((x,base+.015,z),(.19*length,.025,.035),'iron-smooth')
        m.rod((x,base+.025,z),(x,y+length-.02,z),.017,'darkwood',8)
        m.shell((x,y+length-.025,z),[(.023,0),(.026,.018),(.012,.033)],'steel-smooth',8)

    m=Model('weapon-storage');models.append(m)
    for x in (-.17,2.17):
        m.box((x,.48,.35),(.075,.96,.09),'wood')
        m.box((x,.035,.30),(.19,.07,.31),'darkwood')
    for y in (.18,.62,.89): m.box((1,y,.37),(2.44,.07,.09),'wood')
    for x in (.02,.28):
        m.rod((x,.07,.22),(x,1.08,.22),.014,'wood')
        m.extrude([(x-.028,1.05),(x-.045,1.13),(x,1.22),(x+.045,1.13),(x+.028,1.05)],.205,.235,'steel-smooth')
    for x,l in ((.65,.61),(1,.66),(1.35,.57)): sword(m,x,.15,.23,l)
    for x in (1.73,2.02):
        m.rod((x,.10,.22),(x,.72,.22),.018,'wood')
        outline=[(x-.02,.72),(x-.13,.76),(x-.20,.72),(x-.23,.66),
                 (x-.21,.58),(x-.15,.54),(x-.02,.59)]
        # A thick forged socket and broad curved cutting edge, with a raised central cheek.
        m.extrude(outline,.208,.232,'steel-smooth')
        center=np.array((x-.07,.65))
        inner=[tuple(center+(np.array(p)-center)*.72) for p in outline]
        m.face([(a,b,.19) for a,b in reversed(inner)],'iron-smooth')
        for a,b,c,d in zip(outline,outline[1:]+outline[:1],inner,inner[1:]+inner[:1]):
            m.oriented([(*a,.208),(*b,.208),(*d,.19),(*c,.19)],'steel-smooth',(0,0,-1))
        m.box((x-.005,.66,.22),(.055,.12,.052),'iron-smooth')
    # Shield faces above the weapons; wooden back, metal edging, raised boss.
    cx=.72;cy=1.02
    m.extrude([(cx+.17*math.cos(a),cy+.17*math.sin(a)) for a in np.linspace(0,math.tau,16,endpoint=False)],.25,.285,'wood')
    m.ring((cx,cy,.235),.17,.012,'iron-smooth',16,4)
    m.extrude([(cx-.045,cy),(cx,cy-.045),(cx+.045,cy),(cx,cy+.045)],.215,.24,'steel-smooth')
    cx=1.47
    m.extrude([(cx-.15,1.19),(cx+.15,1.19),(cx+.13,1.0),(cx,.83),(cx-.13,1.0)],.25,.29,'iron-smooth')
    m.extrude([(cx-.127,1.17),(cx+.127,1.17),(cx+.11,1.0),(cx,.86),(cx-.11,1.0)],.24,.249,'wool-red-smooth')
    m.box((cx,1.035,.23),(.018,.29,.013),'steel-smooth')

    m=Model('armory-workbench');models.append(m)
    support_z=(-.25,.25)
    for x in (-.36,.36):
        for z in support_z:m.box((x,.16,z),(.065,.32,.065),'wood')
        m.box((x,.10,0),(.05,.055,.50),'darkwood')
    for z in support_z:m.box((0,.275,z),(.76,.07,.045),'wood')
    for z in (-.22,0,.22):m.box((0,.34,z),(.87,.055,.212),'wood')
    helmet_start=len(m.faces)
    m.box((0,.95,.36),(.86,.04,.24),'wood')
    for x in (-.30,.30):m.rod((x,.78,.46),(x,.94,.28),.018,'iron-smooth')
    for x in (-.23,.19):
        m.shell((x,.973,.34),[(.092,0),(.092,.10),(.07,.15),(.025,.18)],'steel-smooth',12,.82)
        m.box((x,1.035,.262),(.11,.018,.008),'dark')
    helmet_end=len(m.faces)
    s=Model('blade');sword(s,0,0,0,.65)
    for p,n,mat in s.faces:
        # Vertical sword becomes a laid-out blade along the worktop's X axis.
        m.face(np.column_stack((p[:,1]-.32,.378-p[:,2],p[:,0]-.06)),mat)
    m.bevel_box((.22,.38,.22),(.15,.035,.075),'stone1',.008)
    m.vessel((-.29,.368,.23),.045,.10,'clay-ochre-smooth')
    cushion(m,(.30,.372,-.20),(.18,.015,.15),'linen-smooth',.002)

    m=Model('bedding-shelf');models.append(m)
    m.box((0,.80,.35),(.87,.045,.28),'wood')
    for x in (-.31,.31):m.rod((x,.64,.48),(x,.78,.24),.016,'iron-smooth')
    for x,col in ((-.24,'wool-green-smooth'),(.04,'linen-smooth')):
        m.rod((x-.10,.91,.34),(x+.10,.91,.34),.087,col,12)
        m.rod((x-.012,.91,.34),(x+.012,.91,.34),.089,'rope',12)
    m.vessel((.30,.823,.34),.06,.16,'clay-ochre-smooth')
    m=Model('coat-pegs');models.append(m)
    m.box((0,1.0,.46),(.84,.07,.06),'wood')
    for x in (-.27,0,.27):m.rod((x,1,.45),(x,1.02,.34),.016,'wood')
    # Cloth mantle with a gathered neck, curved hem and broad irregular folds.
    rows=[]
    for y,w in ((.98,.08),(.88,.20),(.65,.24),(.31,.29)):
        rows.append([(x,y+(.018*math.cos(i*2) if y==.31 else 0),.37+.025*math.cos(i*2.4)) for i,x in enumerate(np.linspace(-w,w,9))])
    for a,b in zip(rows,rows[1:]):
        for j in range(8):
            m.oriented([a[j],b[j],b[j+1]],'wool-red-smooth',(0,0,-1))
            m.oriented([a[j],b[j+1],a[j+1]],'wool-red-smooth',(0,0,-1))
    m.box((.29,.67,.40),(.024,.64,.014),'darkwood')
    m.box((.29,.72,.385),(.040,.055,.012),'iron-smooth')

    m=Model('wall-sconce');models.append(m)
    m.bevel_box((0,.82,.472),(.12,.34,.035),'iron-smooth',.012)
    m.rod((0,.76,.46),(0,.78,.23),.024,'iron-smooth')
    m.lathe((0,0,.23),[(.045,.77),(.15,.85),(.15,.87),(.13,.87),(.055,.80)],'iron-smooth',12)
    m.disk((0,.81,.23),.07,'ash',12)
    # Native metric geometry: tune floor pieces independently of wall mounting heights.
    for m in models:
        transformed=[]
        for i,(p,n,mat) in enumerate(m.faces):
            scale=.6
            offset=np.array([0,0,.49*(1-scale)])
            if m.name.startswith('sleeping-bay'):
                if i >= m.locker_start:
                    # Beside the foot, preserving the larger bed and locker within one visual cell.
                    transformed.append((np.column_stack((.36-(p[:,2]+.30)*scale,
                                                          p[:,1]*scale,-.19+p[:,0]*scale)),
                                        np.array([-n[2],n[1],n[0]]),mat))
                    continue
                offset[2]=.49-1.4175*scale
            elif m.name=='weapon-storage':
                offset[0]=.5-scale  # Preserve rack center within its two-cell visual envelope.
            elif m.name=='armory-workbench':
                if helmet_start <= i < helmet_end:
                    offset[1]=.65-.95*scale
                else:
                    scale=.8
                    offset[2]=.49-.326*scale
            elif m.name=='wall-sconce':
                offset[1]=.58-.86*scale
            elif m.name=='coat-pegs':
                offset[1]=.68-1.0*scale
            elif m.name=='bedding-shelf':
                offset[1]=.65-.80*scale
            transformed.append((p*scale+offset,n,mat))
        m.faces=transformed
    # The right armory sconce moves along its wall toward the workbench.
    near=Model('wall-sconce-near-table')
    near.faces=[(p+np.array([.35,0,0]),n,mat)
                for p,n,mat in models[-1].faces]
    models.append(near)
    return models
