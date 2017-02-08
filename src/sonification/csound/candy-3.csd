<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 10
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
connect "continuous_depth", "out", "mixer", "in_continuous_depth"
connect "continuous_eloquence", "out", "mixer", "in_continuous_eloquence"

;ALWAYSON
turnon "oscReceiver"

;GLOBALS
giOSC OSCinit 7000

gkvariables init 0
gkassigns init 0
gkdecisions init 0
gkdecisions_hwm init 0

instr continuous_depth
  kfreq init 0
  if gkdecisions_hwm > 0 then
    kval = (gkdecisions * 12.0) / gkdecisions_hwm
    kfreq = 440 * exp(log(2.0) * kval/12.0)
  endif
  adecision oscil 1, kfreq, gisaw
  outleta "out", adecision
endin

instr continuous_eloquence
  kfreq init 0
  if gkassigns > 0 then
    kval = (gkdecisions * 12.0) / gkassigns
    kfreq = 440 * exp(log(2.0) * kval/-12.0)
  endif
  aharms oscil 1, kfreq, giharms
  ;alow butterlp aharms, kfreq
  outleta "out", aharms;
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

instr oscReceiver
  kreceive0 init 1
  kstart OSClisten giOSC, "/start", "f", kreceive0
  kstop OSClisten giOSC, "/stop", "f", kreceive0
  
  if (kstart == 1) then 
    turnon "mixer"
    turnon "continuous_depth"
    turnon "continuous_eloquence"
connect "sampler", "out", "mixer", "in_sampler"
connect "learnt", "out", "mixer", "in_learnt"
connect "conflict", "out", "mixer", "in_conflict"
connect "continuous_depth", "out", "mixer", "in_continuous_depth"
connect "continuous_eloquence", "out", "mixer", "in_continuous_eloquence"
  endif
  
  if (kstop == 1) then
    turnoff2 "mixer", 0, 0
    turnoff2 "continuous_depth", 0, 0
    turnoff2 "continuous_eloquence", 0, 0
    ;scoreline {{ e 1 }}, 1
  endif
  
  kvariables init 0
  kvariables_upd OSClisten giOSC, "/variables", "f", kvariables
  if (kvariables_upd == 1) then
    gkvariables = kvariables
  endif
  
  kdecisions init 0
  kdecisions_upd OSClisten giOSC, "/decision", "f", kdecisions
  if (kdecisions_upd == 1) then
    gkdecisions = kdecisions
    if (kdecisions > gkdecisions_hwm) then
      gkdecisions_hwm = kdecisions
    endif
  endif
  
  kassigns init 0
  kassigns_upd OSClisten giOSC, "/assignments", "f", kassigns
  if (kassigns_upd == 1) then
    gkassigns = kassigns
  endif
  
  klearnt init 0
  klearnt_upd OSClisten giOSC, "/learnt", "f", klearnt
  if (klearnt_upd == 1) then
    if (klearnt == 1) then
      event "i", "sampler", 0, .41, gieff1
    elseif (klearnt == 2) then
      event "i", "sampler", 0, .24, girvbd
    elseif (klearnt < 6) then
      event "i", "learnt", 0, .2, klearnt
    endif    
  endif
  
  kconflictlevel init 0
  kconflictlevel_upd OSClisten giOSC, "/conflict", "f", kconflictlevel
  if (kconflictlevel_upd == 1) then
    if gkdecisions_hwm > 0 then
	 kcount active "conflict"
	 if kcount > 20 then
	   turnoff2 "conflict", 1, 1
	 endif
      kval = round((kconflictlevel * 12.0) / gkdecisions_hwm)
      kfreq = 440 * exp(log(2.0) * kval/12.0)
      event "i", "conflict", 0, .4, kfreq
    endif    
  endif
  
  krestart init 0
  krestart_upd OSClisten giOSC, "/restart", "f", krestart
  if (krestart_upd == 1) then
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
