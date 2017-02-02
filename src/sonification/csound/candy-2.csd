<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1.0

;VOICES
gisine ftgen 11, 0, 16384, 10, 1
gisine2 ftgen 12, 0, 16384, 10, 1, .2, .08, .07
gisaw ftgen 13, 0, 16384, 10, 1, 0.5, 0.3, 0.25, 0.2, 0.167, 0.14, 0.125, .111 
girect ftgen 14, 0, 16384, 10, 1, 0, 0.3, 0, 0.2, 0, 0.14, 0, .111
;  f4 0 16384 10 1 0   0.3 0    0.2 0     0.14 0     .111   ; Square
;  f5 0 16384 10 1 1   1   1    0.7 0.5   0.3  0.1          ; Pulse

;global variables
giOSC OSCinit 7000

gkvariables init 0
gkdecisions init 0
gklearnt init 0
gkbacktrack init 0
gkconflictlevel init 0
gkassigns init 0

gkvariables_upd init 0
gkdecisions_upd init 0
gklearnt_upd init 0
gkbacktrack_upd init 0
gkconflictlevel_upd init 0
gkassigns_upd init 0

gkdecisions_hwm init 0

;ROUTING
connect "sampler", "out", "mixer", "mix1_sampler"
connect "decision_level", "out", "mixer", "mix2_decision_level"
connect "backtrack_level", "out", "mixer", "mix3_backtrack_level"
connect "learnt_size", "out", "mixer", "mix4_learnt_size"
connect "conflict", "out", "mixer", "mix5_conflict"
connect "eloquence", "out", "mixer", "mix6_eloquence"

;ALWAYSON
alwayson "oscReceiver"
alwayson "restart_trigger"
alwayson "decision_level"
;alwayson "backtrack_level"
alwayson "learnt_size"
alwayson "conflict_trigger"
alwayson "eloquence"

;restart
instr sampler
  ifn = p4
  idur = ftsr(ifn) / ftlen(ifn)
  aSamp poscil3 3, idur, ifn
  outleta "out", aSamp
endin

;restart-trigger 
instr restart_trigger
  kreceive init 1
  kevent OSClisten giOSC, "/restart", "f", kreceive
  if (kevent > 0) then
    event "i", "sampler", 0, 0.5, 1
  endif
endin

instr oscReceiver
  kreceive1 init 1
  kreceive2 init 1
  kreceive3 init 1
  kreceive4 init 1
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
  
  gkbacktrack_upd OSClisten giOSC, "/backtrack", "f", kreceive4
  if (gkbacktrack_upd == 1) then
    gkbacktrack = kreceive4
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

;instrument: decision level meter
instr decision_level
  kvar init 0
  if gkdecisions_hwm > 0 then
    kvar = 440 + (gkdecisions * 258) / gkdecisions_hwm
  endif
  adecision oscil 1, kvar, gisaw
  outleta "out", adecision
endin

;instrument: backtrack level meter
instr backtrack_level
  kvar init 0
  if gkvariables > 0 then
    kvar = 440 + (gkbacktrack * 258) / gkvariables
  endif
  abacktrack oscil 1, kvar, gisine2
  outleta "out", abacktrack
endin

instr learnt_size 
  if (gklearnt_upd == 1) then 
    if (gklearnt == 1) then
      event "i", "sampler", 0, 0.5, 2
    elseif (gklearnt == 2) then
      event "i", "sampler", 0, 0.5, 3
    elseif (gklearnt == 3) then
      event "i", "sampler", 0, 0.5, 4
    elseif (gklearnt > 10) then
      gklearnt = 10
    endif
  endif
  kvar = 16000 - ((gklearnt * 16000) / 10)
  awhite unirand 16200
  asound oscil 1, awhite, gisaw
  afilt butterlp asound, kvar
  outleta "out", afilt
endin

instr eloquence
  kfreq init 0
  ;asig gbuzz 1, 440, 10, 1, .9, 11
  if gkassigns > 0 then
    kfreq = 440 + (gkdecisions * 258) / gkassigns
  endif
  ;alow, ahigh, aband svfilter asig, kfreq, 490  
  ;asum = alow * 1 + ahigh * 0 + aband * 0
  asum oscil 1, kfreq, girect
  outleta "out", asum;
endin

instr conflict
  kdl init 0
  ilength init 0
  if gkdecisions_hwm > 0 then
    kscale = (gkdecisions * 100) / gkdecisions_hwm
  endif 
  if (kscale > 75) then
    kdl =  698.456 ;f
    ilength = .35
  elseif (kscale > 50) then
    kdl = 659.255 ;e
    ilength = .3
  elseif (kscale > 25) then 
    kdl = 587.330 ;d
    ilength = .25
  else
    kdl = 440 ;a
    ilength = .2
  endif
  aenv expon 2, ilength, 0.0001
  aSig oscil aenv, kdl, gisaw
  outleta "out", aSig 
endin

instr conflict_trigger
  ;kcount init 0
  if (gkconflictlevel_upd > 0) then
    ;kcount = kcount + 1
    ;if (kcount > 10) then	
      event "i", "conflict", 0, 0.1
    ;  kcount = 0
    ;endif
  endif
endin

instr mixer
  ach1 inleta "mix1_sampler"
  ach2 inleta "mix2_decision_level"
  ach3 inleta "mix3_backtrack_level"
  ach4 inleta "mix4_learnt_size"
  ach5 inleta "mix5_conflict"
  ach6 inleta "mix6_eloquence"
  
  kch1vol init 0
  kch2vol init 0
  kch3vol init 0
  kch4vol init 0
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
    kch3vol = 0 ;backtrack_level
    kch4vol = .7
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

</CsInstruments>

<CsScore>
; NUM | INIT_TIME | SIZE | GEN_ROUTINE |  FILE_PATH   | IN_SKIP | FORMAT | CHANNEL
f  1      0          0       1      "samples/46.WAV"     0         4         1
f  2      0          0       1      "samples/23.WAV"     0         4         1
f  3      0          0       1      "samples/26.WAV"     0         4         1
f  4      0          0       1      "samples/36.WAV"     0         4         1
;  f2 0 16384 10 1                                          ; Sine
;  f3 0 16384 10 1 0.5 0.3 0.25 0.2 0.167 0.14 0.125 .111   ; Sawtooth
;  f4 0 16384 10 1 0   0.3 0    0.2 0     0.14 0     .111   ; Square
;  f5 0 16384 10 1 1   1   1    0.7 0.5   0.3  0.1          ; Pulse
;  i "exit_listener" 0 40000
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
