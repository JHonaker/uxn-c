# Uxn/Varvara Compliant Emulator

Varvara is the specification for devices communicating with the Uxn CPU.

https://100r.co/site/uxn.html
https://wiki.xxiivv.com/site/uxn.html
https://wiki.xxiivv.com/site/varvara.html

I'm using it as an excuse to brush-up on my C, and get to know Raylib.

https://www.raylib.com/index.html

## Varvara Specification Compliance

### System Device

| Port | Name | Status |
| --- | --- | --- | 
| 00 | Unused* | NA |
| 01 | | |
| 02 | expansion* | Done |
| 03 | | |
| 04 | wst | Done |
| 05 | rst | Done |
| 06 | metadata* | |
| 07 | | |
| 08 | red* | Done |
| 09 | | |
| 0a | green* | Done |
| 0b | | |
| 0c | blue* | Done |
| 0d | | |
| 0e | debug | |
| 0f | state | Done |

### Console Device

| Port | Name | Status |
| --- | --- | --- | 
| 10 | vector* | Done |
| 11 | | |
| 12 | read | Done |
| 13 | | |
| 14 |  |  |
| 15 |  |  |
| 16 |  | |
| 17 | type | Done |
| 18 | write | Done |
| 19 | error | |
| 1a | | |
| 1b | | |
| 1c | addr* |  |
| 1d | | |
| 1e | mode | |
| 1f | exec |  |

### Screen Device

| Port | Name | Status |
| --- | --- | --- | 
| 20 | vector* | Done |
| 21 |  |  |
| 22 | width* | Done |
| 23 |  |  |
| 24 | height* | Done |
| 25 |  |  |
| 26 | auto | Done |
| 27 |  |  |
| 28 | x* | Done |
| 29 |  |  |
| 2a | y* | Done |
| 2b |  |  |
| 2c | addr* | Done |
| 2d |  |  |
| 2e | pixel | Done |
| 2f | sprite | Done |

### Audio Device

| Port | Name | Status |
| --- | --- | --- | 
| 30 | vector* |  |
| 31 |  |  |
| 32 | position* |  |
| 33 |  |  |
| 34 | output |  |
| 35 |  |  |
| 36 |  |  |
| 37 |  |  |
| 38 | adsr* |  |
| 39 |  |  |
| 3a | length* |  |
| 3b |  |  |
| 3c | addr* |  |
| 3d |  |  |
| 3e | volume |  |
| 3f | pitch |  |

### Controller Device

| Port | Name | Status |
| --- | --- | --- | 
| 80 | vector* |  |
| 81 |  |  |
| 82 | button |  |
| 83 | key |  |
| 84 |  |  |
| 85 | P2 |  |
| 86 | P3 |  |
| 87 | P4 |  |
| 88 |  |  |
| 89 |  |  |
| 8a |  |  |
| 8b |  |  |
| 8c |  |  |
| 8d |  |  |
| 8e |  |  |
| 8f |  |  |

### Mouse Device

| Port | Name | Status |
| --- | --- | --- | 
| 90 | vector* |  |
| 91 |  |  |
| 92 | x* |  |
| 93 |  |  |
| 94 | y* |  |
| 95 |  |  |
| 96 | state |  |
| 97 |  |  |
| 98 |  |  |
| 99 |  |  |
| 9a | scrollx* |  |
| 9b |  |  |
| 9c | scrolly* |  |
| 9d |  |  |
| 9e |  |  |
| 9f |  |  |

### File Device

| Port | Name | Status |
| --- | --- | --- | 
| a0 | vector* |  |
| a1 |  |  |
| a2 | success* |  |
| a3 |  |  |
| a4 | stat* |  |
| a5 |  |  |
| a6 | delete |  |
| a7 | append |  |
| a8 | name* |  |
| a9 |  |  |
| aa | length* |  |
| ab |  |  |
| ac | read* |  |
| ad |  |  |
| ae | write* |  |
| af |  |  |

### Datetime Device

| Port | Name | Status |
| --- | --- | --- | 
| c0 | year* |  |
| c1 |  |  |
| c2 | month |  |
| c3 | day |  |
| c4 | hour |  |
| c5 | minute |  |
| c6 | second |  |
| c7 | dotw |  |
| c8 | doty |  |
| c9 |  |  |
| ca | isdst |  |
| cb |  |  |
| cc |  |  |
| cd |  |  |
| ce |  |  |
| cf |  |  |