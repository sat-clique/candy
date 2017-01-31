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
giSample1 ftgen 1, 0, 0, 1, "samples/46.WAV", 0, 0, 0
giSample2 ftgen 1, 0, 0, 1, "samples/43.WAV", 0, 0, 0
giSample3 ftgen 1, 0, 0, 1, "samples/bd.wav", 0, 0, 0
gisine ftgen 2, 0, 16384, 10, 1
gisine2 ftgen 2, 0, 16384, 10, 1, .2, .08, .07
gisaw ftgen 3, 0, 16384, 10, 1, 0.5, 0.3, 0.25, 0.2, 0.167, 0.14, 0.125, .111 
girect ftgen 4, 0, 16384, 10, 1, 0, 0.3, 0, 0.2, 0, 0.14, 0, .111
;  f4 0 16384 10 1 0   0.3 0    0.2 0     0.14 0     .111   ; Square
;  f5 0 16384 10 1 1   1   1    0.7 0.5   0.3  0.1          ; Pulse

;OSC
giOSC OSCinit 7000

gkch1vol init 0
gkch2vol init 0
gkch3vol init 0
gkch4vol init 0
gkch5vol init 0
gkch6vol init 0

;ROUTING
connect "restart", "out", "mixer", "mix1_restart"
connect "decision_level", "out", "mixer", "mix2_decision_level"
connect "backtrack_level", "out", "mixer", "mix3_backtrack_level"
connect "learnt_size", "out", "mixer", "mix4_learnt_size"
connect "conflict", "out", "mixer", "mix5_conflict"
connect "eloquence", "out", "mixer", "mix6_eloquence"

;ALWAYSON
alwayson "start"
alwayson "stop"
alwayson "restart_trigger"
alwayson "decision_level"
alwayson "backtrack_level"
alwayson "learnt_size"
alwayson "conflict_trigger"
alwayson "eloquence"

;turnoff all instruments
instr stop
  kdone init -2
  kans OSClisten giOSC, "/stop", "f", kdone

  if (kans == 1) then
    scoreline {{ e 1 }}, 1
    gkch1vol = 0
    gkch2vol = 0
    gkch3vol = 0
    gkch4vol = 0
    gkch5vol = 0
    gkch6vol = 0
  endif
endin

instr start
  kreceive init 1
  kans OSClisten giOSC, "/start", "f", kreceive
  if (kans == 1) then 
    gkch1vol = 1
    gkch2vol = .5
    gkch3vol = 0
    gkch4vol = 1
    gkch5vol = 1
    gkch6vol = .3
  endif
endin

;restart
instr restart
  giSample = giSample1
  if (p4 == 2) then
    giSample = giSample2
  endif
  if (p4 == 3) then
    giSample = giSample3
  endif
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
    event "i", "restart", 0, 0.193152, 1
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
  kvar = 440 + ((kdecision * 440) / kvariables)
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
  kvar = 16000 - ((klearnt * 16000) / 10)
  ;printk2 kvar
  awhite unirand 16200
  asound oscil 1, awhite, gisaw
  afilt butterlp asound, kvar
  outleta "out", afilt
endin

instr eloquence2
  kdecision oDecision
  kassigns oAssigns
  kvar = 200 + ((kdecision * 16000) / kassigns)
  aeloquence oscil 1, kvar, girect
  outleta "out", aeloquence
endin

instr eloquence  
  idur     = 5
  ifreq    = 700
  iamp     = 1000
  ilowamp  = 1              ; determines amount of lowpass output in signal
  ihighamp = 0              ; determines amount of highpass output in signal
  ibandamp = 0              ; determines amount of bandpass output in signal
  iq       = 490              ; value of q
  
  iharms   = (sr*.4) / ifreq
  
  asig1    gbuzz 1, 400, iharms, 1, .9, 3             ; Sawtooth-like waveform
  asig2    gbuzz 1, 3000, iharms, 1, .9, 3             ; Sawtooth-like waveform
  asig3    gbuzz 1, 7000, iharms, 1, .9, 4             ; Sawtooth-like waveform
  asig4    gbuzz 1, 16000, iharms, 1, .9, 2             ; Sawtooth-like waveform
  asig5 = asig1 + asig2 + asig3 + asig4
  ;kfreq   linseg 1, idur * 0.5, 4000, idur * 0.5, 1     ; Envelope to control filter cutoff
  
  kdecision oDecision
  kassigns oAssigns
  kfreq = 200 + ((kdecision * 16000) / kassigns)
  alow, ahigh, aband svfilter asig5, kfreq, iq
  
  aout1   = alow * ilowamp
  aout2   = ahigh * ihighamp
  aout3   = aband * ibandamp
  asum    = aout1 + aout2 + aout3
  ;kenv    linseg 0, .1, iamp, idur -.2, iamp, .1, 0     ; Simple amplitude envelope
  outleta "out", asum; * kenv
endin

instr conflict
  kdecision oDecision
  kvariables oVariables  
  kscale = (kdecision * 100) / kvariables
  kdl = 440
  ilength = .4
  if (kscale > 70) then
    kdl =  698.456 ;f
    ilength = .2
  elseif (kscale > 55) then
    kdl = 659.255 ;e
    ilength = .3
  elseif (kscale > 35) then 
    kdl = 587.330 ;d
    ilength = .35
  endif
  aenv expon 2, ilength, 0.0001
  ;aSig oscil aenv, p4, gisaw
  aSig oscil aenv, kdl, gisaw
  outleta "out", aSig 
endin

instr conflict_trigger
  kvar oConflict
    
  kcount init 0
  if (kvar > 0) then
    kcount = kcount + 1
    kvar = 0
    if (kcount > 10000) then	
      event "i", "conflict", 0, 0.1, 440
      kcount = 0
    endif
  endif
endin

instr mixer
  ach1 inleta "mix1_restart"
  ach2 inleta "mix2_decision_level"
  ach3 inleta "mix3_backtrack_level"
  ach4 inleta "mix4_learnt_size"
  ach5 inleta "mix5_conflict"
  ach6 inleta "mix6_eloquence"
  ;BEGIN: VOLUME SHIT
  kans OSClisten giOSC, "/volume/ch1", "f", gkch1vol
  kans OSClisten giOSC, "/volume/ch2", "f", gkch2vol
  kans OSClisten giOSC, "/volume/ch3", "f", gkch3vol
  kans OSClisten giOSC, "/volume/ch4", "f", gkch4vol
  kans OSClisten giOSC, "/volume/ch5", "f", gkch5vol
  kans OSClisten giOSC, "/volume/ch6", "f", gkch6vol
  ;END: VOLUME SHIT
  aout sum ach1 * gkch1vol, ach2 * gkch2vol, ach3 * gkch3vol, ach4 * gkch4vol, ach5 * gkch5vol, ach6 * gkch6vol
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
