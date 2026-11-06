# Provisional dialogue reactions

Original synthetic placeholder performances made with Csound 6 and SoX 14.4.2.
No recordings, external samples, AI models or actor voice references were used.
`reactions.csd` is the durable synthesis source. The three exports are 48 kHz,
mono PCM16, normalized to -9 dBFS with short endpoint fades:

| Export | Source offset | Duration | Character cue |
| --- | ---: | ---: | --- |
| boric-chuckle.wav | 0 s | 1.0 s | Low pulsed chuckle |
| balin-chuckle.wav | 2 s | 0.9 s | Higher, quicker chuckle |
| mayor-groan.wav | 4 s | 1.1 s | Falling voiced groan |

To regenerate, create an agent artifact run, then render there:

```sh
csound -W -s -o RUN/reactions.wav assets/audio/dialogue/reactions.csd
sox RUN/reactions.wav -b 16 RUN/boric-chuckle.wav trim 0 1 highpass 75 fade q .01 1 .06 gain -n -9
sox RUN/reactions.wav -b 16 RUN/balin-chuckle.wav trim 2 .9 highpass 75 fade q .01 .9 .06 gain -n -9
sox RUN/reactions.wav -b 16 RUN/mayor-groan.wav trim 4 1.1 highpass 75 fade q .01 1.1 .06 gain -n -9
```

Apply normal low CPU/I/O priority and one-thread environment settings. Validate
format, non-silence, DC and endpoints, then promote only the selected WAV files
here. These clips establish synchronization and comic rhythm; listening acceptance
and replacement with polished voice performances remain separate work.
