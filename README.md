# iss-sstv-rpi

This set of scripts uses an assortment of software to achieve simulation of the ARISS SSTV Downlink for training and education.

Input PNGs are overlaid with a QRcode to identify reception, encoded as PD120, modulated for NBFM and transmitted using a LimeSDR Mini.

This project is used at very low power, to allow reception only within about 50 meters of the transmitter.

## Software Installation

This repository contains submodules, ensure to clone recursively.

`git clone -r https://github.com/ARISS-UK/iss-sstv-rpi.git`

### ISS TLE Download

The latest ISS TLE is downloaded directly from Space-Track, this requires registering an account at: https://www.space-track.org/ 

Copy `tle-spacetrack/credentials.template` to `tle-spacetrack/credentials` and enter your account credentials.

An application is included which verifies the TLE (checksum & approximate orbit) before replacing your local copy. This needs to be compiled:

```
cd tle-spacetrack/tlevalid/
make
cd ../..
```

The TLE download also needs to be added to a separate cronjob, this is covered in the "Software Configuration" section below.

### ISS Barrier

The ISS position from the downloaded TLE is propagated for the next 10 minutes to check that it is not expected above our horizon, this helps ensure that we do not interfere with the genuine ISS downlink.

```
cd iss-barrier
make
cd ../
```

### SSTV Encoding

SSTV encoding uses `pysstv` ( https://pypi.org/project/PySSTV/ )

The images are also overlaid by a QRcode containing a random string, this is produced by the `qrcode` library.

```
sudo apt install python3-pip libopenjp2-7 libtiff5
pip3 install pysstv
pip3 install qrcode
pip3 install Pillow
```

### IF Modulation

WAV manipulation in preparation of the audio uses `sox`. Periods of silence are prepended and appended to work around some bugs in the LimeSDR FIFO handling.

`sudo apt install sox`

Modulation from audio up to IF is achieved with `csdr` ( https://github.com/ha7ilm/csdr )

```
sudo apt install libfftw3-dev
cd csdr/
make
cd ../
```

### RF Transmission

`limesdr_toolbox` is used to transmit the IQ file from the LimeSDR Mini.

```
echo "deb http://download.opensuse.org/repositories/network:/osmocom:/latest/Raspbian_10/ ./" | sudo tee /etc/apt/sources.list.d/osmocom-latest.list
curl http://download.opensuse.org/repositories/network:/osmocom:/latest/Raspbian_10/Release.key | sudo apt-key add -
sudo apt update
sudo apt install liblimesuite-dev
cd limesdr_toolbox
make
cd ..
```

The fork of `limesdr_toolbox` used here contains a few FIFO handling improvements.

### Webfolder

A simple webserver service script is included to allow others to see the transmission logs.

Note: The repository path is hardcoded in this script, please verify that it matches yours before attempting to run it.

```
sudo cp -f sstv_transmitted_webfolder.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable sstv_transmitted_webfolder
sudo systemctl start sstv_transmitted_webfolder
```

By default this'll serve on port 8080 ( http://localhost:8080/ ) - feel free to customise!

## Software Configuration

### Callsign Transmission

`send_callsign` is a script designed to transmit a pre-modulated voice callsign at least every 15 minutes. The script also uses the opportunity to recalibrate the LimeSDR output.

To use this, you'll need to record your own voice identification, at Mono 12KHz, and then modulate to the file `callsign_48k.iq` with something similar to:
```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./csdr/
sox callsign.wav 2s_silence.wav -t wav - | csdr/csdr convert_i16_f | csdr/csdr gain_ff 1.1 | csdr/csdr fmmod_fc  | csdr/csdr fir_interpolate_cc 4 | csdr/csdr convert_f_i16 > callsign_48k.iq
```

### Input Images

A set of example images are included in `sstv_input`, the image for transmission is chosen at random from this directory. No prevention of repeated transmission is implemented.

All images should be PNGs of 640x480, although larger PNGs will be automatically rescaled. The file extension of `.png` is required.

### Image Transmission

The `send_sstv` script automates the process of checking ISS visibility, encoding and modulating a random image, saving a log, and transmitting the image.

All encoded PNGs and audio WAVs are initially saved into `sstv_encoded` however they're then moved to a dated subfolder in `transmitted` immediately prior to transmission.

### Scheduled Transmission

Cron is normally used to schedule the transmissions, note that PD120 transmissions typically take 2 minutes. There's no locks to prevent 2 scripts fighting over simultaneous control of the LimeSDR, and this can put the LimeSDR into a temporarily unusable state!

To edit cronjobs running as the 'pi' user: `crontab -e`

An example set of cronjobs:
```
3            *  *  *   *   /home/pi/iss-sstv-rpi/tle-spacetrack/spacetrack_tle >/dev/null
59           8  *  6   *   /home/pi/iss-sstv-rpi/send_callsign >/dev/null
11,27,43,55  9  *  6   *   /home/pi/iss-sstv-rpi/send_callsign >/dev/null
*/4          9  *  6   *   /home/pi/iss-sstv-rpi/send_sstv >/dev/null
```

## Authors

Phil Crump <phil@philcrump.co.uk>

SSTV process derived from Dave Honess ( https://gist.github.com/davidhoness/b16a666403c46bfbe159fa2ec907df1d )
