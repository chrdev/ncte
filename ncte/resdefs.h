// Used by rc files, so no enum, only #define
#pragma once

// Legends
// TLabel: TrackLabel
// ALabel: AlbumLabel

#define kRcMargin 7
#define kRcSpacingW 3
#define kRcSpacingH 3
#define kRcToolMargin 2

//#define kRcNavW 58
//#define kRcNavItemH 32

#define kRcTLabelY 6
#define kRcTLabelX 31
#define kRcTEditY 5
#define kRcSpinW 4
#define kRcSpinH 12
#define kRcTrackEditW 18
#define kRcTextH 8


#define kRcComboH 64
#define kRcTextOffsetY 1
#define kRcEditH 12
#define kRcEditW 248
#define kRcLineH (kRcEditH + kRcSpacingH)
#define kRcIconButtonH 16
#define kRcIconButtonW 18
#define kRcButtonH 14
#define kRcButtonW 60


#define kEnuEditX 76
#define kEnuCombo0W 108

#define kRcAlbumFixedLineCount  4
#define kRcTrackFixedLineCount  1

#define kRcLine0Y  kRcMargin
#define kRcLine1Y  (kRcLineH + kRcLine0Y)
#define kRcLine2Y  (kRcLineH + kRcLine1Y)
#define kRcLine3Y  (kRcLineH + kRcLine2Y)
#define kRcLine4Y  (kRcLineH + kRcLine3Y)
#define kRcLine5Y  (kRcLineH + kRcLine4Y)
#define kRcLine6Y  (kRcLineH + kRcLine5Y)
#define kRcLine7Y  (kRcLineH + kRcLine6Y)
#define kRcLine8Y  (kRcLineH + kRcLine7Y)
#define kRcLine9Y  (kRcLineH + kRcLine8Y)
#define kRcLine10Y (kRcLineH + kRcLine9Y)
#define kRcLine11Y (kRcLineH + kRcLine10Y)

#define kRcToolbarFileW (kRcToolMargin + kRcIconButtonW + kRcIconButtonW + kRcIconButtonW + kRcToolMargin)
#define kRcToolbarH (kRcIconButtonH + kRcToolMargin + kRcToolMargin)
#define kRcPanelW (kEnuEditX + kRcEditW + kRcMargin)
#define kRcAlbumH (kRcLine11Y + kRcEditH + kRcMargin)
#define kRcTrackH (kRcLine7Y + kRcEditH + kRcMargin)

//#define kRcLine0Y 7
//#define kRcLine1Y 20
//#define kRcLine2Y 33
//#define kRcLine3Y 46
//#define kRcLine4Y 59
//#define kRcLine5Y 72
//#define kRcLine6Y 85
//#define kRcLine7Y 98
//#define kRcLine8Y 111
//#define kRcLine9Y 124
//#define kRcLine10Y 137

#define kRcMsjisSubstitutes 101
#define kRcUtf16ToMsjis     102
#define kRcMsjisToUtf16     103
#define kRcIconFont	        104

#define kRtFont             300

#define kDlgAlbum           101
#define kDlgTrack           102
#define kDlgMemTrack        103
#define kDlgToolbar         104
						   
#define kDlgOpenPanel       105
						   
						   
#define kMenuNavPopup       200
#define kEditCopy           202
#define kEditPaste          203
#define kShiftUp            204
#define kShiftDown          205
#define kEditDelete         206
						   
#define kCopyAsEac          207
#define kCopyAsTest         208

#define kIdStatic (-1)

#define kIdLanguageLabel    1001
#define kIdCopyProtection   1002
#define kIdEncoding         1003
#define kIdLanguage         1004
#define kIdGenreLabel       1005
#define kIdGenre            1006
#define kIdGenreExtra       1007
#define kIdTitleLabel       1008
#define kIdTitle            1009
#define kIdArtistLabel      1010
#define kIdArtist           1011
#define kIdSongwriterLabel  1012
#define kIdSongwriter       1013
#define kIdComposerLabel    1014
#define kIdComposer         1015
#define kIdArrangerLabel    1016
#define kIdArranger         1017
#define kIdCatalogLabel     1018
#define kIdCatalog          1019
#define kIdCodeLabel        1020
#define kIdCode             1021
#define kIdMessageLabel     1022
#define kIdMessage          1023
#define kIdClosedInfoLabel  1024
#define kIdClosedInfo       1025
#define kIdTocLabel         1026
#define kIdToc              1027
#define kIdTrackNumber      1028
						   
#define kIdSep1             1039
#define kIdNew              1040
#define kIdOpen             1041
#define kIdSave             1042
						   
#define kIdTrackLabel       1043
#define kIdTrackCount       1044
#define kIdTrackFirst       1045
#define kIdTrackFirstSpin   1046
#define kIdTrackHyphen      1047
#define kIdTrackLast        1048
#define kIdTrackLastSpin    1049
#define kIdAlbumFields      1050
#define kIdTrackFields      1051

#define kIdSaveDump         1052
#define kIdExtract          1053

#define kTextMainWndTitle   101
#define kTextAbout          102

#define kTextTrackCount         107
#define kTextFieldsSelectorHelp 108
#define kTextCopyProtection     109

#define kTextTitle       110
#define kTextArtist      111
#define kTextSongwriter  112
#define kTextComposer    113
#define kTextArranger    114
#define kTextCatalog     115
#define kTextUpcEan      116
#define kTextIsrc        117
#define kTextMessage     118
#define kTextClosedInfo  119
#define kTextToc         120
#define kTextAlbumFieldsSelectorWidth 121
#define kTextTrackFieldsSelectorWidth 122

#define kTextPromptDelete         130
#define kTextPromptAbandonUnsaved 131
#define kTextCdNoText             132
#define kTextCdNoCd               133
#define kTextCdOtherError         134

#define kTextFileSaveError        135
#define kTextFileOpenError        136


#define kText8859        151
#define kTextAscii       152
#define kTextMsjis	     153
#define kTextUtf16be     154

#define kTextNotUsed     155
#define kTextNotDefined  156

#define kTextAdultContemporary       178
#define kTextAlternativeRock         179
#define kTextChildrensMusic          180
#define kTextClassical               181
#define kTextContemporaryChristian   182
#define kTextCountry                 183
#define kTextDance                   184
#define kTextEasyListening           185
#define kTextErotic                  186
#define kTextFolk                    187
#define kTextGospel                  188
#define kTextHipHop                  189
#define kTextJazz                    190
#define kTextLatin                   191
#define kTextMusical                 192
#define kTextNewAge                  193
#define kTextOpera                   194
#define kTextOperetta                195
#define kTextPopMusic                196
#define kTextRap                     197
#define kTextReggae                  198
#define kTextRockMusic               199
#define kTextRhythmAndBlues          200
#define kTextSoundEffects            201
#define kTextSoundTrack              202
#define kTextSpokenWord              203
#define kTextWorldMusic              204

#define kTextEnglish            357
#define kTextGerman             358
#define kTextFrench             359
#define kTextSpanish            360
#define kTextItalian            361
#define kTextJapanese           362
#define kTextKorean             363
#define kTextChinese            364
#define kTextPortuguese         365
#define kTextDutch              366
#define kTextDanish             367
#define kTextSwedish            368
#define kTextNorwegian          369
#define kTextFinnish            370
#define kTextPolish             371
#define kTextRussian            372
#define kTextSlovenian          373
#define kTextHungarian          374
#define kTextCzech              375
#define kTextGreek              376
#define kTextTurkish            377
