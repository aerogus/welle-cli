# welle.io modifié pour une captation linéaire

Expérimentation de captation DAB+ avec clé rtl-sdr et welle.io.

**Ce projet est un fork, simplifié, de welle.io.**

welle-cli issu du projet welle.io est intéressant mais demande quelques modifications pour répondre au besoin de captation linéaire. Voici les modifs effectuées ou souhaitées :

- Décodage de tous les programmes du multiplex ("ensemble"), par défaut, ou décodage seulement des programmes souhaités si le param `sid` est saisi,
auquel cas on ne décode que les sid passés en param. séparation par virgule. ex :
-s [--sid] f00d,f00e

- Définir le répertoire de stockage du dump (par défaut le répertoire courant: pas pratique)
-o [--outputDir] /path/to/rec
gestion d'erreur si rep n'existe pas ou pas inscriptible.
création du rép / sous rép si besoin

- baser le nommage/préfixe sur le sid plutôt que le program name qui peut comporter des espaces ou même changer de nom
-n [--sidName]
actuellement comportement modifié en dur

- mode tube nommé plutôt que vrai fichier sur disque
-p [--pipe]

- un répertoire de stockage par sid
  - /home/rec/0xf00d/0xf00d.wav
  - /home/rec/0xf00d/0xf00d.txt
  - /home/rec/0xf00d/0xf00d-content-name.jpg|png


preprocess :

- recup liste des sids trouvés
- si filtre, 
filename = /home/rec/0xf00d.wav
if -f $filename
  unlink $filename
if $sids à decoder:
  boucle $sids
  mkfifo /home/rec/0xf00d.wav
sinon:
  ln -s /home/rec/0xf00d.wav /dev/null


## Visualiser les bibliothèques partagées utilisées par un binaire

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

## Compilation

```
cd welle.io
mkdir build
cd build
cmake .. -DRTLSDR=1 -DBUILD_WELLE_IO=OFF -DBUILD_WELLE_CLI=ON
make
sudo make install
```

## Contenu

- flux audio
natif : aac 88 kbps stéréo 48 kHz 960 frames / sec
décodé : wav pcm stéréo 16 bits 48 kHz

- DLS (Dynamic Label Segment)
  128 octets, encodage utf-8
The dynamic label feature provides short textual messages which are associated with audio programme content for
display on receivers. The messages can have any length up to a maximum of 128 bytes; depending on the character set
used, the message can have up to 128 characters. 
https://www.etsi.org/deliver/etsi_en/300400_300499/300401/02.01.01_60/en_300401v020101p.pdf

- EnsembleId
  ex: 0xf011

- EnsembleLabel
  ex: PARIS 9B

- ServiceLabel
  ex: RADIO PITCHOUN

- ServiceId
  ex: 0xf8fe

- MOT Slideshow (image)
  Multimedia Object Transfer
  JPEG ou PNG 320x240 (ou +)
  ClickThroughURL (512 octets)

https://www.etsi.org/deliver/etsi_ts/101400_101499/101499/02.02.01_60/ts_101499v020201p.pdf

