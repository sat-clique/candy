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
gisine ftgen 3, 0, 16384, 10, 1
gisine2 ftgen 4, 0, 16384, 10, 1, .2, .08, .07
gisaw ftgen 5, 0, 16384, 10, 1, 0.5, 0.3, 0.25, 0.2, 0.167, 0.14, 0.125, .111 
girect ftgen 6, 0, 16384, 10, 1, 0, 0.3, 0, 0.2, 0, 0.14, 0, .111
;  f4 0 16384 10 1 0   0.3 0    0.2 0     0.14 0     .111   ; Square
;  f5 0 16384 10 1 1   1   1    0.7 0.5   0.3  0.1          ; Pulse

;OSC
giOSC OSCinit 7000

;ROUTING
connect "sampler", "out", "mixer", "mix1_sampler"
connect "decision_level", "out", "mixer", "mix2_decision_level"
connect "backtrack_level", "out", "mixer", "mix3_backtrack_level"
connect "learnt_size", "out", "mixer", "mix4_learnt_size"
connect "conflict", "out", "mixer", "mix5_conflict"
connect "eloquence", "out", "mixer", "mix6_eloquence"

;ALWAYSON
alwayson "restart_trigger"
alwayson "decision_level"
alwayson "backtrack_level"
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

opcode oVariables, k, 0 
  kreceive init 1
  knvars init 1
  kans OSClisten giOSC, "/variables", "f", kreceive
  if (kans == 1) then
    knvars = kreceive
  endif
  xout knvars
endop

opcode oDecision, k, 0 
  kreceive init 1
  kndec init 1
  kans OSClisten giOSC, "/decision", "f", kreceive
  if (kans == 1) then
    kndec = kreceive
  endif
  xout kndec
endop

opcode oBacktrack, k, 0
  kreceive init 1
  knback init 1
  kans OSClisten giOSC, "/backtrack", "f", kreceive
  if (kans == 1) then
    knback = kreceive
  endif
  xout knback
endop

opcode oConflict, k, 0 
  kreceive init 1
  knconfl init 1
  kans OSClisten giOSC, "/conflict", "f", kreceive
  if (kans == 1) then
    knconfl = kreceive
  endif
  xout knconfl
endop

opcode oAssigns, k, 0 
  kreceive init 1
  knass init 1
  kans OSClisten giOSC, "/assignments", "f", kreceive
  if (kans == 1) then
    knass = kreceive
  endif
  xout knass
endop

opcode oLearnt, k, 0 
  kreceive init 0
  knlearnt init 1
  kans OSClisten giOSC, "/learnt", "f", kreceive
  if (kans == 1) then
    knlearnt = kreceive
  else 
    knlearnt = 10
  endif
  xout knlearnt
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
  if (klearnt == 1) then
    event "i", "sampler", 0, 0.5, 2
    klearnt = 10
  elseif (klearnt > 10) then 
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
  iq       = 490            ; value of q
  
  iharms   = (sr*.4) / ifreq
  
  asig1    gbuzz 1, 400, iharms, 1, .9, 3             ; Sawtooth-like waveform
  asig2    gbuzz 1, 3000, iharms, 1, .9, 3             ; Sawtooth-like waveform
  asig3    gbuzz 1, 7000, iharms, 1, .9, 4             ; Sawtooth-like waveform
  asig4    gbuzz 1, 16000, iharms, 1, .9, 3             ; Sawtooth-like waveform
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
  kdl init 0
  ilength init 0
  kdecision oDecision
  kvariables oVariables  
  kscale = (kdecision * 100) / kvariables
  if (kscale > 50) then
    kdl =  698.456 ;f
    ilength = .35
  elseif (kscale > 30) then
    kdl = 659.255 ;e
    ilength = .3
  elseif (kscale > 5) then 
    kdl = 587.330 ;d
    ilength = .25
  else
    kdl = 440 ;a
    ilength = .2
  endif
  printk 10, kscale
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
      event "i", "conflict", 0, 0.1
      kcount = 0
    endif
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
    kch2vol = .5
    kch3vol = 0
    kch4vol = 1
    kch5vol = 1
    kch6vol = .5
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
f  2      0          0       1      "samples/06.WAV"     0         4         1
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
