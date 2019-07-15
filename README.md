# Hybrid-Timbre-Synthesis
Modify the timbre of an input music signal so that it resembles a different instrument while preserving the input’s pitch temporal variations, RMS amplitude and spectral centroid.

HTS.pdf---talk slides

train trumpet signals to get trumpet.tf

input sax_original.wav&trumpet.tf

output sax2trumpet.wav

raw code: tfsynth (based on sndan in Prof. James Beauchamp's CMP Lab in UIUC, to install sndan program, send email to 13333312299@163.com)

Hybrid Timbre Synthesis (details in HTS.pdf)

Example

●	program command: tfsynth.fa0

●	input: transfer function(spectral envelope) data--trumpet.tf;

input signal analysis file--leonard.16.nik.mq.an
       
●	output: resynthesized sound signal--leonard.test.wav;

resynthesized analysis file(optional)--leonard.test.an

