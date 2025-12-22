#!/bin/sh
./osvc \
--input-device 6 \
--output-device 7 \
--chunklength 576 --rblength 640 --fsample 48000 \
--show-buffer-health --barlength 50 --show-interval 50000 \
--bind-addr 0.0.0.0 --bind-port 63298 \
--init-pitch 2.0 --init-formant 0.58 \
#--enable-prompt
#--output-device 7 \
#--no-local-out \
#--no-local-in \
#--thru --sleep-time 500 \
#--tx-stream --tx-stream-dest-addr 100.66.0.74 --tx-stream-dest-port 59960 \
#--rx-stream --rx-stream-bind-addr 0.0.0.0 --rx-stream-bind-port 59960 \
