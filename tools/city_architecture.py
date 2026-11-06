"""Offline modular town geometry. Coordinates are runtime cells, with Y up."""
import math
from mathutils import Vector

class House:
    def __init__(self,name,front,back,eaves,ridge,style="home"):
        self.style=style
        self.name,self.front,self.back,self.eaves,self.ridge=name,front,back,eaves,ridge
        self.faces=[]
    def face(self,pts,tile=1,normal=None,shade=1):
        pts=[Vector(p) for p in pts]
        # Remove collapsed arch-cap corners before Blender triangulates the polygon.
        changed=True
        while changed and len(pts)>=3:
            changed=False
            for i in range(len(pts)):
                a,b,c=pts[i-1],pts[i],pts[(i+1)%len(pts)]
                if (b-a).cross(c-b).length<1e-9:
                    pts.pop(i);changed=True;break
        if len(pts)<3:return
        n=(pts[1]-pts[0]).cross(pts[2]-pts[0])
        if n.length<1e-8: return
        if normal and n.dot(Vector(normal))<0: pts.reverse(); n=-n
        n.normalize()
        if n.y<-.5: shade*=.60
        self.faces.append((pts,tile,round(shade,2)))
    def box(self,x0,x1,y0,y1,z0,z1,tile,shade=1):
        for pts,n in [([(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0)],(0,0,-1)),
                      ([(x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)],(0,0,1)),
                      ([(x0,y0,z0),(x0,y0,z1),(x0,y1,z1),(x0,y1,z0)],(-1,0,0)),
                      ([(x1,y0,z0),(x1,y0,z1),(x1,y1,z1),(x1,y1,z0)],(1,0,0)),
                      ([(x0,y1,z0),(x1,y1,z0),(x1,y1,z1),(x0,y1,z1)],(0,1,0)),
                      ([(x0,y0,z0),(x1,y0,z0),(x1,y0,z1),(x0,y0,z1)],(0,-1,0))]: self.face(pts,tile,n,shade)
    def facade(self,point,length,door=False,upper=False):
        # point(u,height,inset) maps a wall's local coordinates into the complete shell.
        normal=(Vector(point(0,0,0))-Vector(point(0,0,1))).normalized()
        def panel(x0,x1,y0,y1,tile=1,inset=0,shade=1):
            self.face([point(x0,y0,inset),point(x1,y0,inset),point(x1,y1,inset),point(x0,y1,inset)],tile,normal,shade)
        windows=[(-length*.32,.49,.44,.43),(length*.32,.49,.44,.43)]
        if not door: windows=[(-length*.26,.52,.46,.47),(length*.26,.52,.46,.47)]
        if upper: windows += [(-length*.28,1.43,.48,.52),(length*.28,1.43,.48,.52)]
        if self.style=='hip': windows=[(-length*.27,.54,.58,.44),(length*.27,.54,.58,.44)]
        if self.style in ('dungeon','gate'): windows=[]
        if self.style=='provisioner' and door:
            windows=[(-.96,.40,.66,.62),(.96,.40,.66,.62)]
        holes=[(x-ww/2,x+ww/2,y,y+hh) for x,y,ww,hh in windows]
        if door: holes.append((-.5,.5,0,1.12))
        xs=sorted(set([-length/2,length/2]+[v for r in holes for v in r[:2]]))
        ys=sorted(set([0,.23,self.eaves]+[v for r in holes for v in r[2:]]+([1.12] if upper else [])))
        for x0,x1 in zip(xs,xs[1:]):
            for y0,y1 in zip(ys,ys[1:]):
                mx,my=(x0+x1)/2,(y0+y1)/2
                if any(a<mx<b and c<my<d for a,b,c,d in holes):continue
                panel(x0,x1,y0,y1,0 if self.style in ("sanctuary","dungeon","smithy","gate") or my<.23 or (upper and my<1.12) else 1)
        for x,y,ww,hh in windows:
            x0,x1=x-ww/2,x+ww/2; top=y+hh; depth=.085
            panel(x0,x1,y,top,3,depth,.66)
            for a,b in [((x0,y),(x0,top)),((x1,top),(x1,y)),((x0,top),(x1,top)),((x1,y),(x0,y))]:
                self.face([point(*a,0),point(*b,0),point(*b,depth),point(*a,depth)],5,shade=.69)
            # Raised stone lintel and sill; front/side faces have real thickness.
            for ya,yb in [(y-.045,y),(top,top+.055)]:
                panel(x0-.035,x1+.035,ya,yb,5,-.025)
                self.face([point(x0-.035,ya,-.025),point(x1+.035,ya,-.025),point(x1+.035,ya,0),point(x0-.035,ya,0)],5,shade=.7)
                self.face([point(x0-.035,yb,-.025),point(x1+.035,yb,-.025),point(x1+.035,yb,0),point(x0-.035,yb,0)],5)
            # Two shutter stiles and crossbars sit inside the reveal.
            for xa,xb in [(x0+.015,x0+.035),(x-.012,x+.012),(x1-.035,x1-.015)]: panel(xa,xb,y+.01,top-.01,3,depth-.008,.8)
            for yy in (y+.09,top-.09): panel(x0+.02,x1-.02,yy,yy+.025,6,depth-.012,.9)
        if not door:return
        # Rectangular opening above the outer arch closes with radial wedges.
        steps=12; spring=.62; ri=.36; ro=.50
        for k in range(steps):
            a=math.pi*k/steps; b=math.pi*(k+1)/steps
            xa,ya=ro*math.cos(a),spring+ro*math.sin(a)
            xb,yb=ro*math.cos(b),spring+ro*math.sin(b)
            self.face([point(xa,ya,0),point(xb,yb,0),point(xb,1.12,0),point(xa,1.12,0)],0 if self.style in ('sanctuary','dungeon','smithy','gate') else 1,normal)
            # Separate wedge stones over a continuous dark mortar backing.
            for gap,inset,shade in [(0,.006,.48),(.006,-.035,1)]:
                aa,bb=a+gap,b-gap
                points=[(ri*math.cos(aa),spring+ri*math.sin(aa)),(ro*math.cos(aa),spring+ro*math.sin(aa)),
                        (ro*math.cos(bb),spring+ro*math.sin(bb)),(ri*math.cos(bb),spring+ri*math.sin(bb))]
                self.face([point(x,y,inset) for x,y in points],5,normal,shade)
            # Seal the outer lip back to the facade; side approaches cannot see behind the arch.
            self.face([point(xa,ya,-.035),point(xb,yb,-.035),point(xb,yb,0),point(xa,ya,0)],5,
                      Vector(point(math.cos((a+b)/2),spring+math.sin((a+b)/2),0))-Vector(point(0,spring,0)),.9)
            ia=(ri*math.cos(a),spring+ri*math.sin(a)); ib=(ri*math.cos(b),spring+ri*math.sin(b))
            self.face([point(*ia,-.035),point(*ib,-.035),point(*ib,.23),point(*ia,.23)],5,Vector(point(0,spring,0))-Vector(point(math.cos((a+b)/2),spring+math.sin((a+b)/2),0)),.57)
        for side in (-1,1):
            panel(min(side*ri,side*ro),max(side*ri,side*ro),0,spring,5,.006,.48)
            for j in range(4):
                panel(min(side*ri,side*ro),max(side*ri,side*ro),j*spring/4+.003,(j+1)*spring/4-.003,5,-.035,1-.025*(j%2))
            self.face([point(side*ri,0,-.035),point(side*ri,spring,-.035),point(side*ri,spring,.23),point(side*ri,0,.23)],5,Vector(point(0,0,0))-Vector(point(side,0,0)),.64)
            self.face([point(side*ro,0,-.035),point(side*ro,spring,-.035),point(side*ro,spring,0),point(side*ro,0,0)],5,
                      Vector(point(side,0,0))-Vector(point(0,0,0)),.9)
        # Closed arched timber leaf behind the actual tunnel, with iron straps.
        outline=[(-ri,0),(ri,0),(ri,spring)]+[(ri*math.cos(math.pi*k/steps),spring+ri*math.sin(math.pi*k/steps)) for k in range(1,steps+1)]
        self.face([point(x,y,.232) for x,y in outline],3,normal,.76)
        for y in (.17,.52):
            panel(-.335,.335,y,y+.04,6,.22)
            for x in (-.29,.29): panel(x-.012,x+.012,y+.01,y+.03,5,.216,.6)
        if self.style=='hip':
            self.face([point(-.30,.19,.208),point(-.26,.15,.208),point(.30,.53,.208),point(.26,.57,.208)],3,normal,.8)
        if self.style in ('inn','dungeon'):
            panel(-.022,.022,0,.95,6 if self.style=='dungeon' else 3,.210,.72)
        # Ring handle: a small octagonal solid torus, not flat icon artwork.
        center=(.19,.42)
        for k in range(12):
            a,b=math.tau*k/12,math.tau*(k+1)/12
            for j in range(4):
                c,d=math.tau*j/4,math.tau*(j+1)/4
                def ring(t,q):return point(center[0]+(.035+.007*math.cos(q))*math.cos(t),center[1]+(.035+.007*math.cos(q))*math.sin(t),.20+.007*math.sin(q))
                self.face([ring(a,c),ring(b,c),ring(b,d),ring(a,d)],6)
        self.face([point(-ro,0,0),point(ro,0,0),point(ro,0,.235),point(-ro,0,.235)],5,(0,1,0),.78)
    def build(self):
        f,b,e,r=self.front,self.back,self.eaves,self.ridge
        self.facade(lambda u,y,d:(u,y,f+d),3,True,e>1.5)
        self.facade(lambda u,y,d:(-u,y,b-d),3,False,e>1.5)
        center=(f+b)/2;length=b-f
        self.facade(lambda u,y,d:(-1.5+d,y,center-u),length,False,e>1.5)
        self.facade(lambda u,y,d:(1.5-d,y,center+u),length,False,e>1.5)
        # Gables close both ends, roof plane sits outside them with a sealed underside.
        hip=self.style=='hip'
        for z,n in [(f,(0,0,-1)),(b,(0,0,1))]:
            if not hip:self.face([(-1.5,e,z),(1.5,e,z),(0,r-.05,z)],1,n)
        # Thin roof prism, restrained overhangs and visible fascia.
        if hip:
            a,c=f-.16,b+.16
            # Four roof planes, closed fascia and underside; ridge ends are recessed.
            ridge_a,ridge_b=a+.85,c-.85
            for side in (-1,1):
                self.face([(side*1.68,e,a),(side*1.68,e,c),(0,r,ridge_b),(0,r,ridge_a)],2,(side,1,0))
                self.face([(side*1.68,e-.06,a),(side*1.68,e-.06,c),(side*1.68,e,c),(side*1.68,e,a)],3,(side,0,0),.8)
            for z,rz,n in [(a,ridge_a,(0,1,-1)),(c,ridge_b,(0,1,1))]:
                self.face([(-1.68,e,z),(1.68,e,z),(0,r,rz)],2,n)
                self.face([(-1.68,e-.06,z),(1.68,e-.06,z),(1.68,e,z),(-1.68,e,z)],3,(0,0,n[2]),.8)
            self.face([(-1.68,e-.06,a),(1.68,e-.06,a),(1.68,e-.06,c),(-1.68,e-.06,c)],3,(0,-1,0),.7)
        else:
            a,c=f-.16,b+.16
            for side in (-1,1):
                self.face([(0,r,a),(side*1.68,e+.01,a),(side*1.68,e+.01,c),(0,r,c)],2,(side,1,0))
                self.face([(0,r-.05,a),(side*1.68,e-.04,a),(side*1.68,e-.04,c),(0,r-.05,c)],3,(0,-1,0),.76)
                for z,n in [(a,(0,0,-1)),(c,(0,0,1))]:
                    self.face([(0,r,z),(side*1.68,e+.01,z),(side*1.68,e-.04,z),(0,r-.05,z)],3,n,.85)
                self.face([(side*1.68,e-.04,a),(side*1.68,e+.01,a),(side*1.68,e+.01,c),(side*1.68,e-.04,c)],3,(side,0,0),.82)
        # Timber sill bands and small jointed eave brackets clear the walking path.
        if e>1.5:
            self.box(-1.51,1.51,1.09,1.15,f-.015,f+.015,3,.8)
            self.box(-1.51,1.51,1.09,1.15,b-.015,b+.015,3,.8)
            for x in (-1.5,1.5):self.box(x-.015,x+.015,1.09,1.15,f,b,3,.8)
        for x in (-1.51,1.51):
            for z in (f+.25,center,b-.25):
                self.box(x-.04,x+.04,e-.15,e-.04,z-.045,z+.045,3,.78)
        # Chimney emerges through the roof; hidden intersection stays below the opaque slate.
        x0,x1=.65,.94; z0,z1=b-.8,b-.48
        bottom=r-(r-e)*.65/1.68-.08; top=r+.26
        self.box(x0,x1,bottom,top,z0,z1,0,.9)
        self.box(x0-.035,x1+.035,top,top+.055,z0-.035,z1+.035,5)
        self.box(x0+.045,x1-.045,top+.055,top+.056,z0+.045,z1-.045,6,.45)
        if self.style=='inn':
            # Distinct timber-framed upper facade and a broad shallow door hood.
            for x in (-1.43,-.57,.57,1.43):
                self.box(x-.035,x+.035,1.15,e,f-.025,f+.015,3,.82)
            self.box(-.68,.68,1.16,1.24,f-.20,f+.02,3,.8)
        elif self.style=='smithy':
            # Large masonry flue; a blind dark firebox inset remains scenery.
            self.box(.54,1.20,e-.1,r+.62,b-.95,b-.25,0,.73)
            self.box(.49,1.25,r+.62,r+.72,b-1.0,b-.20,5,.85)
            self.box(.60,1.14,r+.72,r+.725,b-.89,b-.31,6,.45)
        elif self.style=='provisioner':
            for x in (-.96,.96):
                self.box(x-.36,x+.36,.34,.40,f-.15,f+.025,3,.8)
        elif self.style=='sanctuary':
            for x in (-1.40,1.40):
                self.box(x-.1,x+.1,0,1.28,f-.09,f+.08,5,.9)
            # Small solid stone bell-cote silhouette, with painted dark recess.
            self.box(-.36,.36,r-.10,r+.65,b-.72,b-.10,0)
            self.box(-.24,.24,r+.05,r+.48,b-.728,b-.72,6,.5)
            self.face([(-.43,r+.65,b-.78),(.43,r+.65,b-.78),(0,r+.96,b-.78)],5,(0,0,-1))
            self.face([(-.43,r+.65,b-.04),(.43,r+.65,b-.04),(0,r+.96,b-.04)],5,(0,0,1))
            self.face([(-.43,r+.65,b-.78),(.43,r+.65,b-.78),(.43,r+.65,b-.04),(-.43,r+.65,b-.04)],5,(0,-1,0),.7)
            for side in (-1,1):
                self.face([(0,r+.96,b-.78),(side*.43,r+.65,b-.78),(side*.43,r+.65,b-.04),(0,r+.96,b-.04)],2,(side,1,0))
        return self


def make_kit():
    models=[House('low-house',-1.5,1.5,1.23,1.85).build(),
            House('tall-house',-2.5,1.5,2.15,2.85).build(),
            House('low-house-hip',-1.5,1.5,1.20,1.76,'hip').build(),
            House('inn',-2.5,1.5,2.15,2.85,'inn').build(),
            House('smithy',-1.5,1.5,1.23,1.85,'smithy').build(),
            House('provisioner',-1.5,1.5,1.28,1.90,'provisioner').build(),
            House('sanctuary',-1.5,1.5,1.48,2.12,'sanctuary').build(),
            House('dungeon-entrance',-1.5,1.5,1.23,1.70,'dungeon').build()]
    gate=House('city-gate',-.5,1.5,1.65,1.8,'gate')
    gate.facade(lambda u,y,d:(u*2,y*1.6,-.5+d*1.5),2.5,True)
    gate.box(-2.5,2.5,0,2.64,1.4,1.5,0)
    for side in (-1,1):
        x=side*2.5
        gate.face([(x,0,-.5),(x,0,1.5),(x,2.64,1.5),(x,2.64,-.5)],0,(side,0,0))
        gate.face([(0,2.98,-.5),(side*2.5,2.64,-.5),(side*2.5,2.64,1.5),(0,2.98,1.5)],2,(side,1,0))
    for z,n in [(-.5,(0,0,-1)),(1.5,(0,0,1))]:
        gate.face([(-2.5,2.64,z),(2.5,2.64,z),(0,2.98,z)],0,n)
    # Central meeting stile distinguishes the broad paired gate leaves.
    gate.box(-.018,.018,0,1.75,-.169,-.150,6,.7)
    wall=House('boundary-wall',-.5,.5,1.65,1.75)
    wall.box(-.5,.5,0,1.65,-.5,.5,0,.92)
    wall.box(-.5,.5,1.65,1.75,-.5,.5,5,.95)
    models += [gate,wall]
    assert len(models)<=16
    return models
