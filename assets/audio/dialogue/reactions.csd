<CsoundSynthesizer>
<CsOptions>
-d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
; Provisional voiced reactions. No external recordings, model or impersonation.
; p4 is fundamental, p5 is laugh pulse rate (zero selects a falling groan).
instr 1
kEnv linseg 0, .015, 1, p3-.115, .8, .1, 0
kPitch linseg p4*1.12, p3*.45, p4, p3*.55, p4*.72
kPulse init 1
if p5 > 0 then
kSyll oscili .5, p5, 1
kPulse = max(0, kSyll+.35)
endif
aSource vco2 .15, kPitch, 0
aLow reson aSource, 520, 120, 1
aMid reson aSource, 1050, 160, 1
aHigh reson aSource, 2300, 210, 1
aVoice = (aLow*.65+aMid*.25+aHigh*.1)*kPulse*kEnv
out tanh(aVoice*2)*.4
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1
; Three separate one-second clips with silent separation for offline extraction.
i 1 0 1.0 115 6.5
i 1 2 0.9 155 7.5
i 1 4 1.1 95 0
e
</CsScore>
</CsoundSynthesizer>
