<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 20
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

;ROUTING
connect "sampler", "out", "mixer", "in_sampler"
connect "learnt", "out", "mixer", "in_learnt"
connect "conflict", "out", "mixer", "in_conflict"
connect "continuous_harms", "out", "mixer", "in_continuous_depth"
connect "continuous_saw", "out", "mixer", "in_continuous_eloquence"

;ALWAYSON
turnon "oscReceiver"

;GLOBALS
giOSC OSCinit 7000

opcode fract2octave, k, kkk
    kfract, kbasefreq, kquant xin
    if (kquant != 0) then
	kfreq = kbasefreq * exp(log(2.0) * round(kfract * kquant) / kquant)
    else 
      kfreq = kbasefreq * exp(log(2.0) * kfract)
    endif
    xout kfreq
endop

instr continuous_harms
  ifreq = p4
  Schan = p5
  kval chnget Schan
  kfreq fract2octave kval, ifreq, 0.0
  aout oscil 1, kfreq, giharms
  outleta "out", aout
endin

instr continuous_saw
  ifreq = p4
  Schan = p5
  kval chnget Schan
  kfreq fract2octave kval, ifreq, 0.0
  aout oscil 1, kfreq, gisaw
  outleta "out", aout
endin

instr sampler
  ifn = p4
  idur = ftsr(ifn) / ftlen(ifn)
  aSamp poscil3 2, idur, ifn
  outleta "out", aSamp
endin

instr learnt
  ;aenv expon 1, .5, 0.0001
  ilearnt = p4
  aenv linen 2, .2, p3, .1
  ilowpass = 16000 - ((16000 / 6) * ilearnt)
  anoise oscil aenv, 440, ginoise
  aout butterlp anoise, ilowpass
  outleta "out", aout
endin

instr conflict
  aenv expon 1, p3, 0.0001
  aSig oscil aenv, p4, gisaw
  outleta "out", aSig 
endin

kcount active "conflict"
if kcount > 42 then
  turnoff2 "conflict", 1, 1
endif

kcount active "learnt"
if kcount > 42 then
  turnoff2 "learnt", 1, 1
endif

kcount active "sampler"
if kcount > 42 then
  turnoff2 "sampler", 1, 1
endif

instr oscReceiver
  kreceive init 1
  kstart OSClisten giOSC, "/start", "f", kreceive
  kstop OSClisten giOSC, "/stop", "f", kreceive

  chnset 0.0, "eloquence"
  chnset 0.0, "depth"
  scoreline_i {{
    i "mixer" 0 -1
    i "continuous_saw" 0 -1 220.0 "eloquence"
    i "continuous_harms" 0 -1 440.0 "depth"
  }}
  
  if (kstop == 1) then
    turnoff2 "mixer", 0, 0
    turnoff2 "continuous_saw", 0, 0
    turnoff2 "continuous_harms", 0, 0
    ;scoreline {{ e 1 }}, kstop
  endif
  
  kvariables init 0
  kupd OSClisten giOSC, "/variables", "i", kvariables
  
  kassigns init 1
  kdecisions init 0
  kdecisions_hwm init 1
  kupd OSClisten giOSC, "/decision", "i", kdecisions
  if (kupd == 1) then
    if (kdecisions > kdecisions_hwm) then
      kdecisions_hwm = kdecisions
    endif
    chnset kdecisions / kassigns, "eloquence" 
    chnset kdecisions / kdecisions_hwm, "depth"
  endif    
  
  kupd OSClisten giOSC, "/assignments", "i", kassigns
  if (kupd == 1) then
    chnset kdecisions / kassigns, "eloquence" 
  endif
  
  klearnt init 0
  kupd OSClisten giOSC, "/learnt", "i", klearnt
  if (kupd == 1) then
    if (klearnt == 1) then
      event "i", "sampler", 0, .41, gieff1
    elseif (klearnt == 2) then
      event "i", "sampler", 0, .24, girvbd
    elseif (klearnt < 6) then
      event "i", "learnt", 0, .2, klearnt
    endif    
  endif
  
  kconflictlevel init 0
  kupd OSClisten giOSC, "/conflict", "i", kconflictlevel
  if (kupd == 1) then
    if kdecisions_hwm > 0 then
      kval = kconflictlevel / kdecisions_hwm
      kfreq fract2octave kval, 440.0, 12.0
      event "i", "conflict", 0, .4, kfreq
    endif    
  endif
  
  krestart init 0
  kupd OSClisten giOSC, "/restart", "i", krestart
  if (kupd == 1) then
    event "i", "sampler", 0, .31, gibd
  endif
endin

instr mixer
  asampler inleta "in_sampler"
  alearnt inleta "in_learnt"
  aconflict inleta "in_conflict"
  acdepth inleta "in_continuous_depth"
  aceloquence inleta "in_continuous_eloquence"
  
  kch1vol init 1
  kch2vol init 1
  kch3vol init .7
  kch4vol init .7
  kch5vol init .7
  
  kreceive1 init 1
  kans1 OSClisten giOSC, "/volume/sampler", "f", kreceive1
  if (kans1 == 1) then
    kch1vol = kreceive1
  endif
  
  kreceive2 init 1
  kans2 OSClisten giOSC, "/volume/learnt", "f", kreceive2
  if (kans2 == 1) then
    kch2vol = kreceive2
  endif
  
  kreceive3 init 1
  kans3 OSClisten giOSC, "/volume/conflict", "f", kreceive3
  if (kans3 == 1) then
    kch3vol = kreceive3
  endif
  
  kreceive4 init 1
  kans4 OSClisten giOSC, "/volume/depth", "f", kreceive4
  if (kans4 == 1) then
    kch4vol = kreceive4
  endif
  
  kreceive5 init 1
  kans5 OSClisten giOSC, "/volume/eloquence", "f", kreceive5
  if (kans5 == 1) then
    kch5vol = kreceive5
  endif
  
  aout sum asampler * kch1vol, alearnt * kch2vol, aconflict * kch3vol, acdepth * kch4vol, aceloquence * kch5vol
  outs aout, aout
endin

</CsInstruments>

<CsScore>
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
