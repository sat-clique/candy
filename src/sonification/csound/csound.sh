if [ $# -lt 1 ]; then
  echo "Usage: $0 csound-file"
  exit 0
fi

jack-rack --string-name="csound" csound.rack &
csound -+rtaudio=jack -odac:plug:pulse -m4 -b1024 -B4096 -r44100 $1 &

s=""
while [ -z "$s" ]; do
  s=`jack_lsp | grep csound6`; 
  sleep 2; 
done

jack_connect jack_rack_csound:out_1 hdmi:playback_1
jack_connect jack_rack_csound:out_2 hdmi:playback_2
jack_connect csound6:output1 jack_rack_csound:in_1
jack_connect csound6:output2 jack_rack_csound:in_2
