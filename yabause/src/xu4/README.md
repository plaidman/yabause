# YabaSanshiro for ORDROID XU4

## How to build:

```
$ git clone https://github.com/devmiyax/yabause.git -b minimum_linux
$ cd yabause
$ git submodule update --init --recursive
$ mkdir build
$ cd build
$ cmake ../yabause -DYAB_PORTS=xu4 -DYAB_WANT_DYNAREC_DEVMIYAX=ON -DYAB_WANT_ARM7=ON
$ make
```

## Commandline Usage:

|           |                               |                                              |
|-----------|-------------------------------|----------------------------------------------|
| -b STRING | --bios STRING                 | bios file                                    |
| -i STRING | --iso STRING                  | iso/cue file                                 |
| -r NUMBER | --resolution_mode NUMBER      | 0 .. Native, 1 .. 4x, 2 .. 2x, 3 .. Original |
| -a        | --keep_aspect_rate            | |
| -s NUMBER | --scps_sync_per_frame NUMBER  | |
| -f        | --frame_skip                  | |
| -v        | --version                     | | 
| -h        |                               | Show help information |

## Special functions for Retropie:

* Synchronize input settings with ~/.emulationstation/es_temporaryinput.cfg

| es_temporaryinput.cfg | YabaSanshiro                  |
|-----------------------|-------------------------------|
| up                    | up |
| down                  | down |
| left                  | left |
| right                 | right |
| a                     | a |
| b                     | b |
| rightshoulder         | c |
| a                     | x |
| b                     | y |
| leftshoulder          | z |
| lefttrigger           | l |
| righttrigger          | r |
| start                 | start |

'select' is force to mapped as 'Show/Hide Menu' button
