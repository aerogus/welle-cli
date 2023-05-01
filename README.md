# Forked welle.io for DAB+ linear recording

This project is a simplified fork of [welle.io](https://github.com/AlbrechtL/welle.io)

## Goals

- Learn the official `welle.io` codebase
- Learn how a `DAB+` mux works
- Be focus only on the `cli` tool and `rtl-sdr` support
- Adapt the code to be able to record 24/7 a whole ensemble or only selected service(s)
- Be able to extract audio stream (decoded PCM and native AAC) + DLS + MOT
- if people are interested in these specific features, propose back some patchs ans pull requests to the official [welle.io project](https://github.com/AlbrechtL/welle.io/issues/657)

## Why a fork ?

`welle-cli` command from `welle.io` project is great but I didn't find a way to do some specific things:

- How to selected some services out of an ensemble ?
- How to specify a record path ?
- How to extract the raw ACC stream ?
- How to avoid aac -> PCM -> mp3 decoding/recoding ?

So I began this fork. Of course with free GPL licence.

## Differences / Usage

- Choose a block. arg `-c`. ex: `-c 5A`
- Choose the service(s) to decode. arg `-s` + serviceIds comma separated, ex: `-s f00d,f00e`
- Choose base storage directory. arg `-o`. ex: `-o /path/to/rec`
- Files name are bases on `serviceId` rather than `serviceLabel`.
- Audio streams are decoded and recorded in .pcm file (raw without any header, 48kHz, stereo, 16 bits)
- Metadata are stored in new line delimited json format (extension `.ndjson`)
- MOT pictures are stored with their original extension (png ou jpg), with a timestamp in the file name
- Tree structure (f00d = serviceId in hexa)
  - /path/to/rec/`$serviceId`/`$serviceId`.pcm
  - /path/to/rec/`$serviceId`/`$serviceId`.ndjson
  - /path/to/rec/`$serviceId`/`$serviceId`-`$timestamp`-MOT.[`jpg|png`]
- Ability to use several SDR keys to records more than one ensemble on a same machine (tested by 2)

## Installation

### Debian

```
sudo apt install cmake g++ librtlsdr-dev libfftw3-dev
```

### MacOS

```
brew install cmake mpg123 fftw librtlsdr
```

### Compilation

```
cd welle.io
mkdir build
cd build
cmake ..
make
sudo make install
```

## About DAB+ ensemble

A DAB+ multiplex, or mux, or ensemble, is identified by a central frequency, in MHz
BHF Band III is splitted in blocks, or channels width 1536 kHz bandwidth.

Paris current active blocks are the following :

| Block | Frequency   | EnsembleLabel   |
| ----- | ----------- | --------------- |
|   5A  | 174.928 MHz | EXPE TDF TFL 5A |
|   6A  | 181.936 MHz | PARIS 6A        |
|   6C  | 185.360 MHz | towerCast-m1    |
|   6D  | 187.072 MHz | PARIS 6D        |
|   8C  | 199.360 MHz | towerCast-m2    |
|   9A  | 202.928 MHz | RNT Associative |
|   9B  | 204.640 MHz | PARIS 9B        |
|  11A  | 216.928 MHz | PARIS 11A       |
|  11B  | 218.640 MHz | Paris-Etendu    |

An ensemble gather the following properties :

- `ensembleId` (ex: `F001`)
- `ensembleLabel` (ex: `Paris-Etendu`)
- A service list

A service is caracterised by :

- `serviceId` (ex: `FEED`)
- `serviceLabel` (ex: `RADIO LiFE`)
- an audio stream (codec `HE-AAC`, 960 frames / sec), 88 à 128 kbps, mono or stereo, 48 kHz. We'll decode in WAV PCM 16 bits stéréo 48 kHz

- `DLS` (Dynamic Label Segment) : 128 bytes, utf-8 encodec
  
The dynamic label feature provides short textual messages which are associated with audio programme content for
display on receivers. The messages can have any length up to a maximum of 128 bytes; depending on the character set
used, the message can have up to 128 characters. 
https://www.etsi.org/deliver/etsi_en/300400_300499/300401/02.01.01_60/en_300401v020101p.pdf

- MOT Slideshow (image)

Multimedia Object Transfer, JPEG ou PNG 320x240 (ou +), ClickThroughURL (512 octets)

https://www.etsi.org/deliver/etsi_ts/101400_101499/101499/02.02.01_60/ts_101499v020201p.pdf

## Launch a recording session

In the `conf` directory, create a profile, ex `5A.ini` file with this kind of content :

```
REC_DIR="/Users/gus/dab"
BLOCK="5A"
SERVICES=("F00D:239.0.0.1" "F00E:239.0.0.2")
```

`rec.sh` script uses named pipes. `welle-cli` will write into these pipes (2 for each service), but an application has to read these whole pipes at the other side, otherwise there will be a buffer overflow. So you have to arm these readers before launch `welle-cli`.

`read-pipe.sh` simple script could be used to read these pipes. We could launch in parallel these commands:

```
./read-pipe.sh /Users/gus/dab/f00d/f00d.pcm
./read-pipe.sh /Users/gus/dab/f00d/f00d.ndjson
./read-pipe.sh /Users/gus/dab/f00e/f00e.pcm
./read-pipe.sh /Users/gus/dab/f00e/f00e.ndjson
```

Now execute `rec.sh` which do some checkes (and create named pipes) before the real `welle-cli` launch.

```
% ./rec.sh ./conf/5A.ini
- Autostart:
- Simu:      0
- Config:    ./conf/5A.ini
- Storage:  /Users/gus/dab
- Block:     5A
- Services:  F00D,F00E
- Create directory /Users/gus/dab/f00d
- Create named pipe /Users/gus/dab/f00d/f00d.pcm
- Create named pipe /Users/gus/dab/f00d/f00d.ndjson
- create directory /Users/gus/dab/f00e
- Create named pipe /Users/gus/dab/f00e/f00e.pcm
- Create named pipe /Users/gus/dab/f00e/f00e.ndjson
Did you arm all the recorders for selected services ? (y/N)
y
- welle-cli launch
---
InputFactory:Input device:auto
RTL_SDR: Open rtl-sdr
...
```

Synchronisation is done and named pipes start to be written (so as the MOT files). You can stop the process with `Ctrl+C`.

## Misc

Read a named pipe, ingested by a raw pcm stream, with ffplay

```
cat f201.pcm | ffplay -f s16le -ar 48k -ac 2 -
```

## Visualize shared libraries for a binary

### MacOS

```
$ otool -L welle-cli
welle-cli:
	/usr/local/opt/librtlsdr/lib/librtlsdr.0.dylib (compatibility version 0.0.0, current version 0.6.0)
	/usr/local/opt/fftw/lib/libfftw3f.3.dylib (compatibility version 10.0.0, current version 10.10.0)
	/usr/local/opt/faad2/lib/libfaad.2.dylib (compatibility version 3.0.0, current version 3.0.0)
	/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 1300.23.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1311.100.3)
```

### Linux

```
$ ldd welle-cli
	linux-vdso.so.1 (0x0000007faefa1000)
	librtlsdr.so.0 => /usr/local/lib/aarch64-linux-gnu/librtlsdr.so.0 (0x0000007faee76000)
	libfftw3f.so.3 => /lib/aarch64-linux-gnu/libfftw3f.so.3 (0x0000007faed19000)
	libfaad.so.2 => /lib/aarch64-linux-gnu/libfaad.so.2 (0x0000007faeccb000)
	libasound.so.2 => /lib/aarch64-linux-gnu/libasound.so.2 (0x0000007faebbe000)
	libpthread.so.0 => /lib/aarch64-linux-gnu/libpthread.so.0 (0x0000007faeaac000)
	libstdc++.so.6 => /lib/aarch64-linux-gnu/libstdc++.so.6 (0x0000007fae8d4000)
	libm.so.6 => /lib/aarch64-linux-gnu/libm.so.6 (0x0000007fae829000)
	libgcc_s.so.1 => /lib/aarch64-linux-gnu/libgcc_s.so.1 (0x0000007fae805000)
	libc.so.6 => /lib/aarch64-linux-gnu/libc.so.6 (0x0000007fae68f000)
	libusb-1.0.so.0 => /lib/aarch64-linux-gnu/libusb-1.0.so.0 (0x0000007fae663000)
	/lib/ld-linux-aarch64.so.1 (0x0000007faef71000)
	libdl.so.2 => /lib/aarch64-linux-gnu/libdl.so.2 (0x0000007fae64f000)
	libudev.so.1 => /lib/aarch64-linux-gnu/libudev.so.1 (0x0000007fae618000)
```

## Ressources

- https://aerogus.net/posts/diffuser-tele-radio-reseau-local/#radio-dab-plus
- https://aerogus.net/posts/radio-dab-welle-cli/
- https://aerogus.net/posts/enregistrer-multiplex-radio-dab/
- https://github.com/aerogus/welle-cli
- https://github.com/AlbrechtL/welle.io
- https://fr.wikipedia.org/wiki/Bandes_de_fr%C3%A9quences_de_la_t%C3%A9l%C3%A9vision_terrestre