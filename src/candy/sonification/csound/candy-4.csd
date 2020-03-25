<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 20
nchnls = 2
0dbfs = 1.0

; FUNCTIONS
;f # time size 11 nh [lh] [r]
giharms ftgen 10, 0, 16384, 11, 10, 1, .7
gisaw   ftgen 11, 0, 16384, 10, 1, 0.5, 0.3, 0.25, 0.2, 0.167, 0.14, 0.125, .111
ginoise ftgen 12, 0, 16384, 21, 4
;gisine  ftgen 13, 0, 16384, 10, 1
;gisine2 ftgen 14, 0, 16384, 10, 1, .2, .08, .07
;girect  ftgen 15, 0, 16384, 10, 1, 0, 0.3, 0, 0.2, 0, 0.14, 0, .111

;ROUTING
connect "sample_play", "out", "mixer", "in_sampler"
connect "learnt", "out", "mixer", "in_learnt"
connect "conflict", "out", "mixer", "in_conflict"
connect "continuous_harms", "out", "mixer", "in_continuous_depth"
connect "continuous_saw", "out", "mixer", "in_continuous_eloquence"

;ALWAYSON
turnon "oscReceiver"

;GLOBALS
giOSC OSCinit 7000

opcode fract2octave, k, kkkk
    kfract, kbasefreq, kspread, kquant xin
    if (kquant != 0) then
	  kfreq = kbasefreq * exp(log(2.0) * round(kfract * kquant * kspread) / kquant)
    else 
      kfreq = kbasefreq * exp(log(2.0) * kfract * kspread)
    endif
    xout kfreq
endop

instr continuous_harms
  ifreq = p4
  ispread = p5
  kval chnget "continuous_harms"
  kfreq fract2octave kval, ifreq, ispread, 0
  aout oscil 1, kfreq, giharms
  aout lowpass2 aout, kfreq*4, 2
  outleta "out", aout
endin

instr continuous_saw
  ifreq = p4
  ispread = p5
  kval chnget "continuous_saw"
  kfreq fract2octave kval, ifreq, ispread, 0
  aout oscil 1, kfreq, gisaw
  aout lowpass2 aout, kfreq*4, 2
  outleta "out", aout
endin

instr sample_play
  ifn = p4
  ;idur = 1
  ;idur = ftsr(ifn) / ftlen(ifn)
  idur = p3
  aSamp poscil3 1, idur, ifn
  outleta "out", aSamp
endin

instr sampler
  ismp1 ftgen 0, 0, 0, 1, "samples/48.WAV", 0, 4, 1;bd
  ismp2 ftgen 0, 0, 0, 1, "samples/40.WAV", 0, 4, 1;hd
  ismp3 ftgen 0, 0, 0, 1, "samples/26.WAV", 0, 4, 1;voicehit
  ismp4 ftgen 0, 0, 0, 1, "samples/36.WAV", 0, 4, 1;rvbd
  ismp5 ftgen 0, 0, 0, 1, "samples/12.WAV", 0, 4, 1;eff1
  ismp6 ftgen 0, 0, 0, 1, "samples/14.WAV", 0, 4, 1;eff2
  ismp7 ftgen 0, 0, 0, 1, "samples/18.WAV", 0, 4, 1;brrr
  ismp8 ftgen 0, 0, 0, 1, "samples/29.WAV", 0, 4, 1;rims
  ismp9 ftgen 0, 0, 0, 1, "samples/38.WAV", 0, 4, 1;distbd

  idur1 filelen "samples/48.WAV"
  idur2 filelen "samples/40.WAV"
  idur3 filelen "samples/26.WAV"
  idur4 filelen "samples/36.WAV"
  idur5 filelen "samples/12.WAV"
  idur6 filelen "samples/14.WAV"
  idur7 filelen "samples/18.WAV"
  idur8 filelen "samples/29.WAV"
  idur9 filelen "samples/38.WAV"
    
  kval1 chnget "sample1"
  kval2 chnget "sample2"
  kval3 chnget "sample3"
  kval4 chnget "sample4"
  kval5 chnget "sample5"
  kval6 chnget "sample6"
  kval7 chnget "sample7"
  kval8 chnget "sample8"
  kval9 chnget "sample9"
  
  ktrig changed kval1
  if kval1 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur1, ismp1
    chnset k(0), "sample1"
  endif
  
  ktrig changed kval2
  if kval2 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur2, ismp2
    chnset k(0), "sample2"
  endif
  
  ktrig changed kval3
  if kval3 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur3, ismp3
    chnset k(0), "sample3"
  endif
  
  ktrig changed kval4
  if kval4 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur4, ismp4
  else
    chnset k(0), "sample4"
  endif
  
  ktrig changed kval5
  if kval5 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur5, ismp5
  else
    chnset k(0), "sample5"
  endif
  
  ktrig changed kval6
  if kval6 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur6, ismp6
    chnset k(0), "sample6"
  endif
  
  ktrig changed kval7
  if kval7 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur7, ismp7
    chnset k(0), "sample7"
  endif
  
  ktrig changed kval8
  if kval8 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur8, ismp8
    chnset k(0), "sample8"
  endif
  
  ktrig changed kval9
  if kval9 == 1 && ktrig == 1 then
    event "i", "sample_play", 0, idur9, ismp9
    chnset k(0), "sample9"
  endif
endin

instr learnt    
  aenv expon 3, p3, 0.0001
  ;aenv linen 2, .1, p3, .2  
  kfreq fract2octave p4, 880.0, 0, 1
  aout2 oscil aenv, kfreq, ginoise
  aout3 lowpass2 aout2, kfreq, 30
  
  outleta "out", aout3
endin

instr conflict
  aenv expon 1, p3, 0.0001
  aout oscil aenv, p4, gisaw
  ;aout lowpass2 aout, p4*4, 2
  outleta "out", aout 
endin

kcount active "conflict"
if kcount > 42 then
  turnoff2 "conflict", 1, 1
endif

kcount active "learnt"
if kcount > 42 then
  turnoff2 "learnt", 1, 1
endif

kcount active "sample_play"
if kcount > 42 then
  turnoff2 "sample_play", 1, 1
endif

instr oscReceiver
  kreceive init 1
  kstart OSClisten giOSC, "/start", "f", kreceive
  kstop OSClisten giOSC, "/stop", "f", kreceive

  chnset 0, "continuous_saw"
  chnset 0, "continuous_harms"
  
  schedule "mixer", 0, -1
  schedule "sampler", 0, -1
  schedule "continuous_saw", 0, -1, 55, 2
  schedule "continuous_harms", 0, -1, 440, 1
  
  if (kstop == 1) then
    turnoff2 "mixer", 0, 0
    turnoff2 "sampler", 0, 0
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
    chnset 1 - kdecisions / kassigns, "continuous_saw" 
    chnset kdecisions / kdecisions_hwm, "continuous_harms"
  endif    
  
  kupd OSClisten giOSC, "/assignments", "i", kassigns
  if (kupd == 1) then
    chnset 1 - kdecisions / kassigns, "continuous_saw" 
  endif
  
  klearnt init 0
  kupd OSClisten giOSC, "/learnt", "i", klearnt
  if (kupd == 1) then
    if (klearnt == 1) then
      chnset k(1), "sample5"
    elseif (klearnt == 2) then
      chnset k(1), "sample3"
    elseif (klearnt < 6) then
      event "i", "learnt", 0, .5, (1 - (klearnt - 2) / 3)
    endif    
  endif
  
  kconflictlevel init 0
  kupd OSClisten giOSC, "/conflict", "i", kconflictlevel
  if (kupd == 1) then
    if kdecisions_hwm > 0 then
      kval = kconflictlevel / kdecisions_hwm
      kfreq fract2octave kval, 440, 12, 1
      event "i", "conflict", 0, .4, kfreq
    endif        
  endif
  
  krestart init 0
  kupd OSClisten giOSC, "/restart", "i", krestart
  if (kupd == 1) then
    chnset k(1), "sample1"
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
  kch3vol init .3
  kch4vol init .3
  kch5vol init .3
  
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
