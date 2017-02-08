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
giSample ftgen 1, 0, 0, 1, "bd.wav", 0, 0, 0
gisine ftgen 2, 0, 16384, 10, 1
gisine2 ftgen 2, 0, 16384, 10, 1, .2, .08, .07
gisaw ftgen 3, 0, 16384, 10, 1, 0.5, 0.3, 0.25, 0.2, 0.167, 0.14, 0.125, .111 
girect ftgen 4, 0, 16384, 10, 1, 0, 0.3, 0, 0.2, 0, 0.14, 0, .111
;  f4 0 16384 10 1 0   0.3 0    0.2 0     0.14 0     .111   ; Square
;  f5 0 16384 10 1 1   1   1    0.7 0.5   0.3  0.1          ; Pulse

;OSC
giOSC OSCinit 7000

;ROUTING
connect "restart", "out", "mixer", "mixin1"
connect "decision_level", "out", "mixer", "mixin2"
connect "backtrack_level", "out", "mixer", "mixin3"
connect "learnt_size", "out", "mixer", "mixin4"
connect "conflict", "out", "mixer", "mixin5"
connect "eloquence", "out", "mixer", "mixin6"

;ALWAYSON
alwayson "exit_listener"
alwayson "restart_trigger"
alwayson "decision_level"
alwayson "backtrack_level"
alwayson "learnt_size"
alwayson "conflict_trigger"
alwayson "eloquence"

;turnoff all instruments
instr exit_listener
  kdone init -2
  kquit init 0
  kans OSClisten giOSC, "/done", "f", kdone

  if (kdone > -2) then
    ktrig metro 1000
    if (ktrig > 0) then
      kquit = kquit + 1
    endif
    if (kquit > 1000) then
      scoreline {{ e 1 }}, ktrig
    endif
  endif
endin

;restart
instr restart
  itablen = ftlen(giSample) ;length of the table
  idur = itablen / sr ;duration
  aSamp poscil3 5, 1/idur, giSample
  outleta "out", aSamp
endin

;restart-trigger 
instr restart_trigger
  krestart init 1
  ktrig3 OSClisten giOSC, "/restart", "f", krestart
  
  if (krestart > 0) then
    event "i", "restart", 0, 0.23
    krestart = 0
  endif
endin

opcode oVariables, k, 0 
  kreceive init 1
  gknvars init 1
  kans OSClisten giOSC, "/variables", "f", kreceive
  if (kans == 1) then
    gknvars = kreceive
  endif
  xout gknvars
endop

opcode oDecision, k, 0 
  kreceive init 1
  kans OSClisten giOSC, "/decision", "f", kreceive
  if (kans == 1) then
    gkndec = kreceive
  endif
  xout gkndec
endop

opcode oBacktrack, k, 0
  kreceive init 1
  kans OSClisten giOSC, "/backtrack", "f", kreceive
  if (kans == 1) then
    gknback = kreceive
  endif
  xout gknback
endop

opcode oConflict, k, 0 
  kreceive init 1
  kans OSClisten giOSC, "/conflict", "f", kreceive
  if (kans == 1) then
    gknconfl = kreceive
  endif
  xout gknconfl
endop

opcode oAssigns, k, 0 
  kreceive init 1
  gknass init 1
  kans OSClisten giOSC, "/assignments", "f", kreceive
  if (kans == 1) then
    gknass = kreceive
  endif
  xout gknass
endop

opcode oLearnt, k, 0 
  kreceive init 1
  kans OSClisten giOSC, "/learnt", "f", kreceive
  if (kans == 1) then
    gknlearnt = kreceive
  endif
  xout gknlearnt
endop

;instrument: decision level meter
instr decision_level
  kdecision oDecision
  kvariables oVariables
  kvar = 200 + ((kdecision * 16000) / kvariables)
  adecision oscil 1, kvar, gisine2
  outleta "out", adecision
endin

;instrument: backtrack level meter
instr backtrack_level
  kbacktrack oBacktrack
  kvariables oVariables
  kvar = 200 + ((kbacktrack * 16000) / kvariables)
  abacktrack oscil 0.3, kvar, gisaw
  outleta "out", abacktrack
endin

instr learnt_size 
  klearnt oLearnt
  if (klearnt > 10) then 
    klearnt = 10
  endif
  kvar = 16200 - ((klearnt * 16000) / 10)
  ;printk2 kvar
  awhite unirand 16200
  asound oscil 1, awhite, gisaw
  afilt butterlp asound, kvar
  outleta "out", afilt
endin

instr eloquence
  kdecision oDecision
  kassigns oAssigns
  kvar = 200 + ((kdecision * 16000) / kassigns)
  aeloquence oscil 1, kvar, girect
  outleta "out", aeloquence
endin

instr conflict
  aenv expon 2, 0.3, 0.0001
  aSig oscil aenv, p4, gisaw
  outleta "out", aSig 
endin

instr conflict_trigger
  kvar oConflict
  kcount init 0
  if (kvar > 0) then
    kcount = kcount + 1
    kvar = 0
    if (kcount > 1000) then
      event "i", "conflict", 0, 0.1, 100
      kcount = 0
    endif
  endif
endin


instr mixer
  ach1 inleta "mixin1"
  ach2 inleta "mixin2"
  ach3 inleta "mixin3"
  ach4 inleta "mixin4"
  ach5 inleta "mixin5"
  ach6 inleta "mixin6"
  ;BEGIN: VOLUME SHIT 
  kch1vol init 1
  kch2vol init 0
  kch3vol init 0
  kch4vol init 1
  kch5vol init 1
  kch6vol init 1
  kans OSClisten giOSC, "/volume/ch1", "f", kch1vol
  kans OSClisten giOSC, "/volume/ch2", "f", kch2vol
  kans OSClisten giOSC, "/volume/ch3", "f", kch3vol
  kans OSClisten giOSC, "/volume/ch4", "f", kch4vol
  kans OSClisten giOSC, "/volume/ch5", "f", kch5vol
  kans OSClisten giOSC, "/volume/ch6", "f", kch6vol
  ;END: VOLUME SHIT
  aout sum ach1 * kch1vol, ach2 * kch2vol, ach3 * kch3vol, ach4 * kch4vol, ach5 * kch5vol, ach6 * kch6vol
  outs aout, aout
endin

</CsInstruments>
<CsScore>
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
