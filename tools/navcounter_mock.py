#!/usr/bin/env python3
"""Mock of the navigation counter, as the reference for whoever builds the UI."""
from PIL import Image, ImageDraw, ImageFont
FT="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"; FS="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FM="/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
W,H=1500,860
BG=(8,10,14); PANEL=(18,21,28); AMBER=(240,168,72); TEAL=(96,196,222); FG=(232,236,244); DIM=(132,140,156); RED=(226,96,84)
img=Image.new("RGB",(W,H),BG); d=ImageDraw.Draw(img)
d.rectangle([0,0,W,54],fill=AMBER); d.rectangle([0,54,W,58],fill=PANEL)
d.text((24,16),"ASTROMETRICS  ·  USS VOYAGER  ·  NAVIGATION",font=ImageFont.truetype(FT,20),fill=(20,20,24))
d.text((W-330,20),"DECK 8  ·  STARDATE 51008.4",font=ImageFont.truetype(FM,14),fill=(20,20,24))
# the two big numbers
y=92
d.text((40,y),"DISTANCE TO EARTH",font=ImageFont.truetype(FT,20),fill=DIM)
d.text((40,y+30),"71,240",font=ImageFont.truetype(FT,84),fill=FG)
d.text((430,y+86),"light years",font=ImageFont.truetype(FS,20),fill=DIM)
d.text((40,y+150),"ESTIMATED TIME TO EARTH",font=ImageFont.truetype(FT,20),fill=DIM)
d.text((40,y+182),"71 yr",font=ImageFont.truetype(FT,54),fill=TEAL)
d.text((230,y+200),"nominal  (warp 6.2, healthy crystal)",font=ImageFont.truetype(FS,15),fill=DIM)
d.text((40,y+248),"76 yr",font=ImageFont.truetype(FT,54),fill=AMBER)
d.text((230,y+266),"at current capability",font=ImageFont.truetype(FS,15),fill=DIM)
# delta
d.rectangle([700,y+182,1180,y+310],outline=(60,64,76),width=1)
d.text((722,y+198),"SINCE LAST LOG ENTRY",font=ImageFont.truetype(FT,14),fill=DIM)
d.text((722,y+222),"▲ +1.8 yr",font=ImageFont.truetype(FT,42),fill=RED)
d.text((722,y+276),"crystal integrity down, overhaul pending,",font=ImageFont.truetype(FS,13),fill=DIM)
d.text((722,y+294),"no refuelling site charted",font=ImageFont.truetype(FS,13),fill=DIM)
# capability bars
bx=700; by=y+330
for i,(label,val,col) in enumerate([("DILITHIUM CRYSTAL INTEGRITY",0.62,AMBER),
                                    ("ENGINE HEALTH",0.78,TEAL),
                                    ("CREW CONDITION",0.91,TEAL)]):
    yy=by+i*44
    d.text((bx,yy),label,font=ImageFont.truetype(FT,13),fill=DIM)
    d.rectangle([bx,yy+20,bx+460,yy+32],fill=(30,33,42))
    d.rectangle([bx,yy+20,bx+int(460*val),yy+32],fill=col)
    d.text((bx+470,yy+18),"%d%%"%round(val*100),font=ImageFont.truetype(FM,14),fill=FG)
# route strip
ry=y+500
d.text((40,ry),"ROUTE",font=ImageFont.truetype(FT,14),fill=DIM)
d.rectangle([40,ry+24,W-40,ry+58],fill=(26,29,38))
d.rectangle([40,ry+24,410,ry+58],fill=(40,60,52))                 # travelled
d.text((54,ry+32),"3,760 ly covered",font=ImageFont.truetype(FS,14),fill=(150,200,175))
for x in range(410,W-40,26):                                       # uncharted
    d.line([x,ry+26,x+13,ry+56],fill=(58,62,74),width=2)
d.text((430,ry+32),"uncharted — scan to extend",font=ImageFont.truetype(FS,14),fill=DIM)
d.polygon([(410,ry+14),(424,ry-2),(438,ry+14)],fill=AMBER)
d.text((300,ry+66),"you are here",font=ImageFont.truetype(FS,13),fill=AMBER)
d.text((W-420,ry+66),"◆ charted dilithium candidate, 400 ly off course",font=ImageFont.truetype(FS,13),fill=TEAL)
d.text((40,H-88),"CREW CONSOLES MAY QUERY THIS PAGE. FORECASTS UNDER EACH AVAILABLE COURSE ARE COMMAND ONLY.",font=ImageFont.truetype(FM,14),fill=DIM)
d.text((40,H-60),"The number must never lie: shown is what the journey costs if nothing changes, and what it costs at the capability we have.",font=ImageFont.truetype(FS,14),fill=(96,102,114))
img.save("/home/robot/voyager-nav-counter.png"); print("saved", img.size)
