<CsoundSynthesizer>
<CsOptions>
-d -m0
</CsOptions>
<CsInstruments>
sr=48000
ksmps=32
nchnls=1
0dbfs=1
seed $SEED
instr 1
; Modal impact: independent inharmonic stone/bone/iron resonances and friction.
iFreq=p4
iTone=p5
aEnv expseg 1, p3, .0001
aNoise rand .5
aBody reson aNoise,iFreq, iFreq*.10,2
aHigh reson aNoise,iFreq*2.731,iFreq*.16,2
aRing oscili .13,iFreq*4.13
aClick butterhp aNoise,1800
out (aBody*2+aHigh+iTone*aRing+aClick*.12)*aEnv
endin
instr 2
; Cloth, scuttle and weapon swish with a moving bandpass and rounded envelope.
kFreq expseg p4*.4,p3*.35,p4*2,p3*.65,p4*.5
aNoise rand .6
aBand reson aNoise,kFreq,p4*.5,2
aEnv linseg 0,.004,1,p3*.2,.65,p3*.796,0
out aBand*aEnv
endin
instr 3
; Stick-slip timber/hinge friction. Quasiperiodic slips excite wood modes.
kPitch expseg p4,p3*.35,p4*1.4,p3*.65,p4*.65
kJitter randomi .75,1.3,27
aPulse vco2 .15,kPitch*kJitter,2,.12
aNoise rand .16
aWood reson aPulse+aNoise,380,40,2
aWood2 reson aPulse+aNoise,970,75,2
aIron reson aNoise,2310,30,2
aEnv linseg 0,.015,1,p3-.065,.55,.05,0
out (aWood*2+aWood2+aIron*p5)*aEnv
endin
instr 4
; Creature vocal tract: jittered glottal source, breath and three formants.
kPitch expseg p4*1.2,p3*.25,p4,p3*.75,p4*.62
kJitter randomi .92,1.08,38
aGlottal vco2 .17,kPitch*kJitter,2,.22
aBreath rand .12
aF1 reson aGlottal+aBreath,p5,90,2
aF2 reson aGlottal+aBreath,p5*2.6,140,2
aF3 reson aGlottal+aBreath,p5*4.2,220,2
aEnv linseg 0,.012,1,p3*.35,.8,p3*.65-.032,0,.02,0
out (aF1*3+aF2*2+aF3)*aEnv
endin
instr 5
; Spell layer: FM shimmer, filtered turbulence and a moving tonal body.
kEnv expseg 1,p3,.0001
kFreq expseg p4,p3,p4*p5
aFM foscil .2,kFreq,1,2.71,3,1
aNoise rand .4
aCloud reson aNoise,kFreq*2, max(100,kFreq),2
aEnv linseg 0,.008,1,p3-.028,.4,.02,0
out (aFM+aCloud*.5)*kEnv*aEnv
endin
instr 6
; Restrained room air with slowly varying low resonances, no horror drone.
aNoise rand .3
aLow butterlp aNoise,420
aAir butterhp aLow,65
kLevel oscili .12,.07,1
aRoom reson aNoise,121,18,2
out aAir*(.6+kLevel)+aRoom*.1
endin
instr 7
; Long quiet fire/spectral sustain, reserved for current persistent effects.
aNoise rand .4
aFire butterlp aNoise,p4
kAmp randomi .3,.8,9
aTone oscili .02,p5,1
out aFire*kAmp+aTone
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1
$SCORE
</CsScore>
</CsoundSynthesizer>
