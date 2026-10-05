#!/usr/bin/env python3
"""The illustrated crew manifest: three figures, regenerable.

  1  manifest-1-hierarchy.png    rank ladder, the chain, nine departments with sizes, operational chains,
                                 and the special cases that would break a single-ladder design
  2  manifest-2-watch.png        the three eight-hour watches, stations by watch, who is awake at 0300
  3  manifest-3-hazard-team.png  the Hazard Team: A and B squads, reporting lines, and why the unit exists

Run:  python3 tools/manifest_gen.py
Content lives in `docs/crew-manifest.md`; this file only draws it, so keep them in step.
"""
from PIL import Image, ImageDraw, ImageFont
FT="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FS="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FM="/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
BG=(12,13,17); PANEL=(20,23,30); FG=(236,238,244); DIM=(136,143,156)
AMBER=(240,168,72); TEAL=(96,196,222); GREEN=(104,196,140); RED=(226,96,84); VIOLET=(168,140,232)
def f(p,s): return ImageFont.truetype(p,s)
def box(d,xy,fill=None,outline=None,w=1):
    d.rectangle(xy,fill=fill,outline=outline,width=w)
def wrap(d,text,font,maxw,draw=False):
    words=text.split(); lines=[]; cur=""
    for wd in words:
        t=(cur+" "+wd).strip()
        if d.textlength(t,font=font)<=maxw: cur=t
        else: lines.append(cur); cur=wd
    if cur: lines.append(cur)
    return lines
def para(d,x,y,text,font,fill,maxw,lh=None):
    lh=lh or font.size+5
    for i,l in enumerate(wrap(d,text,font,maxw)):
        d.text((x,y+i*lh),l,font=font,fill=fill)
    return y+len(wrap(d,text,font,maxw))*lh

# ---------------- panel 1: hierarchy ----------------
W,H=1760,1180
img=Image.new("RGB",(W,H),BG); d=ImageDraw.Draw(img)
d.rectangle([0,0,W,58],fill=AMBER)
d.text((26,16),"USS VOYAGER — CREW MANIFEST   ·   141 CREW   ·   RANK, POST, DEPARTMENT",font=f(FT,22),fill=(18,18,22))
d.text((26,74),"Three axes, kept apart:  RANK is authority   ·   POST is function and access   ·   DEPARTMENT is scope",font=f(FS,16),fill=DIM)

# ladder
lx=34; ly=120
box(d,[lx,ly,lx+330,H-40],fill=PANEL)
d.text((lx+16,ly+12),"THE LADDER",font=f(FT,17),fill=AMBER)
ladder=[("Captain","ship-wide","decides, accountable"),
        ("Commander","ship-wide","first officer; owns the crew"),
        ("Lt. Commander","department","department head"),
        ("Lieutenant","division","leads a division or watch"),
        ("Lieutenant (jg)","shift","leads a shift or team"),
        ("Ensign","post","holds a post; sent in first"),
        ("Chief Petty Officer","post","knows the machinery; trains ensigns"),
        ("Petty Officer","post","the technical backbone"),
        ("Crewman","post","the hands")]
y=ly+44
for r,scope,what in ladder:
    d.text((lx+16,y),r,font=f(FT,15),fill=FG)
    d.text((lx+16,y+19),what,font=f(FS,12),fill=DIM)
    d.text((lx+250,y+2),scope,font=f(FM,11),fill=TEAL)
    y+=52
d.text((lx+16,y+8),"A POST needs a qualification.",font=f(FS,12),fill=AMBER)
d.text((lx+16,y+26),"A RANK qualifies nobody for anything.",font=f(FS,12),fill=AMBER)

# chain
cx=400
d.text((cx,120),"THE CHAIN — administrative",font=f(FT,17),fill=AMBER)
d.rectangle([cx,152,cx+760,196],fill=(40,34,20),outline=AMBER)
d.text((cx+18,162),"CAPTAIN",font=f(FT,18),fill=AMBER)
d.text((cx+140,166),"decides · ship-wide access · accountable",font=f(FS,14),fill=FG)
d.line([cx+380,196,cx+380,222],fill=(90,96,110),width=2)
d.rectangle([cx,222,cx+760,262],fill=PANEL,outline=(70,76,90))
d.text((cx+18,230),"COMMANDER (XO)",font=f(FT,16),fill=TEAL)
d.text((cx+200,234),"sequences · owns the crew as a body: roster, welfare, morale, services",font=f(FS,13),fill=FG)
d.text((cx,282),"DEPARTMENT HEADS",font=f(FT,15),fill=DIM)
depts=[("Command","9","CO, XO · watch-officer pool · chief of the boat"),
       ("Operations","16","ops · conn · comms · transporters · sensors"),
       ("Engineering","36","warp core · EPS · hull · environmental · damage control"),
       ("Security & Tactical","18","armed response · brig · armoury · tactical systems"),
       ("Science","15","astrometrics · cartography · labs · archives"),
       ("Medical","12","sickbay · surgical bay · medics · the Doctor"),
       ("Flight & Shuttlebay","10","pilots · shuttle maintenance · deck crew"),
       ("Support & Logistics","19","cargo · galley · airponics · quarters · recreation"),
       ("Civilians & attached","6","morale officer · botanical aide · consultant · guests")]
dw=(760-24)//3; dh=104
y0=306
for i,(n,c,what) in enumerate(depts):
    r,cc=divmod(i,3)
    x=cx+cc*(dw+12); yy=y0+r*(dh+12)
    box(d,[x,yy,x+dw,yy+dh],fill=PANEL,outline=(58,64,78))
    d.text((x+12,yy+10),n,font=f(FT,14),fill=FG)
    d.text((x+dw-46,yy+9),c,font=f(FT,18),fill=AMBER)
    para(d,x+12,yy+34,what,f(FS,11),DIM,dw-24,15)
# operational chains
ox=1230
box(d,[ox,120,W-34,H-40],fill=PANEL)
d.text((ox+18,132),"OPERATIONAL CHAINS",font=f(FT,17),fill=AMBER)
d.text((ox+18,164),"They change with the state of the ship.",font=f(FS,13),fill=DIM)
ops=[("Officer of the deck","while a watch runs, the bridge watch officer commands the ship's operation — including officers senior to them who are not the captain. Waking the chief engineer at 0300 means the OOD is briefly giving orders three ranks up.",TEAL),
     ("Damage control","a repair party reports to the engineer of the watch, whatever anyone's rank. The chief engineer outranks them; the watch still runs the job.",VIOLET),
     ("Boarding","security owns the corridors. Everyone else is a body to be moved or a hatch to be held.",RED),
     ("Command vacancies","with the senior staff dead the administrative chain has a hole at the top, and the operational chain still works. That is why the ship can be run by whoever is left.",AMBER)]
y=196
for t,txt,col in ops:
    d.text((ox+18,y),t,font=f(FT,14),fill=col)
    y=para(d,ox+18,y+20,txt,f(FS,12),FG,W-ox-52,16)+14
d.line([ox+18,y+4,W-52,y+4],fill=(60,66,80))
d.text((ox+18,y+20),"SPECIAL CASES",font=f(FT,17),fill=AMBER)
y+=52
for t,txt,col in [("The Doctor","chief medical officer with NO rank. His authority is situational and explicit: sickbay, medical decisions, quarantine. It does not extend to the conn.",TEAL),
                  ("Neelix","a civilian holding a post — morale officer. Access by post, authority by none. He must be asked.",GREEN),
                  ("Field commissions","provisional ranks, flagged as such, which the crew remember and resent.",AMBER)]:
    d.text((ox+18,y),t,font=f(FT,13),fill=col)
    y=para(d,ox+18,y+18,txt,f(FS,12),FG,W-ox-52,15)+12
img.save("/home/robot/manifest-1-hierarchy.png"); print("hierarchy", img.size)


from PIL import Image, ImageDraw, ImageFont
FT="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"; FS="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FM="/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
BG=(12,13,17); PANEL=(20,23,30); FG=(236,238,244); DIM=(136,143,156)
AMBER=(240,168,72); TEAL=(96,196,222); GREEN=(104,196,140); RED=(226,96,84); VIOLET=(168,140,232)
def f(p,s): return ImageFont.truetype(p,s)
def wrap(d,t,font,maxw):
    out=[];cur=""
    for w in t.split():
        c=(cur+" "+w).strip()
        if d.textlength(c,font=font)<=maxw: cur=c
        else: out.append(cur); cur=w
    if cur: out.append(cur)
    return out
def para(d,x,y,t,font,fill,maxw,lh=None):
    lh=lh or font.size+5
    for i,l in enumerate(wrap(d,t,font,maxw)): d.text((x,y+i*lh),l,font=font,fill=fill)
    return y+len(wrap(d,t,font,maxw))*lh

# ---------------- panel 2 ----------------
W,H=1760,930
img=Image.new("RGB",(W,H),BG); d=ImageDraw.Draw(img)
d.rectangle([0,0,W,58],fill=TEAL)
d.text((26,16),"USS VOYAGER — THE WATCH   ·   THREE EIGHT-HOUR WATCHES   ·   WHO IS AWAKE",font=f(FT,22),fill=(10,20,24))
d.text((26,74),"A third of the crew is on duty at any moment.  A third of the POSTS are not staffed.",font=f(FS,16),fill=DIM)
bx,by,bw,bh=40,140,1680,96
d.text((bx,114),"THE DAY, SHIP TIME  —  0300 marked",font=f(FT,14),fill=DIM)
segs=[("GAMMA  0000–0800","night watch: a fraction of a fraction",(46,52,66)),
      ("ALPHA  0800–1600","day watch: the ship at full strength",(40,60,52)),
      ("BETA  1600–2400","evening: reduced but solid",(44,52,66))]
x=bx
for t,sub,col in segs:
    d.rectangle([x,by,x+556,by+bh],fill=col,outline=(70,78,92))
    d.text((x+16,by+16),t,font=f(FT,16),fill=FG)
    d.text((x+16,by+44),sub,font=f(FS,13),fill=DIM)
    x+=560
mx=bx+180
d.line([mx,by+4,mx,by+bh-4],fill=RED,width=3)
d.text((mx+8,by+bh-26),"0300 — the thin watch",font=f(FT,13),fill=RED)
tx,ty=40,272
d.text((tx,ty-26),"STATIONS BY WATCH",font=f(FT,16),fill=DIM)
cols=[("station",330),("ALPHA",300),("BETA",300),("GAMMA",300)]
rows=[("bridge watch officer (OOD)","lieutenant +","lieutenant (jg) +","ensign +"),
      ("conn","officer","officer","officer, junior"),("operations","officer + 1","officer","1"),
      ("tactical","officer + 1","officer","1"),("science or comms","officer","1","by call"),
      ("duty engineer","CPO + party","CPO + party","1 CPO + roving hand"),
      ("damage control party","4 on call","3 on call","2 on call"),
      ("security","patrol + brig watch","same","1 patrol + brig watch"),
      ("medical","medic + Doctor standby","medic","1 medic + EMH")]
x=tx
for c,w in cols: d.text((x+10,ty),c,font=f(FT,14),fill=TEAL); x+=w
y=ty+26
for i,r in enumerate(rows):
    if i%2==0: d.rectangle([tx,y,tx+1230,y+30],fill=PANEL)
    x=tx
    for j,(val,(_,w)) in enumerate(zip(r,cols)):
        d.text((x+10,y+7),val,font=f(FT if j==0 else FS,13),fill=FG if j==0 else DIM); x+=w
    y+=30
ox=1300
d.text((ox,ty-26),"WHO IS AWAKE AT 0300",font=f(FT,16),fill=RED)
y2=ty+4
for it in ["the officer of the deck","a conn officer, usually junior","one on operations","one on tactical",
           "the night engineer on rounds","a medic, and the EMH for anything worse",
           "one security patrol, plus the brig watch","one transporter room"]:
    d.text((ox+8,y2),"·",font=f(FT,16),fill=RED); d.text((ox+24,y2),it,font=f(FS,13),fill=FG); y2+=25
d.line([ox+6,y2+8,W-40,y2+8],fill=(60,66,80)); y2+=26
d.text((ox,y2),"WATCH-STANDERS vs DAY SPECIALISTS",font=f(FT,15),fill=AMBER); y2+=24
y2=para(d,ox+6,y2,"Bridge, engineering, operations, security and medical run around the clock, three deep. Science labs, administration, fabrication, quartermaster and airponics are Alpha-heavy, thin on Beta, and on call in Gamma.",f(FS,13),FG,W-ox-52,17)+14
para(d,ox+6,y2,"The night watch is thin by design, and waking someone off watch costs sleep, then morale, then tomorrow's performance at their post.",f(FS,13),AMBER,W-ox-52,17)
img.save("/home/robot/manifest-2-watch.png"); print("watch ok")

# ---------------- panel 3 ----------------
W,H=1760,900
img=Image.new("RGB",(W,H),BG); d=ImageDraw.Draw(img)
d.rectangle([0,0,W,58],fill=GREEN)
d.text((26,16),"USS VOYAGER — THE HAZARD TEAM   ·   FOURTEEN, TWO SQUADS   ·   THE AWAY-TEAM BILL",font=f(FT,22),fill=(8,20,14))
d.text((26,74),"Formed under Janeway after losses to boarders.  A standing unit that cuts across departments — not a department.",font=f(FS,16),fill=DIM)
ry=112
d.rectangle([40,ry,1720,ry+84],fill=PANEL,outline=(58,64,78))
d.text((56,ry+10),"REPORTING",font=f(FT,13),fill=GREEN)
d.text((150,ry+8),"CAPTAIN — forms it and tasks it",font=f(FT,14),fill=AMBER)
d.text((620,ry+8),"TUVOK — trains it, leads in person, tactical direction",font=f(FS,14),fill=TEAL)
d.text((150,ry+34),"LT. LES FOSTER — commands it, goes in first",font=f(FS,14),fill=FG)
d.text((620,ry+34),"FIRST OFFICER — owns the crew, so owns their casualties",font=f(FS,14),fill=VIOLET)
d.text((150,ry+58),"HOLODECK FIRING RANGE — where it trains",font=f(FS,13),fill=DIM)
d.text((620,ry+58),"THE AWAY-TEAM BILL — wherever the ship is not",font=f(FS,13),fill=DIM)
def squad(x,y,w,title,colour,members,note=None):
    rh=58; h=48+len(members)*rh+(30 if note else 0)
    d.rectangle([x,y,x+w,y+h],fill=PANEL,outline=colour,width=2)
    d.text((x+16,y+12),title,font=f(FT,17),fill=colour)
    yy=y+48
    for name,rank,role,trait in members:
        d.text((x+16,yy),name,font=f(FT,15),fill=FG)
        if rank: d.text((x+16+d.textlength(name,font=f(FT,15))+10,yy+3),rank,font=f(FS,12),fill=DIM)
        d.text((x+16,yy+21),role,font=f(FS,12),fill=colour)
        if trait: d.text((x+16,yy+38),trait,font=f(FS,12),fill=DIM)
        yy+=rh
    if note:
        d.text((x+16,yy+4),note,font=f(FS,12),fill=DIM)
    return y+h
alpha=[("Lt. Les Foster","","leader · trainer · goes in first","steady, methodical"),
       ("Ens. Alexander Munro","","second in command","headstrong; better leader than he looks"),
       ("Crewman Telsia Murphy","","scout and sniper","canon bond: Munro's longtime friend"),
       ("Crewman Austin Chang","","demolitionist","joined when the team formed"),
       ("Crewman Kendrick Biessman","","heavy weapons","sarcastic, cocky, loud, very good at it"),
       ("Crewman Chell","","technician (Bolian)","averse to fighting, complains, indispensable"),
       ("Crewman Juliet Jurot","","field medic (Betazoid)","fiercely logical, and empathic with it")]
beta=[("Ens. Elizabeth Laird","","","")]+[(f"Crewman {n}","","","") for n in
      ("Jeffrey Nelson","Perfecto Oviedo","Kenn Lathrop","Thomas Odell","Mitch Csatlos","Michael Jaworski")]
note="Roles and traits not specified in the game's lore — ours to fill."
hA=squad(40,222,830,"A-SQUAD",GREEN,alpha)
squad(898,222,822,"B-SQUAD",TEAL,beta,note)
cy=hA+28
for i,(t,txt,col) in enumerate([
    ("the away-team bill, made permanent","Everyone else's battle station is a place aboard. The Hazard Team's is wherever the ship is not.",GREEN),
    ("casualties hit three departments at once","A squad carries a technician, a medic, a demolitionist and a security specialist. One bad action costs engineering, medical and security together.",RED),
    ("two squads are a relief resource","A mission takes one and leaves the other. A bad mission leaves the ship with only B-squad — a worse team, and a demoralised one.",AMBER)]):
    w=(1680-40)//3; x=40+i*(w+20)
    d.rectangle([x,cy,x+w,cy+142],fill=PANEL,outline=(58,64,78))
    d.text((x+14,cy+10),t,font=f(FT,13),fill=col)
    para(d,x+14,cy+34,txt,f(FS,12),FG,w-28,16)
img.save("/home/robot/manifest-3-hazard-team.png"); print("hazard ok")
