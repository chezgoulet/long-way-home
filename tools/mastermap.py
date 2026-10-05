#!/usr/bin/env python3
"""The master map: fifteen decks, what exists, what is missing, and what each hosts."""
from PIL import Image, ImageDraw, ImageFont

FT="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FS="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FM="/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
BG=(14,15,19); FG=(238,238,242); DIM=(140,146,158)
BUILT=(64,180,120); PART=(226,170,60); GONE=(96,100,110); SYSTEM=(90,160,230)

DECKS=[
 (1,"Bridge, ready room, briefing room, aft cargo","command; weapons locker","built"),
 (2,"Mess hall, captain's dining room, officers' quarters","galley / replicators","built"),
 (3,"Captain's and officers' quarters","crew life","built"),
 (4,"Transporter rooms 1 & 2, cargo bay 2, gel pack grid","transporters; stores","built"),
 (5,"Sickbay, CMO's office, sections 10-53","medical","built"),
 (6,"Holodeck 2, armory, crew quarters","holodeck; security post","gone"),
 (7,"Auxiliary computer core, cargo bays 1-2, science labs, escape pods","computer; escape","gone"),
 (8,"Astrometrics, cargo bay 2 sec 4, science lab, quarters","sensors; stores","built"),
 (9,"Crew quarters","crew life; aerowing dock","built"),
 (10,"Shuttlebay, junction 32 Alpha","shuttlecraft; computer core (planned)","built"),
 (11,"Main engineering, airponics, deflector control","warp core; EPS; power","built"),
 (12,"Environmental control, navigational control B7, section 42","LIFE SUPPORT","gone"),
 (13,"Life support plant, roughly 10 m below engineering","life support machinery","gone"),
 (14,"Stasis chambers, holodeck 1","stasis; holodeck","gone"),
 (15,"Plasma relay room 16, Jefferies tube G-33, landing gear","plasma relays; landing","built"),
]
BUILD_ORDER=[
 ("Deck 12","environmental control","1 - atmosphere is unlocated without it"),
 ("Deck 13","life support machinery","1 - the other half of the same system"),
 ("Deck 10","main computer core","2 - gel packs; the Borg's best target"),
 ("Deck 8","deuterium processing","3 - fuel becomes a place"),
 ("Deck 4","cargo bay 2","4 - stores and the decompression precedent"),
 ("Deck 6","security office, holodeck 2","5 - the security post"),
 ("Deck 14","stasis, holodeck 1","6 - decide the contested placement"),
 ("Deck 7","aux core, labs, escape pods","7 - depth, not dependency"),
 ("1/2/3/5/9","amenity items","8 - campaign maps already have most"),
]
W,H=1680,1010
img=Image.new("RGB",(W,H),BG); d=ImageDraw.Draw(img)
d.text((28,20),"USS VOYAGER — MASTER MAP",font=ImageFont.truetype(FT,30),fill=FG)
d.text((28,58),"fifteen decks · left: what exists and what each hosts · right: build order and the lift spine",
       font=ImageFont.truetype(FS,15),fill=DIM)
y0=100; row=54
d.text((28,y0-24),"DECK   STATUS      CONTENTS                                         HOSTS",font=ImageFont.truetype(FM,13),fill=DIM)
for i,(num,contents,hosts,status) in enumerate(DECKS):
    y=y0+i*row
    col={"built":BUILT,"gone":GONE,"part":PART}[status]
    d.rectangle([28,y+6,34,y+row-12],fill=col)
    d.text((48,y+8),"D%-2d"%num,font=ImageFont.truetype(FT,17),fill=FG)
    label={"built":"map ships","gone":"no map","part":"partial"}[status]
    d.text((96,y+11),label,font=ImageFont.truetype(FM,12),fill=col)
    d.text((196,y+11),contents[:84],font=ImageFont.truetype(FS,13),fill=FG if status!="gone" else (168,172,182))
    d.text((196,y+30),hosts[:72],font=ImageFont.truetype(FS,12),fill=SYSTEM)
# right column
x=880
d.text((x,80),"BUILD ORDER — by system dependency, not deck number",font=ImageFont.truetype(FT,17),fill=FG)
for i,(deck,what,why) in enumerate(BUILD_ORDER):
    y=112+i*44
    d.text((x,y),deck,font=ImageFont.truetype(FT,14),fill=(226,170,60))
    d.text((x+92,y),what,font=ImageFont.truetype(FS,14),fill=FG)
    d.text((x+92,y+18),why,font=ImageFont.truetype(FS,12),fill=DIM)
d.text((x,540),"THE LIFT SPINE — 139 level-change edges",font=ImageFont.truetype(FT,17),fill=FG)
for i,line in enumerate([
  "every VV deck links to every other VV deck, plus:",
  "   _brig            the brig, and the security adjacency",
  "   voy15            a campaign interior, already wired in",
  "   eight holodeck programmes reachable from deck04:",
  "      camelot · garden · high noon · temple · warlord",
  "      proton · firing range · minigame",
  "a new deck with no incoming edge is unreachable",
  "deck04 declares the most destinations: copy its pattern"]):
    d.text((x,570+i*22),line,font=ImageFont.truetype(FS,13),fill=FG if i==0 or i>5 else DIM)
d.text((x,790),"SCENARIO SITES",font=ImageFont.truetype(FT,17),fill=FG)
for i,line in enumerate([
  "env control + section 42 next door  → the haunting",
  "cargo bay 2                         → explosive decompression",
  "transporter rooms                   → boarding arrives here",
  "sickbay                             → the wounded become drones",
  "computer core                       → they inherit the crew roster",
  "deuterium processing                → the price of fuel",
  "plasma relays                       → nineteen relays, no turbolifts"]):
    d.text((x,822+i*21),line,font=ImageFont.truetype(FS,13),fill=DIM)
d.text((28,H-30),"built = the game ships this deck · no map = absent and on the build order · generated from canon deck list + the game's own map sources",
       font=ImageFont.truetype(FS,12),fill=(110,114,124))
img.save("/home/robot/voyager-master-map.png")
print("saved", img.size)
