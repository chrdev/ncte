# Neo CDTEXT Editor (NCTE)

NCTE is a CDTEXT editor that supports MS-JIS and UTF-16BE.

中文请见下。

## MIT License

## Feature List

- Supports Windows 10 and Windows 11 with High DPI.

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

# NCTE CDTEXT 编辑器

NCTE 是录入 CDTEXT 的专用编辑器，支持 MS-JIS 和 UTF-16BE。

## 录入汉字的建议

1. 在 Block2 设置语言为 UTF-16BE， 75h Chinese。 然后录入完整信息。

2. 在 Block2 点右键选择复制，在 Block1 点右键选择粘贴。然后设置语言为 MS-JIS， 75h Chinese。

3. 保存。

这样可保持最大兼容商用播放机，并且完全正确的保存汉字信息。UTF-16BE 区块能被电脑软件和定制 CD 播放机读取。
