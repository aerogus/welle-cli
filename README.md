# welle.io modifié pour une captation linéaire du DAB+

Expérimentation de captation DAB+ avec clé rtl-sdr et welle.io.

**Ce projet est un fork, simplifié, de welle.io.**

welle-cli issu du projet welle.io est intéressant mais demande quelques modifications pour répondre au besoin de captation linéaire. Voici les modifs effectuées :

- Choix du canal du multiplex. param `-c`. Ex: `-c 5A`
- Choix du/des services à décoder. param `-s`. Saisir les serviceIds séparés par des virgules. Ex: `-s f00d,f00e`
- Choix du répertoire de stockage. param `-o`. Ex: `-o /path/to/rec`
- Le nommage des fichiers est basé sur le serviceId plutôt que le serviceLabel.
- type d'arborescence
  - /home/rec/0xf00d/0xf00d.pcm
  - /home/rec/0xf00d/0xf00d.ndjson
  - /home/rec/0xf00d/0xf00d-timestamp.jpg|png

## Compilation

```
cd welle.io
mkdir build
cd build
cmake .. -DRTLSDR=1 -DBUILD_WELLE_IO=OFF -DBUILD_WELLE_CLI=ON
make
sudo make install
```

## Contenu d'un multiplex DAB+

Un multiplex DAB+, ou ensemble, est caractérisé par une fréquence centrale en MHz.
La bande III VHF est divisée en blocs, ou canaux, de 1536 kHz de large.

À titre d'exemple Les blocs actuellement utilisés sur Paris sont les suivants :

| Bloc | Fréquence   |
| ---- | ----------- |
|  5A  | 174.928 MHz |
|  6A  | 181.936 MHz |
|  6C  | 185.360 MHz |
|  6D  | 187.072 MHz |
|  8C  | 199.360 MHz |
|  9A  | 202.928 MHz |
|  9B  | 204.640 MHz |
| 11A  | 216.928 MHz |
| 11B  | 218.640 MHz |

source: https://fr.wikipedia.org/wiki/Bandes_de_fr%C3%A9quences_de_la_t%C3%A9l%C3%A9vision_terrestre

Un ensemble a comme propriétés les informations suivantes :

- ensembleId : ex F001
- ensembleLabel: ex Paris-Etendu
- une liste de service

Un service est caractérisé par :

- serviceId: ex FEED
- serviceLabel: ex RADIO LiFE

- flux audio
natif : aac 88 kbps stéréo 48 kHz 960 frames / sec
décodé : wav pcm stéréo 16 bits 48 kHz

- DLS (Dynamic Label Segment)
  128 octets, encodage utf-8
The dynamic label feature provides short textual messages which are associated with audio programme content for
display on receivers. The messages can have any length up to a maximum of 128 bytes; depending on the character set
used, the message can have up to 128 characters. 
https://www.etsi.org/deliver/etsi_en/300400_300499/300401/02.01.01_60/en_300401v020101p.pdf

- MOT Slideshow (image)
  Multimedia Object Transfer
  JPEG ou PNG 320x240 (ou +)
  ClickThroughURL (512 octets)

https://www.etsi.org/deliver/etsi_ts/101400_101499/101499/02.02.01_60/ts_101499v020201p.pdf


## Démarrage captation

Création préalable des tubes nommés pour les services à capter. ex:

```
mkfifo 0xf201/0xf201.pcm 0xf201/0xf201.ndjson
```

Armer les captations

```
systemctl start captation@NWB_FIF
systemctl start captation-dab@NWB_FIF
````

Démarrer la réception + décodage du multiplex avec welle-cli

```
./rec.sh
```

ex lire un tube nommé, alimenté en flux pcm, avec ffplay

```
cat 0xf201.pcm | ffplay -f s16le -ar 48k -ac 2 -
```

## Divers: visualiser les bibliothèques partagées utilisées par un binaire

### sous MacOS

```
$ otool -L welle-cli
welle-cli:
	/usr/local/opt/librtlsdr/lib/librtlsdr.0.dylib (compatibility version 0.0.0, current version 0.6.0)
	/usr/local/opt/fftw/lib/libfftw3f.3.dylib (compatibility version 10.0.0, current version 10.10.0)
	/usr/local/opt/faad2/lib/libfaad.2.dylib (compatibility version 3.0.0, current version 3.0.0)
	/usr/local/opt/mpg123/lib/libmpg123.0.dylib (compatibility version 48.0.0, current version 48.0.0)
	/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 1300.23.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1311.100.3)
```

### sous Linux

```
$ ldd welle-cli
	linux-vdso.so.1 (0x0000007faefa1000)
	librtlsdr.so.0 => /usr/local/lib/aarch64-linux-gnu/librtlsdr.so.0 (0x0000007faee76000)
	libfftw3f.so.3 => /lib/aarch64-linux-gnu/libfftw3f.so.3 (0x0000007faed19000)
	libfaad.so.2 => /lib/aarch64-linux-gnu/libfaad.so.2 (0x0000007faeccb000)
	libasound.so.2 => /lib/aarch64-linux-gnu/libasound.so.2 (0x0000007faebbe000)
	libmpg123.so.0 => /lib/aarch64-linux-gnu/libmpg123.so.0 (0x0000007faeadd000)
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
