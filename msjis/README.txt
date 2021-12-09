Creative Commons

# Study on MS-JIS

MS-JIS, a.k.a Music Shift-JIS, RIS-506-1996, RIAJ, ミュージック JIS (Music JIS), レコード用文字符号 (Characters and Symbols for Recordings), is a combination of Shift-JIS and 656 gaiji characters.

No known official / unofficial documentation available on the Internet.

## Shift-JIS part

The Shift-JIS part is clear thanks to UNICODE
https://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/JIS/SHIFTJIS.TXT

NCTE has full Shift-JIS coverage. It also replaces characters not found in Shift-JIS to covered characters, although the result can't be 100% accurate, usability prevails.

NCTE also replaces single-byte characters, like ASCII, to their double-byte counterparts, because bincheck.exe (from Sony CDTEXT package) doesn't seem to like the mixture.

## Gaiji(外字) part

According to this page
https://web.archive.org/web/20120916223857/http://www.culti.co.jp/PRO_bitmap/bit_MUSIC.html

The gaiji part contains 297 full-width characters and 359 half-width characters, make a total of 656.

The gaiji table is available at
http://www.enfour.co.jp/media/music/MusicBitmap.html

This table contains 37 * 17 + 25 = 654 characters, makeing a discrepancy of 2.

By using software to generate fake MS-JIS codes and feed them to bincheck.exe, we can see that bincheck.exe acknowledges the following range:

First      Last      Count
--------------------------
0xF640     0xF67E       63
0xF680     0xF6B9       58
0xF6BF     0xF6FB       61
0xF740     0xF765       38
0xF79F     0xF7EB       77
---------------------- 297
0xF840     0xF87E       63
0xF880     0xF8FC      125
0xF940     0xF97E       63
0xF980     0xF9EC      109
---------------------- 360
                 total 657

We can guess that the first 297 counts are full-width characters. NCTE contains the more frequently needed roman numerals, as well as circled numbers from 1 to 20, based on this guessing.

However, the total number doesn't match, the study stalled at this point.

## Where to look

The MS-JIS documentation can be found at a larger library, or datasheets for some micro controllers, eg. Epson S5U1C88000R1, Hitachi HD66735.
If I would had one of these, NCTE can cover full MS-JIS.
