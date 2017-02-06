<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1.0

; SAMPLES
gibd       ftgen 1, 0, 0, 1, "samples/48.WAV", 0, 4, 1;0.31
gihd       ftgen 2, 0, 0, 1, "samples/40.WAV", 0, 4, 1;0.58
givoicehit ftgen 3, 0, 0, 1, "samples/26.WAV", 0, 4, 1;0.50
girvbd     ftgen 4, 0, 0, 1, "samples/36.WAV", 0, 4, 1;0.24
gieff1     ftgen 5, 0, 0, 1, "samples/12.WAV", 0, 4, 1;0.41
gieff2     ftgen 6, 0, 0, 1, "samples/14.WAV", 0, 4, 1;0.25
gibrrr     ftgen 7, 0, 0, 1, "samples/18.WAV", 0, 4, 1;0.25
girims     ftgen 8, 0, 0, 1, "samples/29.WAV", 0, 4, 1;0.04
gidistbd   ftgen 9, 0, 0, 1, "samples/38.WAV", 0, 4, 1;0.46
; FUNCTIONS
;f # time size 11 nh [lh] [r]
giharms ftgen 10, 0, 16384, 11, 10, 1, .7
gisaw   ftgen 11, 0, 16384, 10, 1, 0.5, 0.3, 0.25, 0.2, 0.167, 0.14, 0.125, .111
ginoise ftgen 12, 0, 16384, 21, 4
;gisine  ftgen 13, 0, 16384, 10, 1
;gisine2 ftgen 14, 0, 16384, 10, 1, .2, .08, .07
;girect  ftgen 15, 0, 16384, 10, 1, 0, 0.3, 0, 0.2, 0, 0.14, 0, .111

;GLOBALS
giOSC OSCinit 7000

gkvariables init 0
gkdecisions init 0
gklearnt init 0
gkconflictlevel init 0
gkassigns init 0

gkvariables_upd init 0
gkdecisions_upd init 0
gklearnt_upd init 0
gkconflictlevel_upd init 0
gkassigns_upd init 0

gkdecisions_hwm init 0

;ROUTING
connect "sampler", "out", "mixer", "mix1_sampler"
connect "decision_level", "out", "mixer", "mix2_decision_level"
connect "learnt", "out", "mixer", "mix4_learnt"
connect "conflict", "out", "mixer", "mix5_conflict"
connect "eloquence", "out", "mixer", "mix6_eloquence"

;ALWAYSON
alwayson "oscReceiver"
alwayson "restart_trigger"
alwayson "decision_level"
alwayson "learnt_trigger"
alwayson "conflict_trigger"
alwayson "eloquence"

cpuprc "learnt", 10

instr decision_level
  kvar init 0
  if gkdecisions_hwm > 0 then
    kvar = 440 + (gkdecisions * 258) / gkdecisions_hwm
  endif
  adecision oscil .5, kvar, gisaw
  outleta "out", adecision
endin

;restart
instr sampler
  ifn = p4
  idur = ftsr(ifn) / ftlen(ifn)
  aSamp poscil3 2, idur, ifn
  outleta "out", aSamp
endin

;restart-trigger 
instr restart_trigger
  kreceive init 1
  kevent OSClisten giOSC, "/restart", "f", kreceive
  if (kevent > 0) then
    event "i", "sampler", 1, .31, gibd
  endif
endin

instr learnt
  ;aenv expon 1, .5, 0.0001
  aenv linen 2, .2, p3, .1
  ilowpass = 16000 - (16000 / 6 * i(gklearnt))
  anoise oscil aenv, 440, ginoise
  aout butterlp anoise, ilowpass
  outleta "out", aout
endin

instr learnt_trigger 
  if (gklearnt_upd == 1) then 
    if (gklearnt == 1) then
      event "i", "sampler", 1, .41, gieff1
    elseif (gklearnt == 2) then
      event "i", "sampler", 1, .24, girvbd
    elseif (gklearnt > 6) then
      gklearnt = 6
    endif
    event "i", "learnt", 0, .2
  endif
endin

instr eloquence
  kfreq init 0
  if gkassigns > 0 then
    kfreq = 440 + (gkdecisions * 258) / gkassigns
  endif
  aharms oscil 1, kfreq, giharms
  ;alow butterlp aharms, kfreq
  outleta "out", aharms;
endin

instr conflict
  aenv expon 1, p3, 0.0001
  aSig oscil aenv, p4, gisaw
  outleta "out", aSig 
endin

instr conflict_trigger
  kfrq init 0
  kdur init 0
  if (gkconflictlevel_upd > 0) then
    if gkdecisions_hwm > 0 then
      kscale = (gkdecisions * 100) / gkdecisions_hwm
    endif 
    if (kscale > 75) then
      kfrq =  698.456 ;f
      kdur = .45
    elseif (kscale > 50) then
      kfrq = 659.255 ;e
      kdur = .4
    elseif (kscale > 25) then 
      kfrq = 587.330 ;d
      kdur = .35
    else
      kfrq = 440 ;a
      kdur = .3
    endif
    event "i", "conflict", 0, kdur, kfrq
  endif
endin

instr mixer
  ach1 inleta "mix1_sampler"
  ach2 inleta "mix2_decision_level"
  ach3 inleta "mix3"
  ach4 inleta "mix4_learnt"
  ach5 inleta "mix5_conflict"
  ach6 inleta "mix6_eloquence"
  
  kch1vol init 0
  kch2vol init 0
  kch3vol init 0
  kch4vol init 1
  kch5vol init 0
  kch6vol init 0
  
  kreceive0 init 1
  kreceive1 init 1
  kreceive2 init 1
  kreceive3 init 1
  kreceive4 init 1
  kreceive5 init 1
  kreceive6 init 1
  
  kstart OSClisten giOSC, "/start", "f", kreceive0
  kstop OSClisten giOSC, "/stop", "f", kreceive0
  
  if (kstart == 1) then 
    kch1vol = 1
    kch2vol = 1
    kch3vol = 0 ;unassigned
    kch4vol = 1
    kch5vol = 1
    kch6vol = 1
  endif
  
  if (kstop == 1) then
    scoreline {{ e 1 }}, 1
    kch1vol = 0
    kch2vol = 0
    kch3vol = 0
    kch4vol = 0
    kch5vol = 0
    kch6vol = 0
  endif
  
  kans1 OSClisten giOSC, "/volume/ch1", "f", kreceive1
  if (kans1 == 1) then
    kch1vol = kreceive1
  endif
  kans2 OSClisten giOSC, "/volume/ch2", "f", kreceive2
  if (kans2 == 1) then
    kch2vol = kreceive2
  endif
  kans3 OSClisten giOSC, "/volume/ch3", "f", kreceive3
  if (kans3 == 1) then
    kch3vol = kreceive3
  endif
  kans4 OSClisten giOSC, "/volume/ch4", "f", kreceive4
  if (kans4 == 1) then
    kch4vol = kreceive4
  endif
  kans5 OSClisten giOSC, "/volume/ch5", "f", kreceive5
  if (kans5 == 1) then
    kch5vol = kreceive5
  endif
  kans6 OSClisten giOSC, "/volume/ch6", "f", kreceive6
  if (kans6 == 1) then
    kch6vol = kreceive6
  endif
  
  aout sum ach1 * kch1vol, ach2 * kch2vol, ach3 * kch3vol, ach4 * kch4vol, ach5 * kch5vol, ach6 * kch6vol
  outs aout, aout
endin

instr oscReceiver
  kreceive1 init 1
  kreceive2 init 1
  kreceive3 init 1
  kreceive5 init 1
  kreceive6 init 1
  
  gkvariables_upd OSClisten giOSC, "/variables", "f", kreceive1
  if (gkvariables_upd == 1) then
    gkvariables = kreceive1
  endif
  
  gkdecisions_upd OSClisten giOSC, "/decision", "f", kreceive2
  if (gkdecisions_upd == 1) then
    gkdecisions = kreceive2
    if (gkdecisions > gkdecisions_hwm) then
      gkdecisions_hwm = gkdecisions
    endif
  endif
  
  gklearnt_upd OSClisten giOSC, "/learnt", "f", kreceive3
  if (gklearnt_upd == 1) then
    gklearnt = kreceive3
  endif
  
  gkconflictlevel_upd OSClisten giOSC, "/conflict", "f", kreceive5
  if (gkconflictlevel_upd == 1) then
    gkconflictlevel = kreceive5
  endif
  
  gkassigns_upd OSClisten giOSC, "/assignments", "f", kreceive6
  if (gkassigns_upd == 1) then
    gkassigns = kreceive6
  endif
endin

</CsInstruments>

<CsScore>
  i "mixer" 0 40000
  e
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
