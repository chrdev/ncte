# Neo CD-TEXT Editor (NCTE)

NCTE may be the only publicly available CDTEXT editor that supports MS-JIS (and UTF-16BE)

## Feature List

- Supports Windows 10 and above with High DPI.

- Supports all CD-TEXT fields except for 89h (2nd TOC, planned feature). Those include
    1.  Title
	2.  Artist
	3.  Songwriter
	4.  Composer
	5.  Arranger
	6.  Message
	7.  Album Catalog Number
	8.  Album UPC / EAN code
	9.  Track ISRC
	10. Closed Information
	11. TOC (Table of Contents)

- Supports text encodings of
    1. ISO 8859-1
    2. ASCII
    3. MS-JIS: full Shift-JIS, and a small portion of gaiji for roman numericals and circled numbers. Caution: gaiji support are based on speculation and untested
    4. UTF-16BE: undefined by CD-TEXT standards, this is a customized option. Use it only when you know what you are doing

- Regulates inputs
    1. User inputs are regulated to conform to the encoding standard
	2. When using MS-JIS, uncovered kanji and symbols are replaced by their best-matched counterparts
	
- Supports multiple formats
    1. .cdt and .bin binary files, read and write
	2. .txt 0.7T sheet files, read only
	3. Dump from CD directly

## Burnning a CD

cdrecord.exe -raw96r textfile=file.cdt cuefile=file.cue


Neo CD-TEXT Editor

## 录入汉字的建议

## MIT License

Copyright 2021 chrdev

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
