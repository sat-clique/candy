<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 8
nchnls = 2
0dbfs = 1.0

;load samples
giSample ftgen 1, 0, 0, 1, "bd.wav", 0, 0, 0

;define instruments
gisine ftgen 0, 0, 16384, 10, 1
gisaw ftgen 3, 0, 16384, 10, 1, 0.5, 0.3, 0.25, 0.2, 0.167, 0.14, 0.125, .111 
;  f4 0 16384 10 1 0   0.3 0    0.2 0     0.14 0     .111   ; Square
;  f5 0 16384 10 1 1   1   1    0.7 0.5   0.3  0.1          ; Pulse

;open osc port
giOSC OSCinit 7000

;routing
connect "restart", "out", "mixer", "in1"
connect "decision_level", "out", "mixer", "in2"
connect "backtrack_level", "out", "mixer", "in3"
connect "backtrack_level_continuous", "out", "mixer", "in4"

;always on instruments
alwayson "exit_listener"
alwayson "restart_trigger"
alwayson "decision_level"
alwayson "backtrack_level_continuous"
alwayson "backtrack_trigger"

;controller 1: turnoff all instruments
instr exit_listener
  kdone init -2
  kans OSClisten giOSC, "/done", "f", kdone

  if (kdone > -2) then
    scoreline"e 1", 1
  endif
endin

;sample 1: restart
instr restart
  itablen = ftlen(giSample) ;length of the table
  idur = itablen / sr ;duration
  aSamp poscil3 5, 1/idur, giSample
  outleta "out", aSamp
endin

;controller 2: trigger restarts
instr restart_trigger
  krestart init 1
  ktrig3 OSClisten giOSC, "/restart", "f", krestart
  
  if (krestart > 0) then
    event "i", "restart", 0, 0.23
    krestart = 0
  endif
endin

opcode oVariables, k, 0 
  klevel init 1
  kans OSClisten giOSC, "/variables", "f", klevel
  xout klevel
endop

opcode oDecision, k, 0 
  klevel init 0
  kans OSClisten giOSC, "/decision", "f", klevel
  xout klevel
endop

opcode oBacktrack, k, 0 
  klevel init 0
  kans OSClisten giOSC, "/backtrack", "f", klevel
  xout klevel
endop

opcode oDecByVar, k, 0
  kdecision oDecision
  kvariables oVariables
  xout 200 + ((kdecision * 16000) / kvariables)
endop

opcode oBackByVar, k, 0
  kdecision oDecision
  kvariables oVariables
  xout 200 + ((kdecision * 16000) / kvariables)
endop

;instrument: decision level meter
instr decision_level
  kvar oDecByVar
  adecision oscil 1, kvar, gisine
  outleta "out", adecision
endin

;instrument: backtrack level meter
instr backtrack_level_continuous
  kvar oBackByVar
  abacktrack oscil 1, kvar, gisine
  outleta "out", abacktrack
endin

instr backtrack_level
  aenv expon 2, 0.3, 0.0001
  aSig oscilaenv, p4, gisaw
  outleta "out", aSig 
endin

instr backtrack_trigger
  kbacktrack init 0
  kans OSClisten giOSC, "/backtrack", "f", kbacktrack
  /*
  kvariables init 1
  kans OSClisten giOSC, "/variables", "f", kvariables
  */
  kcount init 0
  if (kbacktrack > 0) then
    kcount = kcount + 1
    kbacktrack = 0
    prints "%i", kcount
    if (kcount > 100) then
      event "i", "backtrack_level", 0, 0.3, 100 ;+ ((kbacktrack * 16000) / kvariables)
      kcount = 0
    endif
  endif
endin


instr mixer
  ain1 inleta "in1"
  ain2 inleta "in2"
  ain3 inleta "in3"
  ain4 inleta "in4"
  outs ain1 + ain2 + ain3 + ain4, ain1 + ain2 + ain3 + ain4
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
