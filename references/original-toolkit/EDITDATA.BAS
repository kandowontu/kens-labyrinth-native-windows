numwalls% = 448
numboards% = 30
OPTION BASE 0
DIM regs%(7): regs%(0) = 0: CALL int86old(&H33, regs%(), regs%())
DIM pal%(768), cl%(16, 3), palookup%(256)
DIM buf%(64, 64), buf2%(64, 64), buf3%(64, 64), buf4%(4096)
DIM walstatus%(numwalls%), typ%(numwalls%)
boardnum& = 0: wallnum& = 0: anywallstatchg% = 0: updatecnt% = 0
OPEN "boards.dat" FOR BINARY AS #1
OPEN "walls.dat" FOR BINARY AS #2
SEEK #2, 1
buf$ = STRING$(1024, 0)
GET #2, , buf$
FOR z% = 0 TO numwalls% - 1
   walstatus%(z%) = ASC(MID$(buf$, z% + 1, 1))
NEXT z%
SCREEN 13
LOCATE 25, 1: PRINT "Loading data...";
GOSUB setupcolors
FOR z% = 0 TO numwalls%
   READ typ%(z%)
NEXT z%
LOCATE 25, 1: PRINT "               ";
palval% = 0
trailstatus% = 0
z$ = ""
DO UNTIL z$ = CHR$(27)
   IF z$ <> CHR$(27) THEN GOSUB editboards
   IF z$ <> CHR$(27) THEN GOSUB editwalls
LOOP
IF anywallstatchg% = 1 THEN
   buf$ = ""
   FOR z% = 0 TO numwalls% - 1
      buf$ = buf$ + CHR$(walstatus%(z%))
   NEXT z%
   SEEK #2, 1
   PUT #2, , buf$
END IF
CLOSE #1
CLOSE #2
END

editboards:
   GOSUB loadnewboard
   c% = 31: d% = 31
   mousx% = 0: mousy% = 0
   col% = 0
   GOSUB printcol
   DO
      DO
         LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), 15, BF
         GOSUB mouse
         z$ = UCASE$(INKEY$)
         OUT &H3C8, 255
         WAIT &H3DA, 8
         OUT &H3C9, palval%: OUT &H3C9, palval%: OUT &H3C9, palval%
         palval% = (palval% + 1) AND 63
      LOOP WHILE z$ = "" AND mousx% = 0 AND mousy% = 0
      LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), buf%(c%, d%), BF
      IF trailstatus% = 0 THEN LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), buf%(c%, d%), BF
      IF trailstatus% = 1 THEN LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), col%, BF: buf%(c%, d%) = col%: asksave% = 1
      IF z$ = CHR$(0) + CHR$(72) AND d% > 0 THEN d% = d% - 1
      IF z$ = CHR$(0) + CHR$(80) AND d% < 63 THEN d% = d% + 1
      IF z$ = CHR$(0) + CHR$(75) AND c% > 0 THEN c% = c% - 1
      IF z$ = CHR$(0) + CHR$(77) AND c% < 63 THEN c% = c% + 1
      IF z$ = "\" THEN c% = 31: d% = 31
      IF z$ = "|" THEN col% = 4096: GOSUB printsprite
      IF z$ = CHR$(0) + CHR$(73) AND boardnum& > 0 THEN
         IF asksave% = 1 THEN GOSUB testsaveboardchanges
         boardnum& = boardnum& - 1
         GOSUB loadnewboard
      END IF
      IF z$ = CHR$(0) + CHR$(81) AND boardnum& < numboards% - 1 THEN
         IF asksave% = 1 THEN GOSUB testsaveboardchanges
         boardnum& = boardnum& + 1
         GOSUB loadnewboard
      END IF
      c% = c% + mousx%: mousx% = 0
      d% = d% + mousy%: mousy% = 0
      IF c% < 0 THEN c% = 0
      IF c% > 63 THEN c% = 63
      IF d% < 0 THEN d% = 0
      IF d% > 63 THEN d% = 63
      IF z$ = "?" THEN
         CLS
         FOR z% = 0 TO 1023
            buf4%(z%) = 0
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               dat% = buf%(z%, zz%) AND 1023
               IF typ%(dat%) = 1 THEN buf4%(dat%) = buf4%(dat%) + 1
            NEXT zz%
         NEXT z%
         cnt% = 0
         FOR zx% = 1 TO numwalls%
            snotbut% = 0
            dat% = 0
            FOR zzx% = 1 TO numwalls%
               IF buf4%(zzx%) > snotbut% AND (buf4%(zzx%) AND 4096) = 0 THEN
                  snotbut% = buf4%(zzx%)
                  dat% = zzx%
               END IF
            NEXT zzx%
            IF buf4%(dat%) = 0 THEN EXIT FOR
            IF buf4%(dat%) > 0 THEN
               p& = (cnt% \ 10) * 320 + (cnt% MOD 10)
               p& = p& * 32
               walnume& = dat% - 1
               GOSUB showall2
               LOCATE (cnt% \ 10) * 4 + 1, (cnt% MOD 10) * 4 + 1
               PRINT LTRIM$(RTRIM$(STR$(buf4%(dat%))));
               buf4%(dat%) = 0
               cnt% = cnt% + 1
            END IF
         NEXT zx%
         FOR z% = 0 TO 1023
            buf4%(z%) = 0
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               dat% = buf%(z%, zz%) AND 1023
               IF typ%(dat%) = 2 THEN buf4%(dat%) = buf4%(dat%) + 1
            NEXT zz%
         NEXT z%
         cnt% = 0
         FOR zx% = 1 TO numwalls%
            snotbut% = 0
            dat% = 0
            FOR zzx% = 1 TO numwalls%
               IF buf4%(zzx%) > snotbut% AND (buf4%(zzx%) AND 4096) = 0 THEN
                  snotbut% = buf4%(zzx%)
                  dat% = zzx%
               END IF
            NEXT zzx%
            IF buf4%(dat%) = 0 THEN EXIT FOR
            IF buf4%(dat%) > 0 THEN
               p& = (cnt% \ 10) * 320 + (cnt% MOD 10)
               p& = (p& + 40) * 32 + 21120&
               walnume& = dat% - 1
               GOSUB showall2
               LOCATE (cnt% \ 10) * 4 + 10, (cnt% MOD 10) * 4 + 1
               PRINT LTRIM$(RTRIM$(STR$(buf4%(dat%))));
               buf4%(dat%) = 0
               cnt% = cnt% + 1
            END IF
         NEXT zx%
         FOR z% = 0 TO 1023
            buf4%(z%) = 0
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               dat% = buf%(z%, zz%) AND 1023
               IF typ%(dat%) = 3 THEN buf4%(dat%) = buf4%(dat%) + 1
            NEXT zz%
         NEXT z%
         cnt% = 0
         FOR zx% = 1 TO numwalls%
            snotbut% = 0
            dat% = 0
            FOR zzx% = 1 TO numwalls%
               IF buf4%(zzx%) > snotbut% AND (buf4%(zzx%) AND 4096) = 0 THEN
                  snotbut% = buf4%(zzx%)
                  dat% = zzx%
               END IF
            NEXT zzx%
            IF buf4%(dat%) = 0 THEN EXIT FOR
            IF buf4%(dat%) > 0 THEN
               p& = (cnt% \ 10) * 320 + (cnt% MOD 10)
               p& = (p& + 40) * 32 + 42240
               walnume& = dat% - 1
               GOSUB showall2
               LOCATE (cnt% \ 10) * 4 + 18, (cnt% MOD 10) * 4 + 1
               PRINT LTRIM$(RTRIM$(STR$(buf4%(dat%))));
               buf4%(dat%) = 0
               cnt% = cnt% + 1
            END IF
         NEXT zx%
         DO: LOOP WHILE INKEY$ = ""
         CLS
         GOSUB setupcolors
         GOSUB printcol
         GOSUB loadnewboard
      END IF
      IF z$ = "1" THEN x1% = c%: y1% = d%
      IF z$ = "2" THEN
         x2% = c%: y2% = d%
         IF x1% > x2% THEN SWAP x1%, x2%
         IF y1% > y2% THEN SWAP y1%, y2%
         FOR z% = x1% TO x2%
            FOR zz% = y1% TO y2%
               buf3%(z% - x1%, zz% - y1%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         x2% = x2% - x1%
         y2% = y2% - y1%
         x1% = 0
         y1% = 0
      END IF
      IF z$ = "*" THEN GOSUB count.items
      IF z$ = "_" THEN GOSUB clear.items
      IF z$ = "4" THEN
         FOR z% = 0 TO x2% \ 2
            FOR zz% = 0 TO y2%
               SWAP buf3%(z%, zz%), buf3%(x2% - z%, zz%)
            NEXT zz%
         NEXT z%
         SOUND 262, .1
      END IF
      IF z$ = "5" THEN
         FOR z% = 0 TO x2%
            FOR zz% = 0 TO y2% \ 2
               SWAP buf3%(z%, zz%), buf3%(z%, y2% - zz%)
            NEXT zz%
         NEXT z%
         SOUND 262, .1
      END IF
      IF z$ = "6" THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF z% > zz% THEN SWAP buf3%(z%, zz%), buf3%(zz%, z%)
            NEXT zz%
         NEXT z%
         SWAP x2%, y2%
         SOUND 262, .1
      END IF
      IF z$ = "3" THEN
         asksave% = 1
         FOR z% = 0 TO x2%
            FOR zz% = 0 TO y2%
               IF z% + c% >= 0 AND z% + c% < 64 AND zz% + d% >= 0 AND zz% + d% < 64 THEN
                  IF buf3%(z%, zz%) <> 255 THEN buf%(z% + c%, zz% + d%) = buf3%(z%, zz%)
               END IF
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "-" AND (col% AND 1023) > 0 THEN
         col% = col% - 1
         IF (col% AND 4096) = 0 THEN
            IF (col% AND 1023) > 0 AND (col% AND 1023) <= numwalls% THEN
               walattr% = walstatus%((col% + 1023) AND 1023)
               col% = col% AND 1023
               IF (walattr% AND 8) > 0 THEN col% = col% OR 1024
               IF (walattr% AND 16) > 0 THEN col% = col% OR 2048
            END IF
         END IF
         GOSUB printsprite
      END IF
      IF z$ = "+" AND (col% AND 1023) < 1023 THEN
         col% = col% + 1
         IF (col% AND 4096) = 0 THEN
            IF (col% AND 1023) > 0 AND (col% AND 1023) <= numwalls% THEN
               walattr% = walstatus%((col% + 1023) AND 1023)
               col% = col% AND 1023
               IF (walattr% AND 8) > 0 THEN col% = col% OR 1024
               IF (walattr% AND 16) > 0 THEN col% = col% OR 2048
            END IF
         END IF
         GOSUB printsprite
      END IF
      IF z$ = " " OR bstatus% > 0 THEN buf%(c%, d%) = col%: asksave% = 1
      IF z$ = "I" OR z$ = "i" THEN col% = col% XOR 1024: GOSUB printsprite
      IF z$ = "B" OR z$ = "b" THEN col% = col% XOR 2048: GOSUB printsprite
      IF z$ = "W" OR z$ = "w" THEN col% = col% XOR 8192: GOSUB printsprite
      IF z$ = "A" THEN GOSUB addboard
      IF z$ = "T" OR z$ = "t" THEN trailstatus% = 1 - trailstatus%: GOSUB printcol
      IF z$ = "J" THEN
         asksave% = 1
         col1% = buf%(c%, d%)
         col2% = col%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF buf%(z%, zz%) = col1% THEN
                  IF INT(10 * RND) = 0 THEN buf%(z%, zz%) = col2%
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
         SOUND 262, .1
      END IF
      IF z$ = "`" THEN
         LOCATE 2, 26: INPUT "Board?"; board%
         IF board% > 0 AND board% <= numboards% THEN
            IF asksave% = 1 THEN GOSUB testsavewallchanges
            boardnum& = board% - 1
            GOSUB loadnewboard
         ELSE
            SOUND 60, 1
         END IF
         LOCATE 2, 26: PRINT "            ";
      END IF
      IF z$ = "R" THEN
         asksave% = 1
         LOCATE 2, 26: PRINT "Rotate way?";
         DO
            z$ = UCASE$(INKEY$)
         LOOP WHILE z$ = ""
         LOCATE 2, 26: PRINT "           ";
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf3%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         IF z$ = CHR$(0) + CHR$(72) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%(z%, (zz% + 63) AND 63)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
         IF z$ = CHR$(0) + CHR$(80) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%(z%, (zz% + 1) AND 63)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
         IF z$ = CHR$(0) + CHR$(75) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%((z% + 63) AND 63, zz%)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
         IF z$ = CHR$(0) + CHR$(77) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%((z% + 1) AND 63, zz%)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
      END IF
      IF z$ = "S" THEN
         LOCATE 2, 26: INPUT "Search?"; z%
         z% = z% AND 255
         IF z% >= 0 AND z% <= 255 THEN
            OUT &H3C7, z%
            oldcol1% = INP(&H3C9)
            oldcol2% = INP(&H3C9)
            oldcol3% = INP(&H3C9)
            zz% = 0
            DO
               zz% = (zz% + 1) AND 255
               WAIT &H3DA, 8
               WAIT &H3DA, 8, 255
               OUT &H3C8, z%
               OUT &H3C9, zz%
               OUT &H3C9, zz%
               OUT &H3C9, zz%
            LOOP WHILE INKEY$ = ""
            OUT &H3C8, z%
            OUT &H3C9, oldcol1%
            OUT &H3C9, oldcol2%
            OUT &H3C9, oldcol3%
         END IF
         LOCATE 2, 26: PRINT "            ";
      END IF
      IF z$ = "M" THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf3%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "P" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf%(z%, zz%) = buf3%(z%, zz%)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "C" THEN
         asksave% = 1
         chanfromcol% = (buf%(c%, d%) AND 1023)
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF (buf%(z%, zz%) AND 1023) = chanfromcol% THEN
                  buf%(z%, zz%) = (col% AND 1023) + (buf%(z%, zz%) AND &HFC00)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), col%, BF
               END IF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "F" THEN
         asksave% = 1
         LINE (0, 0)-(191, 191), col%, B
         PAINT (c% * 3 + 1, d% * 3 + 1), col%, col%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               datz% = POINT(z% * 3 + 1, zz% * 3 + 1)
               IF datz% = col% THEN buf%(z%, zz%) = col%
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = CHR$(9) THEN col% = buf%(c%, d%): GOSUB printsprite
   LOOP WHILE z$ <> CHR$(27) AND z$ <> CHR$(13)
   IF asksave% = 1 THEN GOSUB testsaveboardchanges
RETURN

editwalls:
   GOSUB loadnewall
   DEF SEG = &HA000
   c% = 31: d% = 31
   mousx% = 0: mousy% = 0
   col% = 0
   asksave% = 0
   GOSUB printcol
   DO
      DO
         WAIT &H3DA, 8
         WAIT &H3DA, 8, 255
         LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), (buf%(c%, d%) + 147) MOD 252, BF
         WAIT &H3DA, 8
         WAIT &H3DA, 8, 255
         LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), buf%(c%, d%), BF
         GOSUB mouse
         z$ = UCASE$(INKEY$)
         OUT &H3C8, 255
         WAIT &H3DA, 8
         OUT &H3C9, palval%: OUT &H3C9, palval%: OUT &H3C9, palval%
         palval% = (palval% + 1) AND 63
      LOOP WHILE z$ = "" AND mousx% = 0 AND mousy% = 0
      DEF SEG = 0: keystate% = PEEK(&H417)
      IF trailstatus% = 0 THEN LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), buf%(c%, d%), BF
      IF trailstatus% = 1 THEN LINE (c% * 3, d% * 3)-(c% * 3 + 2, d% * 3 + 2), col%, BF: buf%(c%, d%) = col%: asksave% = 1
      IF z$ = "R" THEN
         asksave% = 1
         LOCATE 2, 26: PRINT "Rotate way?";
         DO
            z$ = UCASE$(INKEY$)
         LOOP WHILE z$ = ""
         LOCATE 2, 26: PRINT "           ";
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf3%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         IF z$ = CHR$(0) + CHR$(72) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%(z%, (zz% + 63) AND 63)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
         IF z$ = CHR$(0) + CHR$(80) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%(z%, (zz% + 1) AND 63)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
         IF z$ = CHR$(0) + CHR$(75) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%((z% + 63) AND 63, zz%)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
         IF z$ = CHR$(0) + CHR$(77) THEN
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  buf%(z%, zz%) = buf3%((z% + 1) AND 63, zz%)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               NEXT zz%
            NEXT z%
         END IF
      END IF
      IF z$ = "\" THEN c% = 31: d% = 31
      IF z$ = "|" THEN LOCATE 18, 30: PRINT c%; d%; "  ";
      IF (keystate% AND 7) = 0 THEN
         IF z$ = CHR$(0) + CHR$(72) AND d% > 0 THEN d% = d% - 1
         IF z$ = CHR$(0) + CHR$(80) AND d% < 63 THEN d% = d% + 1
         IF z$ = CHR$(0) + CHR$(75) AND c% > 0 THEN c% = c% - 1
         IF z$ = CHR$(0) + CHR$(77) AND c% < 63 THEN c% = c% + 1
      END IF
      IF (keystate% AND 3) > 0 THEN
         GOSUB clearcol
         IF z$ = CHR$(0) + CHR$(72) AND col% > 0 THEN col% = col% - 1
         IF z$ = CHR$(0) + CHR$(80) AND col% < 255 THEN col% = col% + 1
         IF z$ = CHR$(0) + CHR$(75) AND col% > 15 THEN col% = col% - 16
         IF z$ = CHR$(0) + CHR$(77) AND col% < 240 THEN col% = col% + 16
         GOSUB printcol
      END IF
      IF z$ = "0" THEN
         band% = col% AND 240
         FOR z% = 63 TO 0 STEP -1
            FOR zz% = 63 TO 0 STEP -1
               IF (buf%(z%, zz%) AND 240) <> band% THEN
                  IF (buf%((z% + 63) AND 63, (zz% + 63) AND 63) AND 240) = band% THEN
                     buf%(z%, zz%) = 255
                     LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
                  END IF
                  IF (buf%((z% + 62) AND 63, (zz% + 62) AND 63) AND 240) = band% THEN
                     buf%(z%, zz%) = 255
                     LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
                  END IF
                  IF (buf%((z% + 61) AND 63, (zz% + 61) AND 63) AND 240) = band% THEN
                     buf%(z%, zz%) = 255
                     LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
                  END IF
                  IF (buf%((z% + 60) AND 63, (zz% + 60) AND 63) AND 240) = band% THEN
                     buf%(z%, zz%) = 255
                     LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
                  END IF
               END IF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = CHR$(0) + CHR$(59) THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF SQR((z% - 31.5) ^ 2 + (zz% - 31.5) ^ 2) >= 32 THEN
                  buf%(z%, zz%) = 255
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = CHR$(0) + CHR$(60) THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf%(z%, zz%) = INT(SQR(z% ^ 2 + zz% ^ 2) / 89.1 * 64)
               buf%(z%, zz%) = buf%(z%, zz%) + INT(SQR((63 - z%) ^ 2 + zz% ^ 2) / 89.1 * 64)
               buf%(z%, zz%) = buf%(z%, zz%) + INT(SQR(z% ^ 2 + (63 - zz%) ^ 2) / 89.1 * 64)
               buf%(z%, zz%) = buf%(z%, zz%) + INT(SQR((63 - z%) ^ 2 + (63 - zz%) ^ 2) / 89.1 * 64)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = CHR$(0) + CHR$(61) THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf%(z%, zz%) = (INT(140 + 18 * COS(z% * 3.141592 / 16) + 18 * SIN(zz% * 3.141592 / 16)) AND 255) + 40
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = CHR$(0) + CHR$(62) THEN
         LOCATE 25, 1: INPUT "Wallbackground: "; wallnumber&
         wallnumber& = wallnumber& - 1
         GOSUB loadnewall2
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF buf%(z%, zz%) = 0 OR buf%(z%, zz%) = 255 THEN buf%(z%, zz%) = buf2%(z%, zz%)
            NEXT zz%
         NEXT z%
         CLS
         GOSUB setupcolors
         LOCATE 24, 30: PRINT "Sprite"; wallnum& + 1;
         attrval% = walstatus%(wallnum&): GOSUB printattrib
         GOSUB printcol
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
         asksave% = 1
         LOCATE 25, 1: PRINT "                              ";
      END IF
      IF z$ = CHR$(0) + CHR$(63) THEN
         LOCATE 25, 1: INPUT "Wallbackground: "; wallnumber&
         wallnumber& = wallnumber& - 1
         GOSUB loadnewall2
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf3%(z%, zz%) = buf%(z%, zz%)
               IF buf%(z%, zz%) = 0 OR buf%(z%, zz%) = 255 THEN
                  bz% = z%: bzz% = zz%
                  IF bz% < 1 THEN bz% = 1
                  IF bz% > 63 THEN bz% = 63
                  IF bzz% < 1 THEN bzz% = 1
                  IF bzz% > 63 THEN bzz% = 63
                  IF buf%(bz% - 1, bzz%) = 0 OR buf%(bz% - 1, bzz%) = 255 THEN
                     bz% = z%: bzz% = zz%
                     IF bz% < 1 THEN bz% = 1
                     IF bz% > 63 THEN bz% = 63
                     IF bzz% < 1 THEN bzz% = 1
                     IF bzz% > 63 THEN bzz% = 63
                     IF buf%(bz%, bzz% - 1) = 0 OR buf%(bz%, bzz% - 1) = 255 THEN
                        bz% = z%: bzz% = zz%
                        IF bz% < 0 THEN bz% = 0
                        IF bz% > 63 THEN bz% = 63
                        IF bzz% < 1 THEN bzz% = 1
                        IF bzz% > 63 THEN bzz% = 63
                        IF buf%(bz% + 1, bzz%) = 0 OR buf%(bz% + 1, bzz%) = 255 THEN
                           bz% = z%: bzz% = zz%
                           IF bz% < 1 THEN bz% = 1
                           IF bz% > 63 THEN bz% = 63
                           IF bzz% < 0 THEN bzz% = 0
                           IF bzz% > 63 THEN bzz% = 63
                           IF buf%(bz%, bzz% + 1) = 0 OR buf%(bz%, bzz% + 1) = 255 THEN
                              bz% = z%: bzz% = zz%
                              IF bz% < 1 THEN bz% = 1
                              IF bz% > 63 THEN bz% = 63
                              IF bzz% < 1 THEN bzz% = 1
                              IF bzz% > 63 THEN bzz% = 63
                              IF buf%(bz% - 1, bzz% - 1) = 0 OR buf%(bz% - 1, bzz% - 1) = 255 THEN
                                 bz% = z%: bzz% = zz%
                                 IF bz% < 0 THEN bz% = 0
                                 IF bz% > 63 THEN bz% = 63
                                 IF bzz% < 0 THEN bzz% = 0
                                 IF bzz% > 63 THEN bzz% = 63
                                 IF buf%(bz% + 1, bzz% + 1) = 0 OR buf%(bz% + 1, bzz% + 1) = 255 THEN
                                    bz% = z%: bzz% = zz%
                                    IF bz% < 1 THEN bz% = 1
                                    IF bz% > 63 THEN bz% = 63
                                    IF bzz% < 0 THEN bzz% = 0
                                    IF bzz% > 63 THEN bzz% = 63
                                    IF buf%(bz% - 1, bzz% + 1) = 0 OR buf%(bz% - 1, bzz% + 1) = 255 THEN
                                       bz% = z%: bzz% = zz%
                                       IF bz% < 0 THEN bz% = 0
                                       IF bz% > 63 THEN bz% = 63
                                       IF bzz% < 1 THEN bzz% = 1
                                       IF bzz% > 63 THEN bzz% = 63
                                       IF buf%(bz% + 1, bzz% - 1) = 0 OR buf%(bz% + 1, bzz% - 1) = 255 THEN
                                          buf3%(z%, zz%) = buf2%(z%, zz%)
                                       END IF
                                    END IF
                                 END IF
                              END IF
                           END IF
                        END IF
                     END IF
                  END IF
               END IF
            NEXT zz%
         NEXT z%
         CLS
         GOSUB setupcolors
         LOCATE 24, 30: PRINT "Sprite"; wallnum& + 1;
         attrval% = walstatus%(wallnum&): GOSUB printattrib
         GOSUB printcol
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf%(z%, zz%) = buf3%(z%, zz%)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
         asksave% = 1
         LOCATE 25, 1: PRINT "                              ";
      END IF
      IF z$ = CHR$(0) + CHR$(64) THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF (INT(SQR((z% - 31.5) ^ 2 + (zz% - 31.5) ^ 2)) AND 3) = 1 THEN
                  buf%(z%, zz%) = 255
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = CHR$(0) + CHR$(65) THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF (INT(SQR((z% - 31.5) ^ 2 + (zz% - 31.5) ^ 2)) AND 3) = 3 THEN
                  buf%(z%, zz%) = 255
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = CHR$(0) + CHR$(66) THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf%(z%, zz%) = INT(SQR(z% ^ 2 + zz% ^ 2) / 89.1 * 6)
               buf%(z%, zz%) = buf%(z%, zz%) + INT(SQR((63 - z%) ^ 2 + zz% ^ 2) / 89.1 * 6) * 42
               buf%(z%, zz%) = buf%(z%, zz%) + INT(SQR(z% ^ 2 + (63 - zz%) ^ 2) / 89.1 * 4) * 6
               buf%(z%, zz%) = ((buf%(z%, zz%) + INT(SQR((63 - z%) ^ 2 + (63 - zz%) ^ 2) / 89.1 * 4) * 6) AND 255)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = "1" THEN x1% = c%: y1% = d%
      IF z$ = "2" THEN
         x2% = c%: y2% = d%
         IF x1% > x2% THEN SWAP x1%, x2%
         IF y1% > y2% THEN SWAP y1%, y2%
         FOR z% = x1% TO x2%
            FOR zz% = y1% TO y2%
               buf3%(z% - x1%, zz% - y1%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         x2% = x2% - x1%
         y2% = y2% - y1%
         x1% = 0
         y1% = 0
      END IF
      IF z$ = "4" THEN
         FOR z% = 0 TO x2% \ 2
            FOR zz% = 0 TO y2%
               SWAP buf3%(z%, zz%), buf3%(x2% - z%, zz%)
            NEXT zz%
         NEXT z%
         SOUND 262, .1
      END IF
      IF z$ = "5" THEN
         FOR z% = 0 TO x2%
            FOR zz% = 0 TO y2% \ 2
               SWAP buf3%(z%, zz%), buf3%(z%, y2% - zz%)
            NEXT zz%
         NEXT z%
         SOUND 262, .1
      END IF
      IF z$ = "6" THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF z% > zz% THEN SWAP buf3%(z%, zz%), buf3%(zz%, z%)
            NEXT zz%
         NEXT z%
         SWAP x2%, y2%
         SOUND 262, .1
      END IF
      IF z$ = "7" THEN
         asksave% = 1
         col1% = buf%(c%, d%)
         col2% = col%
         newcol% = (col1% + col2%) \ 2
         IF col1% = 240 THEN newcol% = (col2% AND &HF0) + (col2% AND 15) \ 2
         IF col2% = 240 THEN newcol% = (col1% AND &HF0) + (col1% AND 15) \ 2
         IF (newcol% = col1% OR newcol% = col2% OR ((col1% \ 16) <> (col2% \ 16))) AND col1% <> 240 AND col2% <> 240 THEN
            SOUND 37, 2
         ELSE
            FOR z% = 0 TO 63
               FOR zz% = 0 TO 63
                  IF buf%(z%, zz%) = col1% THEN
                     IF buf%((z% + 63) AND 63, zz%) = col2% THEN
                        buf%((z% + 63) AND 63, zz%) = newcol%
                        LINE (((z% + 63) AND 63) * 3, zz% * 3)-((((z% + 63) AND 63)) * 3 + 2, zz% * 3 + 2), buf%(((z% + 63) AND 63), zz%), BF
                     END IF
                  END IF
                  IF buf%(z%, zz%) = col1% THEN
                     IF buf%((z% + 1) AND 63, zz%) = col2% THEN
                        buf%((z% + 1) AND 63, zz%) = newcol%
                        LINE (((z% + 1) AND 63) * 3, zz% * 3)-((((z% + 1) AND 63)) * 3 + 2, zz% * 3 + 2), buf%(((z% + 1) AND 63), zz%), BF
                     END IF
                  END IF
                  IF buf%(z%, zz%) = col1% THEN
                     IF buf%(z%, (zz% + 63) AND 63) = col2% THEN
                        buf%(z%, (zz% + 63) AND 63) = newcol%
                        LINE (z% * 3, ((zz% + 63) AND 63) * 3)-(z% * 3 + 2, ((zz% + 63) AND 63) * 3 + 2), buf%(z%, (zz% + 63) AND 63), BF
                     END IF
                  END IF
                  IF buf%(z%, zz%) = col1% THEN
                     IF buf%(z%, (zz% + 1) AND 63) = col2% THEN
                        buf%(z%, (zz% + 1) AND 63) = newcol%
                        LINE (z% * 3, ((zz% + 1) AND 63) * 3)-(z% * 3 + 2, ((zz% + 1) AND 63) * 3 + 2), buf%(z%, (zz% + 1) AND 63), BF
                     END IF
                  END IF
               NEXT zz%
            NEXT z%
            SOUND 262, .1
         END IF
      END IF
      IF z$ = "g" OR z$ = "G" THEN
         FOR z% = 0 TO 63 STEP 2
            FOR zz% = 0 TO 63
               IF (buf%(z%, zz%) AND 15) > 0 THEN
                  buf%(z%, zz%) = buf%(z%, zz%) - 1
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
               IF (buf%(zz%, z%) AND 15) > 0 THEN
                  buf%(zz%, z%) = buf%(zz%, z%) - 1
                  LINE (zz% * 3, z% * 3)-(zz% * 3 + 2, z% * 3 + 2), buf%(zz%, z%), BF
               END IF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = "," OR z$ = "<" THEN
         zstep% = SGN(c% - x1%)
         zzstep% = SGN(d% - y1%)
         IF zstep% = 0 THEN zstep% = 1
         IF zzstep% = 0 THEN zzstep% = 1
         FOR z% = x1% TO c% STEP zstep%
            FOR zz% = y1% TO d% STEP zzstep%
               IF (buf%(z%, zz%) AND 15) > 0 THEN
                  IF z$ = "," THEN buf%(z%, zz%) = buf%(z%, zz%) - 1
                  IF z$ = "<" THEN buf%(z%, zz%) = (buf%(z%, zz%) + 240) AND 255
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = "." OR z$ = ">" THEN
         zstep% = SGN(c% - x1%)
         zzstep% = SGN(d% - y1%)
         IF zstep% = 0 THEN zstep% = 1
         IF zzstep% = 0 THEN zzstep% = 1
         FOR z% = x1% TO c% STEP zstep%
            FOR zz% = y1% TO d% STEP zzstep%
               IF (buf%(z%, zz%) AND 15) < 15 THEN
                  IF z$ = "." THEN buf%(z%, zz%) = buf%(z%, zz%) + 1
                  IF z$ = ">" THEN buf%(z%, zz%) = (buf%(z%, zz%) + 16) AND 255
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
         asksave% = 1
      END IF
      IF z$ = "/" THEN
         FOR z% = x1% TO x2%
            FOR zz% = y1% TO y2%
               IF z% < 32 AND zz% < 32 THEN buf3%(z%, zz%) = buf3%(z% * 2, zz% * 2)
            NEXT zz%
         NEXT z%
         x2% = x2% \ 2: y2% = y2% \ 2
      END IF
      IF z$ = "*" THEN
         x2% = x2% * 2 + 1: y2% = y2% * 2 + 1
         FOR z% = x2% TO x1% STEP -1
            FOR zz% = y2% TO y1% STEP -1
               IF z% < 64 AND zz% < 64 THEN buf3%(z%, zz%) = buf3%(z% \ 2, zz% \ 2)
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "l" OR z$ = "L" THEN
         x1 = x1%: y1 = y1%
         x2 = c%: y2 = d%
         GOSUB drawline
         asksave% = 1
      END IF
      IF z$ = "e" OR z$ = "E" THEN
         x1 = x1%: y1 = y1%
         r = SQR((x1% - c%) ^ 2 + (y1% - d%) ^ 2)
         GOSUB drawcircle
         asksave% = 1
      END IF
      IF z$ = "3" THEN
         asksave% = 1
         FOR z% = 0 TO x2%
            FOR zz% = 0 TO y2%
               IF z% + c% >= 0 AND z% + c% < 64 AND zz% + d% >= 0 AND zz% + d% < 64 THEN
                  IF buf3%(z%, zz%) <> 255 THEN buf%(z% + c%, zz% + d%) = buf3%(z%, zz%)
               END IF
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "T" OR z$ = "t" THEN trailstatus% = 1 - trailstatus%: GOSUB printcol
      IF z$ = "`" THEN
         LOCATE 2, 26: INPUT "Wall?"; wall%
         IF wall% > 0 AND wall% <= numwalls% THEN
            IF asksave% = 1 THEN GOSUB testsavewallchanges
            wallnum& = wall% - 1
            GOSUB loadnewall
         ELSE
            SOUND 60, 1
         END IF
         LOCATE 2, 26: PRINT "            ";
      END IF
      IF z$ = "~" THEN
         snotcmp% = (wallnum& + 1) AND 1023
         LOCATE 1, 1
         FOR zx& = 0 TO numboards% - 1
            SEEK #1, zx& * 8192& + 1
            buf$ = STRING$(8192, 0)
            GET #1, , buf$
            LOCATE (zx& MOD 10) + 1, (zx& \ 10) * 10 + 1
            PRINT zx& + 1;
            x% = 0
            FOR z% = 1 TO 8191 STEP 2
               snot% = ASC(MID$(buf$, z%, 1))
               snot% = snot% + 256 * ASC(MID$(buf$, z% + 1, 1))
               IF (snot% AND 1023) = snotcmp% AND (snot% AND 4096) = 0 THEN x% = x% + 1
            NEXT z%
            PRINT x%
         NEXT zx&
         DO: LOOP WHILE INKEY$ = ""
         CLS
         GOSUB setupcolors
         GOSUB loadnewall
         GOSUB printcol
      END IF
      IF z$ = CHR$(0) + CHR$(73) AND wallnum& > 0 THEN
         IF asksave% = 1 THEN GOSUB testsavewallchanges
         wallnum& = wallnum& - 1
         GOSUB loadnewall
      END IF
      IF z$ = CHR$(0) + CHR$(81) AND wallnum& < numwalls% - 1 THEN
         IF asksave% = 1 THEN GOSUB testsavewallchanges
         wallnum& = wallnum& + 1
         GOSUB loadnewall
      END IF
      c% = c% + mousx%: mousx% = 0
      d% = d% + mousy%: mousy% = 0
      IF c% < 0 THEN c% = 0
      IF c% > 63 THEN c% = 63
      IF d% < 0 THEN d% = 0
      IF d% > 63 THEN d% = 63
      IF z$ = CHR$(8) THEN GOSUB clearcol: col% = 255: GOSUB printcol
      IF z$ = " " OR bstatus% > 0 THEN buf%(c%, d%) = col%: asksave% = 1
      IF z$ = "K" THEN
         walstatus%(wallnum&) = (walstatus%(wallnum&) AND (255 - 7)) + (((walstatus%(wallnum&) AND 7) + 1) MOD 3)
         LOCATE 21, 30: PRINT "Kind=";
         IF (walstatus%(wallnum&) AND 7) = 0 THEN PRINT "cube ";
         IF (walstatus%(wallnum&) AND 7) = 1 THEN PRINT "plane";
         IF (walstatus%(wallnum&) AND 7) = 2 THEN PRINT "direc";
         anywallstatchg% = 1
      END IF
      IF z$ = "I" THEN
         walstatus%(wallnum&) = walstatus%(wallnum&) XOR 8
         LOCATE 22, 30: PRINT "Inside=";
         IF (walstatus%(wallnum&) AND 8) = 0 THEN PRINT "off";
         IF (walstatus%(wallnum&) AND 8) > 0 THEN PRINT "on ";
         anywallstatchg% = 1
      END IF
      IF z$ = "B" THEN
         walstatus%(wallnum&) = walstatus%(wallnum&) XOR 16
         LOCATE 23, 30: PRINT "Blast=";
         IF (walstatus%(wallnum&) AND 16) = 0 THEN PRINT "off";
         IF (walstatus%(wallnum&) AND 16) > 0 THEN PRINT "on ";
         anywallstatchg% = 1
      END IF
      IF z$ = "A" THEN GOSUB addwall
      IF z$ = "U" THEN GOSUB updatewall
      IF z$ = CHR$(0) + CHR$(18) THEN
         OPEN updat$ + ".bmp" FOR BINARY AS #3
         buf$ = STRING$(3, 0)
         GET #3, , buf$
         IF MID$(buf$, 3, 1) = "6" THEN
            snot% = 1
            filoffs& = ASC(MID$(buf$, 3, 1)) + 1024 + 1
            buf$ = STRING$(ASC(MID$(buf$, 3, 1)) - 3, 0): GET #3, , buf$
            buf$ = ""
            OUT &H3C7, 0
            FOR z% = 0 TO 255
               c3% = INP(&H3C9) * 4
               c2% = INP(&H3C9) * 4
               c1% = INP(&H3C9) * 4
               buf$ = buf$ + CHR$(c1%) + CHR$(c2%) + CHR$(c3%) + CHR$(0)
            NEXT z%
            PUT #3, , buf$
         ELSE
            snot% = 0
            filoffs& = 26 + 768 + 1
            buf$ = STRING$(23, 0): GET #3, , buf$
            buf$ = ""
            OUT &H3C7, 0
            FOR z% = 0 TO 255
               c1% = INP(&H3C9) * 4
               c2% = INP(&H3C9) * 4
               c3% = INP(&H3C9) * 4
               buf$ = buf$ + CHR$(c1%) + CHR$(c2%) + CHR$(c3%)
            NEXT z%
            PUT #3, , buf$
         END IF
         FOR zz& = updateplcy% TO updateplcy% + 63
            SEEK #3, (199& - zz&) * 320& + updateplcx% + filoffs&
            z$ = ""
            FOR z& = updateplcx% TO updateplcx% + 63
               z$ = z$ + CHR$(buf%(z& - updateplcx%, zz& - updateplcy%))
            NEXT z&
            PUT #3, , z$
         NEXT zz&
         CLOSE #3
         SOUND 262, 1
      END IF
      IF z$ = "V" THEN GOSUB showallwalls
      IF z$ = "S" THEN
         LOCATE 2, 26: INPUT "Search?"; z%
         z% = z% AND 255
         IF z% >= 0 AND z% <= 255 THEN
            OUT &H3C7, z%
            oldcol1% = INP(&H3C9)
            oldcol2% = INP(&H3C9)
            oldcol3% = INP(&H3C9)
            zz% = 0
            DO
               zz% = (zz% + 1) AND 255
               WAIT &H3DA, 8
               WAIT &H3DA, 8, 255
               OUT &H3C8, z%
               OUT &H3C9, zz%
               OUT &H3C9, zz%
               OUT &H3C9, zz%
            LOOP WHILE INKEY$ = ""
            OUT &H3C8, z%
            OUT &H3C9, oldcol1%
            OUT &H3C9, oldcol2%
            OUT &H3C9, oldcol3%
         END IF
         LOCATE 2, 26: PRINT "            ";
      END IF
      IF z$ = "M" THEN
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf3%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "P" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf%(z%, zz%) = buf3%(z%, zz%)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "C" THEN
         asksave% = 1
         chanfromcol% = buf%(c%, d%)
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF buf%(z%, zz%) = chanfromcol% THEN
                  buf%(z%, zz%) = col%
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), col%, BF
               END IF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "J" THEN
         asksave% = 1
         col1% = buf%(c%, d%)
         col2% = col%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               IF buf%(z%, zz%) = col1% THEN
                  IF INT(10 * RND) = 0 THEN buf%(z%, zz%) = col2%
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
         SOUND 262, .1
      END IF
      IF z$ = "F" THEN
         asksave% = 1
         LINE (0, 0)-(191, 191), col%, B
         PAINT (c% * 3 + 1, d% * 3 + 1), col%, col%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf%(z%, zz%) = POINT(z% * 3 + 1, zz% * 3 + 1)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "'" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf2%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               band% = buf2%(z%, zz%) AND &HF0
               snot% = 0
               totzx% = 0
               FOR zx% = 1 TO 6
                  x% = INT(7 * RND)
                  y% = INT(7 * RND)
                  IF ABS(x% + y%) > 4 THEN
                     x% = INT(7 * RND)
                     y% = INT(7 * RND)
                  END IF
                  avg% = buf2%((x% + z% + 63) AND 63, (y% + zz% + 63) AND 63)
                  IF (avg% AND &HF0) = band% THEN
                     snot% = snot% + avg%
                     totzx% = totzx% + 1
                  END IF
               NEXT zx%
               IF totzx% > 0 THEN
                  snot% = snot% \ totzx%
                  buf%(z%, zz%) = snot% + (buf2%(z%, zz%) - snot%) * 1.5
                  IF (buf%(z%, zz%) AND &HF0) < band% THEN buf%(z%, zz%) = band%
                  IF (buf%(z%, zz%) AND &HF0) > band% + 15 THEN buf%(z%, zz%) = band% + 15
               END IF
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = ":" THEN
         zz% = 0
         DO
            zz% = (zz% + 1) AND 255
            WAIT &H3DA, 8
            WAIT &H3DA, 8, 255
            FOR z% = 0 TO 240 STEP 16
               OUT &H3C8, z%
               OUT &H3C9, zz%
               OUT &H3C9, zz%
               OUT &H3C9, zz%
               OUT &H3C8, z% + 15
               OUT &H3C9, zz%
               OUT &H3C9, zz%
               OUT &H3C9, zz%
            NEXT z%
         LOOP WHILE INKEY$ = ""
         GOSUB setupcolors
      END IF
      IF z$ = ";" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf2%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               band% = buf2%(z%, zz%) AND &HF0
               FOR zx% = 1 TO 1
                  x% = INT(3 * RND)
                  y% = INT(3 * RND)
                  IF ABS(x% + y%) > 1 THEN
                     x% = INT(3 * RND)
                     y% = INT(3 * RND)
                  END IF
                  dat% = buf2%((x% + z% + 63) AND 63, (y% + zz% + 63) AND 63)
                  IF (dat% AND &HF0) <> band% THEN
                     IF (buf%(z%, zz%) AND 15) < (dat% AND 15) - 1 THEN buf%(z%, zz%) = buf%(z%, zz%) + 1
                     IF (buf%(z%, zz%) AND 15) > (dat% AND 15) + 1 THEN buf%(z%, zz%) = buf%(z%, zz%) - 1
                  END IF
               NEXT zx%
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "]" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf2%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               band% = buf2%(z%, zz%) AND &HF0
               snot% = 0
               totzx% = 0
               FOR zx% = 1 TO 16
                  x% = INT(3 * RND)
                  y% = INT(3 * RND)
                  IF ABS(x% + y%) > 1 THEN
                     x% = INT(3 * RND)
                     y% = INT(3 * RND)
                  END IF
                  avg% = buf2%((x% + z% + 63) AND 63, (y% + zz% + 63) AND 63)
                  IF (avg% AND &HF0) = band% THEN
                     snot% = snot% + avg%
                     totzx% = totzx% + 1
                  END IF
               NEXT zx%
               IF totzx% > 0 THEN buf%(z%, zz%) = snot% \ totzx%
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "[" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf2%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               band% = buf2%(z%, zz%) AND &HF0
               snot% = 0
               totzx% = 0
               FOR zx% = 1 TO 6
                  x% = INT(3 * RND)
                  y% = INT(3 * RND)
                  IF ABS(x% + y%) > 1 THEN
                     x% = INT(3 * RND)
                     y% = INT(3 * RND)
                  END IF
                  avg% = buf2%((x% + z% + 63) AND 63, (y% + zz% + 63) AND 63)
                  IF (avg% AND &HF0) = band% THEN
                     snot% = snot% + (avg% AND 15) * (avg% AND 15)
                     totzx% = totzx% + 1
                  END IF
               NEXT zx%
               IF totzx% > 0 THEN
                  snot% = SQR(snot% \ totzx%)
                  buf%(z%, zz%) = (buf%(z%, zz%) AND &HF0) + (snot% AND 15)
                  LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
               END IF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "}" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf2%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               band% = buf2%(z%, zz%) AND &HF0
               colband% = buf%(z%, zz%) AND 15
               FOR zx% = 1 TO 2
                  x% = INT(3 * RND) - 1
                  y% = INT(3 * RND) - 1
                  IF ABS(x% + y%) > 1 THEN
                     x% = INT(3 * RND)
                     y% = INT(3 * RND)
                  END IF
                  datzx% = buf2%((x% + z% + 64) AND 63, (y% + zz% + 64) AND 63)
                  IF (datzx% AND &HF0) <> band% THEN
                     IF x% + y% > 0 AND colband% > 0 THEN colband% = colband% - 1
                     IF x% + y% < 0 AND colband% < 15 THEN colband% = colband% + 1
                  END IF
               NEXT zx%
               buf%(z%, zz%) = (buf%(z%, zz%) AND &HF0) OR (colband% AND 15)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "?" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf2%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               band% = buf2%(z%, zz%) AND &HF0
               colband% = buf%(z%, zz%) AND 15
               FOR zx% = 1 TO 2
                  x% = INT(3 * RND) - 1
                  y% = INT(3 * RND) - 1
                  IF ABS(x% + y%) > 1 THEN
                     x% = INT(3 * RND)
                     y% = INT(3 * RND)
                  END IF
                  datzx% = buf2%((x% + z% + 64) AND 63, (y% + zz% + 64) AND 63)
                  IF (datzx% AND &HF0) <> band% AND (datzx% AND &HF0) = (col% AND &HF0) THEN
                     IF x% + y% > 0 AND colband% > 0 THEN colband% = colband% - 1
                     IF x% + y% < 0 AND colband% < 15 THEN colband% = colband% + 1
                  END IF
               NEXT zx%
               buf%(z%, zz%) = (buf%(z%, zz%) AND &HF0) OR (colband% AND 15)
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = "{" THEN
         asksave% = 1
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               buf2%(z%, zz%) = buf%(z%, zz%)
            NEXT zz%
         NEXT z%
         FOR z% = 0 TO 63
            FOR zz% = 0 TO 63
               band% = buf2%(z%, zz%) AND &HF0
               FOR zx% = 1 TO 2
                  x% = INT(5 * RND) - 1
                  y% = INT(5 * RND) - 1
                  IF ABS(x% + y%) > 3 THEN
                     x% = INT(5 * RND)
                     y% = INT(5 * RND)
                  END IF
                  IF ABS(x% + y%) > 1 THEN
                     x% = INT(5 * RND)
                     y% = INT(5 * RND)
                  END IF
                  datzx% = buf2%((x% + z% + 64) AND 63, (y% + zz% + 64) AND 63)
                  IF (datzx% AND &HF0) = band% THEN
                     IF (datzx% AND 15) < (buf2%(z%, zz%) AND 15) AND (buf%(z%, zz%) AND 15) > 0 THEN buf%(z%, zz%) = buf%(z%, zz%) - 1
                     IF (datzx% AND 15) > (buf2%(z%, zz%) AND 15) AND (buf%(z%, zz%) AND 15) < 15 THEN buf%(z%, zz%) = buf%(z%, zz%) + 1
                  END IF
               NEXT zx%
               LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
            NEXT zz%
         NEXT z%
      END IF
      IF z$ = CHR$(9) THEN GOSUB clearcol: col% = buf%(c%, d%): GOSUB printcol
      IF z$ = "+" THEN
          GOSUB clearcol: col% = buf%(c%, d%)
          IF (col% AND 15) < 15 THEN col% = col% + 1 ELSE SOUND 37, 2
          GOSUB printcol
          buf%(c%, d%) = col%
          LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
          asksave% = 1
      END IF
      IF z$ = "-" THEN
          GOSUB clearcol: col% = buf%(c%, d%)
          IF (col% AND 15) > 0 THEN col% = col% - 1 ELSE SOUND 37, 2
          GOSUB printcol
          buf%(c%, d%) = col%
          LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
          asksave% = 1
      END IF
   LOOP WHILE z$ <> CHR$(27) AND z$ <> CHR$(13)
   IF asksave% = 1 THEN GOSUB testsavewallchanges
RETURN

mouse:
   regs%(0) = 11: CALL int86old(&H33, regs%(), regs%())
   mousx% = mousx% + regs%(2) \ 2: mousy% = mousy% + regs%(3) \ 2
   regs%(0) = 5: CALL int86old(&H33, regs%(), regs%())
   bstatus% = regs%(0)
RETURN

printsprite:
   LOCATE 1, 26: PRINT (col% AND 1023);
   IF (col% AND 1023) < 100 THEN
      PRINT " ";
      IF (col% AND 1023) < 10 THEN PRINT " ";
   END IF
   DEF SEG = &HA000
   IF col% > 0 AND (col% AND 4096) = 0 THEN
      SEEK #2, ((col% + 1023) AND 1023) * 4096& + 1025
      tempbuf$ = STRING$(4096, 0)
      GET #2, , tempbuf$
      DEF SEG = VARSEG(tempbuf$)
      offs& = SADD(tempbuf$)
      FOR zz% = 0 TO 63
         FOR zzz% = 0 TO 63
            buf3%(zz%, zzz%) = PEEK(offs&)
            offs& = offs& + 64
         NEXT zzz%
         offs& = offs& - 4095
      NEXT zz%
      DEF SEG = &HA000
      p% = 240
      FOR zz% = 0 TO 63
         FOR zzz% = 0 TO 63
            POKE p%, buf3%(zz%, zzz%)
            p% = p% + 1
         NEXT zzz%
         p% = p% + 256
      NEXT zz%
   ELSE
      LINE (240, 0)-(303, 63), 0, BF
   END IF
   IF (col% AND 4096) > 0 THEN
      LOCATE 18, 30: PRINT "Start=";
      IF (col% AND 1023) = 0 THEN PRINT "R";
      IF (col% AND 1023) = 1 THEN PRINT "D";
      IF (col% AND 1023) = 2 THEN PRINT "L";
      IF (col% AND 1023) = 3 THEN PRINT "U";
   ELSE
      LOCATE 18, 30: PRINT "       ";
   END IF
   IF (col% AND 1023) > 0 AND (col% AND 1023) <= numwalls% THEN
      IF ((walstatus%((col% - 1) AND 1023) AND 7) = 2) THEN
         LOCATE 21, 30: PRINT "Way=";
         IF (col% AND 8192) = 0 THEN PRINT "horiz";
         IF (col% AND 8192) > 0 THEN PRINT "verti";
      ELSE
         LOCATE 21, 30: PRINT "         ";
      END IF
   ELSE
      LOCATE 21, 30: PRINT "         ";
   END IF
   LOCATE 22, 30: PRINT "Inside=";
   IF (col% AND 1024) = 0 THEN PRINT "off";
   IF (col% AND 1024) > 0 THEN PRINT "on ";
   LOCATE 23, 30: PRINT "Blast=";
   IF (col% AND 2048) = 0 THEN PRINT "off";
   IF (col% AND 2048) > 0 THEN PRINT "on ";
RETURN
       
clearcol:
   LOCATE 20, 30: PRINT "TRAIL ";
   IF trailstatus% = 0 THEN PRINT "OFF";  ELSE PRINT "ON ";
   x% = 203 + (col% \ 16) * 3
   y% = 75 + (col% AND 15) * 3
   PSET (x% + 1, y% + 1), col%
RETURN

printcol:
   LOCATE 1, 26: PRINT col%; "    ";
   LOCATE 20, 30: PRINT "TRAIL ";
   IF trailstatus% = 0 THEN PRINT "OFF";  ELSE PRINT "ON ";
   x% = 203 + (col% \ 16) * 3
   y% = 75 + (col% AND 15) * 3
   PSET (x% + 1, y% + 1), (col% AND &HF0) + (15 - (col% AND &HF))
RETURN

setupcolors:
   RESTORE
   FOR z% = 0 TO 15
      READ cl%(z%, 0), cl%(z%, 1), cl%(z%, 2)
   NEXT z%
   OUT &H3C8, 0
   FOR z% = 0 TO 15
      FOR zz% = 1 TO 16
         OUT &H3C9, (cl%(z%, 0) * zz%) \ 17
         OUT &H3C9, (cl%(z%, 1) * zz%) \ 17
         OUT &H3C9, (cl%(z%, 2) * zz%) \ 17
      NEXT zz%
   NEXT z%
   OUT &H3C7, 0
   FOR z% = 0 TO 767
      pal%(z%) = INP(&H3C9)
   NEXT z%
   FOR z% = 0 TO 15
      FOR zz% = 0 TO 15
         LINE (z% * 3 + 203, zz% * 3 + 75)-(z% * 3 + 205, zz% * 3 + 77), z% * 16 + zz%, BF
      NEXT zz%
   NEXT z%
RETURN
  
testsavewallchanges:
   LOCATE 25, 1: PRINT "Save changes?";
   zx$ = INPUT$(1)
   IF zx$ = "y" OR zx$ = "Y" THEN
      buf$ = ""
      FOR z% = 0 TO 63
         FOR zz% = 0 TO 63
            buf$ = buf$ + CHR$(buf%(z%, zz%))
         NEXT zz%
      NEXT z%
      SEEK #2, wallnum& * 4096& + 1025
      PUT #2, , buf$
   END IF
   LOCATE 25, 1: PRINT "             ";
   asksave% = 0
RETURN

testsaveboardchanges:
   LOCATE 25, 1: PRINT "Save changes?";
   zx$ = INPUT$(1)
   IF zx$ = "y" OR zx$ = "Y" THEN
      buf$ = ""
      FOR z% = 0 TO 63
         FOR zz% = 0 TO 63
            buf$ = buf$ + CHR$(buf%(z%, zz%) AND 255)
            buf$ = buf$ + CHR$(buf%(z%, zz%) \ 256)
         NEXT zz%
      NEXT z%
      SEEK #1, boardnum& * 8192& + 1
      PUT #1, , buf$
   END IF
   LOCATE 25, 1: PRINT "             ";
   asksave% = 0
RETURN

loadnewall:
   LOCATE 24, 30: PRINT "Sprite"; wallnum& + 1;
   attrval% = walstatus%(wallnum&): GOSUB printattrib
   SEEK #2, wallnum& * 4096& + 1025
   buf$ = STRING$(4096, 0)
   GET #2, , buf$
   DEF SEG = VARSEG(buf$)
   offs& = SADD(buf$)
   FOR z% = 0 TO 63
      FOR zz% = 0 TO 63
         buf%(z%, zz%) = PEEK(offs&): offs& = offs& + 1
         LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
      NEXT zz%
   NEXT z%
   asksave% = 0
RETURN

loadnewall2:
   'LOCATE 24, 30: PRINT "Sprite"; wallnum& + 1;
   'attrval% = walstatus%(wallnum&): GOSUB printattrib
   SEEK #2, wallnumber& * 4096& + 1025
   buf$ = STRING$(4096, 0)
   GET #2, , buf$
   FOR zz% = 0 TO 63
      FOR z% = 0 TO 63
         buf2%(z%, zz%) = ASC(MID$(buf$, zz% + 64 * z% + 1, 1))
      NEXT z%
   NEXT zz%
   asksave% = 0
RETURN

loadnewboard:
   LOCATE 24, 30: PRINT "Board"; boardnum& + 1; "  ";
   LOCATE 21, 30: PRINT "           ";
   LOCATE 22, 30: PRINT "           ";
   LOCATE 23, 30: PRINT "           ";
   SEEK #1, boardnum& * 8192& + 1
   buf$ = STRING$(8192, 0)
   GET #1, , buf$
   GOSUB printsprite
   episode% = (boardnum& \ 10) + 1
   FOR zz% = 0 TO 63
      FOR z% = 0 TO 63
         buf%(z%, zz%) = ASC(MID$(buf$, 2 * (zz% + 64 * z%) + 1, 1))
         buf%(z%, zz%) = buf%(z%, zz%) + 256 * ASC(MID$(buf$, 2 * (zz% + 64 * z%) + 2, 1))
         LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
         dat% = buf%(z%, zz%) AND 1023
         IF typ%(dat%) > episode% THEN LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), 255
      NEXT z%
   NEXT zz%
   asksave% = 0
RETURN

showallwalls:
   IF asksave% = 1 THEN GOSUB testsavewallchanges
   regs%(0) = &H38: CALL int86old(&H10, regs%(), regs%())
   DEF SEG = &HA000
   GOSUB setupcolors
   tempbuf$ = STRING$(4096, 0)
   FOR z% = 0 TO 191
      walnume& = z%
      GOSUB showall
   NEXT z%
   place& = 0
   DO
      OUT &H3C8, 255
      WAIT &H3DA, 8
      OUT &H3C9, palval%: OUT &H3C9, palval%: OUT &H3C9, palval%
      palval% = (palval% + 1) AND 63
      z$ = INKEY$
      IF z$ = CHR$(0) + CHR$(72) AND place& > 0 THEN
         place& = place& - 65536
         OUT &H3D4, &H33: OUT &H3D5, (place& \ 262144) AND 3
         OUT &H3D4, &HC: OUT &H3D5, (place& \ 1024) AND 255
         OUT &H3D4, &HD: OUT &H3D5, (place& \ 4) AND 255
         FOR z% = 0 TO 15
            walnume& = z% + (place& \ 4096)
            GOSUB showall
         NEXT z%
      END IF
      IF z$ = CHR$(0) + CHR$(80) AND place& < 1048576 THEN
         place& = place& + 65536
         OUT &H3D4, &H33: OUT &H3D5, (place& \ 262144) AND 3
         OUT &H3D4, &HC: OUT &H3D5, (place& \ 1024) AND 255
         OUT &H3D4, &HD: OUT &H3D5, (place& \ 4) AND 255
         FOR z% = 176 TO 191
            walnume& = z% + (place& \ 4096)
            GOSUB showall
         NEXT z%
      END IF
   LOOP WHILE z$ <> CHR$(27)
   z$ = ""
   regs%(0) = &H13: CALL int86old(&H10, regs%(), regs%())
   DEF SEG = &HA000
   GOSUB setupcolors
   GOSUB loadnewall
   GOSUB printcol
RETURN

showall:
   SEEK #2, walnume& * 4096& + 1025&
   GET #2, , tempbuf$
   p& = (walnume& AND 15) * 64&
   OUT &H3CD, ((walnume& \ 16) AND 15)
   zx% = 1
   DEF SEG = VARSEG(tempbuf$)
   offs& = SADD(tempbuf$)
   FOR zz% = 0 TO 63
      FOR zzz% = 0 TO 63
         buf3%(zz%, zzz%) = PEEK(offs&)
         offs& = offs& + 64
      NEXT zzz%
      offs& = offs& - 4095
   NEXT zz%
   DEF SEG = &HA000
   FOR zz% = 0 TO 63
      FOR zzz% = 0 TO 63
         POKE p&, buf3%(zz%, zzz%)
         p& = p& + 1
      NEXT zzz%
      p& = p& + 960
   NEXT zz%
RETURN

showall2:
   SEEK #2, walnume& * 4096& + 1025&
   tempbuf$ = STRING$(4096, 0)
   GET #2, , tempbuf$
   DEF SEG = VARSEG(tempbuf$)
   offs& = SADD(tempbuf$)
   FOR zz% = 0 TO 63 STEP 2
      FOR zzz% = 0 TO 63 STEP 2
         buf3%(zz%, zzz%) = PEEK(offs&)
         offs& = offs& + 128
      NEXT zzz%
      offs& = offs& - 4094
   NEXT zz%
   DEF SEG = &HA000
   FOR zz% = 0 TO 63 STEP 2
      FOR zzz% = 0 TO 63 STEP 2
         POKE p&, buf3%(zz%, zzz%)
         p& = p& + 1
      NEXT zzz%
      p& = p& + 320 - 32
   NEXT zz%
RETURN

addwall:
   LOCATE 25, 1: PRINT "What wall file"; : INPUT zx$
   OPEN zx$ FOR BINARY AS #3
   LOCATE 25, 1: PRINT STRING$(40, 32);
   LOCATE 25, 1: PRINT "What wall number"; : INPUT zx&
   IF zx& > 0 THEN
      zx& = zx& - 1
      SEEK #3, zx& * 4096& + 1025
   END IF
   buf$ = STRING$(4096, 0)
   GET #3, , buf$
   CLOSE #3
   CLS
   GOSUB setupcolors
   attrval% = walstatus%(wallnum&): GOSUB printattrib
   GOSUB printcol
   FOR z% = 0 TO 63
      FOR zz% = 0 TO 63
         buf%(z%, zz%) = ASC(MID$(buf$, zz% + 64 * z% + 1, 1))
         LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
      NEXT zz%
   NEXT z%
   asksave% = 1
RETURN

addboard:
   LOCATE 25, 1: PRINT "What board file"; : INPUT zx$
   OPEN zx$ FOR BINARY AS #3
   LOCATE 25, 1: PRINT STRING$(40, 32);
   LOCATE 25, 1: PRINT "What board number"; : INPUT zx&
   SEEK #3, (zx& - 1) * 8192& + 1
   buf$ = STRING$(8192, 0)
   GET #3, , buf$
   CLOSE #3
   CLS
   GOSUB setupcolors
   LOCATE 24, 30: PRINT "Board"; boardnum& + 1; " ";
   LOCATE 23, 30: PRINT "           ";
   GOSUB printcol
   FOR z% = 0 TO 63
      FOR zz% = 0 TO 63
         buf%(z%, zz%) = ASC(MID$(buf$, 2 * (zz% + 64 * z%) + 1, 1))
         buf%(z%, zz%) = buf%(z%, zz%) + 256 * ASC(MID$(buf$, 2 * (zz% + 64 * z%) + 2, 1))
         LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
      NEXT zz%
   NEXT z%
   asksave% = 1
RETURN

drawline:
   IF x1 > x2 THEN SWAP x1, x2: SWAP y1, y2
   IF x1 = x2 THEN m = 99999999 ELSE m = (y2 - y1) / (x2 - x1)
   B = y1 - m * x1
   IF x1 < 1 THEN x1 = 1
   IF y1 < 1 THEN y1 = 1
   IF x1 > 62 THEN x1 = 62
   IF y1 > 62 THEN y1 = 62
   IF x2 < 1 THEN x2 = 1
   IF y2 < 1 THEN y2 = 1
   IF x2 > 62 THEN x2 = 62
   IF y2 > 62 THEN y2 = 62
   IF y1 < y2 THEN
      FOR x = x1 - 1 TO x2 + 1
         FOR y = y1 - 1 TO y2 + 1
            dist = ABS(15 * (y - m * x - B) / SQR(m * m + 1))
            IF dist <= 15 THEN
               buf%(x, y) = 15 - dist + (col% AND 240)
               LINE (x * 3, y * 3)-(x * 3 + 2, y * 3 + 2), buf%(x, y), BF
            END IF
         NEXT y
      NEXT x
   ELSE
      FOR x = x1 - 1 TO x2 + 1
         FOR y = y1 + 1 TO y2 - 1 STEP -1
            dist = ABS(15 * (y - m * x - B) / SQR(m * m + 1))
            IF dist <= 15 THEN
               buf%(x, y) = 15 - dist + (col% AND 240)
               LINE (x * 3, y * 3)-(x * 3 + 2, y * 3 + 2), buf%(x, y), BF
            END IF
         NEXT y
      NEXT x
   END IF
RETURN

drawcircle:
   FOR x = x1 - INT(r) - 5 TO x1 + INT(r) + 5
      FOR y = y1 - INT(r) - 5 TO y1 + INT(r) + 5
         IF x >= 0 AND y >= 0 AND x <= 63 AND y <= 63 THEN
            dist = 15 * ABS(SQR((x - x1) ^ 2 + (y - y1) ^ 2) - r)
            IF dist <= 15 THEN
               buf%(x, y) = 15 - dist + (col% AND 240)
               LINE (x * 3, y * 3)-(x * 3 + 2, y * 3 + 2), buf%(x, y), BF
            END IF
         END IF
      NEXT y
   NEXT x
RETURN

printattrib:
   LOCATE 21, 30: PRINT "Kind=";
   IF (attrval% AND 7) = 0 THEN PRINT "cube ";
   IF (attrval% AND 7) = 1 THEN PRINT "plane";
   IF (attrval% AND 7) = 2 THEN PRINT "direc";
   LOCATE 22, 30: PRINT "Inside=";
   IF (attrval% AND 8) = 0 THEN PRINT "off";
   IF (attrval% AND 8) > 0 THEN PRINT "on ";
   LOCATE 23, 30: PRINT "Blast=";
   IF (attrval% AND 16) = 0 THEN PRINT "off";
   IF (attrval% AND 16) > 0 THEN PRINT "on ";
RETURN

clear.items:
   red.bullets% = 0
   green.bullets% = 0
   heat.seeking.bullets% = 0
   lightnings% = 0
   extra.lives% = 0
   cloaks% = 0
   green.potions% = 0
   purple.potions% = 0
   gold.keys% = 0
   silver.keys% = 0
   french.fries% = 0
   meats% = 0
   medical.kits% = 0
   soda.machines% = 0
   maps% = 0
   footprints% = 0
   ken.monsters% = 0
   andy.monsters% = 0
   green.monsters% = 0
   skeletons% = 0
   grim.reapers% = 0
   walking.holes% = 0
   stationary.holes% = 0
   fans% = 0
   eight.balls% = 0
   bats% = 0
   aliens% = 0
   normal.doors% = 0
   open.normal.doors% = 0
   see.through.doors% = 0
   open.see.through.doors% = 0
   gold.locked.doors% = 0
   silver.locked.doors% = 0
   walk.through.walls% = 0
   blast.walls% = 0
   curtains% = 0
   warps% = 0
RETURN

count.items:
   IF asksave% = 1 THEN GOSUB testsaveboardchanges
   GOSUB loadnewboard

   FOR z% = 0 TO 63
      FOR zz% = 0 TO 63
              
         mybuffer% = POINT(z% * 3 + 1, zz% * 3 + 1)
        
         IF mybuffer% = 75 THEN red.bullets% = red.bullets% + 1
         IF mybuffer% = 28 THEN green.bullets% = green.bullets% + 1
         IF mybuffer% = 34 THEN heat.seeking.bullets% = heat.seeking.bullets% + 1
         IF mybuffer% = 36 THEN lightnings% = lightnings% + 1
         IF mybuffer% = 32 THEN extra.lives% = extra.lives% + 1
         IF mybuffer% = 88 THEN cloaks% = cloaks% + 1
         IF mybuffer% = 91 THEN green.potions% = green.potions% + 1
         IF mybuffer% = 86 THEN purple.potions% = purple.potions% + 1
         IF mybuffer% = 37 THEN gold.keys% = gold.keys% + 1
         IF mybuffer% = 107 THEN silver.keys% = silver.keys% + 1
         IF mybuffer% = 59 THEN french.fries% = french.fries% + 1
         IF mybuffer% = 46 THEN meats% = meats% + 1
         IF mybuffer% = 60 THEN medical.kits% = medical.kits% + 1
         IF mybuffer% = 179 THEN soda.machines% = soda.machines% + 1
         IF mybuffer% = 80 THEN maps% = maps% + 1
         IF mybuffer% = 102 OR mybuffer% = 103 THEN footprints% = footprints% + 1
         IF mybuffer% = 66 THEN ken.monsters% = ken.monsters% + 1
         IF mybuffer% = 54 THEN andy.monsters% = andy.monsters% + 1
         IF mybuffer% = 38 THEN green.monsters% = green.monsters% + 1
         IF mybuffer% = 187 THEN skeletons% = skeletons% + 1
         IF mybuffer% = 109 THEN grim.reapers% = grim.reapers% + 1
         IF mybuffer% = 98 THEN walking.holes% = walking.holes% + 1
         IF mybuffer% = 73 THEN stationary.holes% = stationary.holes% + 1
         IF mybuffer% = 49 THEN fans% = fans% + 1
         IF mybuffer% = 68 THEN eight.balls% = eight.balls% + 1
         IF mybuffer% = 160 THEN bats% = bats% + 1
         IF mybuffer% = 94 THEN aliens% = aliens% + 1
         IF mybuffer% = 152 THEN normal.doors% = normal.doors% + 1
         IF mybuffer% = 157 THEN open.normal.doors% = normal.doors% + 1
         IF mybuffer% = 168 THEN see.through.doors% = see.through.doors% + 1
         IF mybuffer% = 173 THEN open.see.through.doors% = see.through.doors% + 1
         IF mybuffer% = 21 THEN gold.locked.doors% = gold.locked.doors% + 1
         IF mybuffer% = 31 THEN silver.locked.doors% = silver.locked.doors% + 1
         IF mybuffer% = 23 THEN walk.through.walls% = walk.through.walls% + 1
         IF mybuffer% = 134 THEN blast.walls% = blast.walls% + 1
         IF mybuffer% = 180 THEN curtains% = curtains% + 1
         IF mybuffer% = 123 THEN warps% = warps% + 1
      NEXT zz%
   NEXT z%
   CLS
   OUT &H3C8, 15
   OUT &H3C9, 50
   OUT &H3C9, 50
   OUT &H3C9, 50

   PRINT "           Red Bullets - "; red.bullets%
   PRINT "         Green Bullets - "; green.bullets%
   PRINT "  Heat Seeking Bullets - "; heat.seeking.bullets%
   PRINT "            Lightnings - "; lightnings%
   PRINT "           Extra Lives - "; extra.lives%
 
   DO UNTIL INKEY$ <> ""
   LOOP
   CLS

   PRINT "                Cloaks - "; cloaks%
   PRINT "         Green Potions - "; green.potions%
   PRINT "        Purple Potions - "; purple.potions%
   PRINT "             Gold Keys - "; gold.keys%
   PRINT "           Silver Keys - "; silver.keys%
   PRINT "          French Fries - "; french.fries%
   PRINT "                 Meats - "; meats%
   PRINT "          Medical Kits - "; medical.kits%
   PRINT "         Soda Machines - "; soda.machines%
   PRINT "                  Maps - "; maps%
   PRINT "            Footprints - "; footprints%
 
   DO UNTIL INKEY$ <> ""
   LOOP
   CLS

   PRINT "                  Bats - "; bats%
   PRINT "          Ken Monsters - "; ken.monsters%
   PRINT "             Skeletons - "; skeletons%
   PRINT "        Green Monsters - "; green.monsters%
   PRINT "         Andy Monsters - "; andy.monsters%
   PRINT "           Eight Balls - "; eight.balls%
   PRINT "                Aliens - "; aliens%
   PRINT "          Grim Reapers - "; grim.reapers%
   PRINT "         Walking Holes - "; walking.holes%
   PRINT "      Stationary Holes - "; stationary.holes%
   PRINT "                  Fans - "; fans%
 
   DO UNTIL INKEY$ <> ""
   LOOP
   CLS

   PRINT "          Normal Doors - "; normal.doors%
   PRINT "     Open Normal Doors - "; open.normal.doors%
   PRINT "     See Through Doors - "; see.through.doors%
   PRINT "Open See Through Doors - "; open.see.through.doors%
   PRINT "     Gold Locked Doors - "; gold.locked.doors%
   PRINT "   Silver Locked Doors - "; silver.locked.doors%
   PRINT "    Walk-Through Walls - "; walk.through.walls%
   PRINT "           Blast Walls - "; blast.walls%
   PRINT "              Curtains - "; curtains%
   PRINT "                 Warps - "; warps%
 
   DO UNTIL INKEY$ <> ""
   LOOP
   CLS

   regs%(0) = &H13: CALL int86old(&H10, regs%(), regs%())
   DEF SEG = &HA000
   GOSUB setupcolors
   GOSUB loadnewboard
   GOSUB printcol
   GOSUB printsprite
RETURN

updatewall:
   LOCATE 25, 1: PRINT "Name of MCGA BMP file"; : INPUT updat$
   OPEN updat$ + ".bmp" FOR BINARY AS #3
   buf$ = STRING$(3, 0)
   GET #3, , buf$
   IF MID$(buf$, 3, 1) = "6" THEN
      snot% = 1
      buf$ = STRING$(ASC(MID$(buf$, 3, 1)) - 3, 0): GET #3, , buf$
      buf$ = STRING$(1024, 0): GET #3, , buf$
      FOR z% = 0 TO 255
         buf4%(z% * 3 + 0) = ASC(MID$(buf$, z% * 4 + 3, 1)) \ 4
         buf4%(z% * 3 + 1) = ASC(MID$(buf$, z% * 4 + 2, 1)) \ 4
         buf4%(z% * 3 + 2) = ASC(MID$(buf$, z% * 4 + 1, 1)) \ 4
      NEXT z%
      OUT &H3C8, 0
      FOR z% = 0 TO 767
         OUT &H3C9, buf4%(z%)
      NEXT z%
      FOR z% = 0 TO 767 STEP 3
         c1% = buf4%(z%)
         c2% = buf4%(z% + 1)
         c3% = buf4%(z% + 2)
         bestcol% = 0
         dadist% = 1024
         FOR zz% = 0 TO 767 STEP 3
            zx% = ABS(c1% - pal%(zz%)) + ABS(c2% - pal%(zz% + 1)) + ABS(c3% - pal%(zz% + 2))
            IF zx% < dadist% THEN dadist% = zx%: bestcol% = zz% \ 3
         NEXT zz%
         palookup%(z% \ 3) = (bestcol% AND &HF0) OR (((z% \ 3) AND 15))
      NEXT z%
   ELSE
      snot% = 0
      buf$ = STRING$(23, 0): GET #3, , buf$
      buf$ = STRING$(768, 0): GET #3, , buf$
      FOR z% = 0 TO 255
         palookup%(z%) = z%
      NEXT z%
   END IF
   FOR zz& = 199 TO 0 STEP -1
      buf$ = STRING$(320, 0)
      GET #3, , buf$
      DEF SEG = VARSEG(buf$)
      offs& = SADD(buf$)
      FOR z% = 0 TO 319
         buf4%(z%) = PEEK(offs& + z%)
      NEXT z%
      DEF SEG = &HA000
      p& = zz& * 320
      FOR z& = 0 TO 319
         POKE p& + z&, buf4%(z&)
      NEXT z&
   NEXT zz&
   CLOSE #3
   x% = (updatecnt% MOD 5) * 64: y% = (updatecnt% \ 5) * 64
   xorval% = 255
   FOR z% = 0 TO 63
      PSET (x%, y% + z%), POINT(x%, y% + z%) XOR xorval%
      PSET (x% + 63, y% + z%), POINT(x% + 63, y% + z%) XOR xorval%
      PSET (x% + z%, y%), POINT(x% + z%, y%) XOR xorval%
      PSET (x% + z%, y% + 63), POINT(x% + z%, y% + 63) XOR xorval%
   NEXT z%
   DO
      z$ = INKEY$
      regs%(0) = 11: CALL int86old(&H33, regs%(), regs%())
      IF regs%(2) <> 0 AND regs%(3) <> 0 OR LEN(z$) = 2 THEN
         FOR z% = 0 TO 63
            PSET (x%, y% + z%), POINT(x%, y% + z%) XOR xorval%
            PSET (x% + 63, y% + z%), POINT(x% + 63, y% + z%) XOR xorval%
            PSET (x% + z%, y%), POINT(x% + z%, y%) XOR xorval%
            PSET (x% + z%, y% + 63), POINT(x% + z%, y% + 63) XOR xorval%
         NEXT z%
         xorval% = 255
         x% = x% + regs%(2) \ 2
         y% = y% + regs%(3) \ 2
         IF z$ = CHR$(0) + CHR$(75) THEN x% = x% - 1
         IF z$ = CHR$(0) + CHR$(77) THEN x% = x% + 1
         IF z$ = CHR$(0) + CHR$(72) THEN y% = y% - 1
         IF z$ = CHR$(0) + CHR$(80) THEN y% = y% + 1
         IF x% < 0 THEN x% = 0
         IF y% < 0 THEN y% = 0
         IF x% > 256 THEN x% = 256
         IF y% > 136 THEN y% = 136
         FOR z% = 0 TO 63
            PSET (x%, y% + z%), POINT(x%, y% + z%) XOR xorval%
            PSET (x% + 63, y% + z%), POINT(x% + 63, y% + z%) XOR xorval%
            PSET (x% + z%, y%), POINT(x% + z%, y%) XOR xorval%
            PSET (x% + z%, y% + 63), POINT(x% + z%, y% + 63) XOR xorval%
         NEXT z%
      END IF
      regs%(0) = 5: CALL int86old(&H33, regs%(), regs%())
   LOOP WHILE z$ <> CHR$(13) AND z$ <> CHR$(27) AND regs%(0) = 0
   z$ = ""
   FOR z% = 0 TO 63
      PSET (x%, y% + z%), POINT(x%, y% + z%) XOR xorval%
      PSET (x% + 63, y% + z%), POINT(x% + 63, y% + z%) XOR xorval%
      PSET (x% + z%, y%), POINT(x% + z%, y%) XOR xorval%
      PSET (x% + z%, y% + 63), POINT(x% + z%, y% + 63) XOR xorval%
   NEXT z%
   updateplcx% = x%
   updateplcy% = y%
   FOR z% = 0 TO 63
      FOR zz% = 0 TO 63
         buf%(z%, zz%) = palookup%(POINT(x% + z%, y% + zz%))
      NEXT zz%
   NEXT z%
   CLS
   GOSUB setupcolors
   attrval% = walstatus%(wallnum&): GOSUB printattrib
   GOSUB printcol
   FOR z% = 0 TO 63
      FOR zz% = 0 TO 63
         LINE (z% * 3, zz% * 3)-(z% * 3 + 2, zz% * 3 + 2), buf%(z%, zz%), BF
      NEXT zz%
   NEXT z%
   asksave% = 1
   updatecnt% = (updatecnt% + 1) MOD 15
RETURN

'Color data
DATA 0,30,63
DATA 28,34,60
DATA 0,50,20
DATA 41,52,28
DATA 63,63,25
DATA 63,63,63
DATA 63,20,20
DATA 63,0,63
DATA 63,48,27
DATA 63,40,25
DATA 63,48,48
DATA 45,63,45
DATA 55,55,63
DATA 63,40,63
DATA 63,30,20
DATA 55,25,30

'Episode screening data
DATA 0
DATA 1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2
DATA 2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1
DATA 1,1,1,3,3,3,3,3,3,1,1,2,2,1,2,1
DATA 2,2,2,2,1,1,2,3,1,2,2,1,1,3,1,3
DATA 1,3,1,1,1,1,1,3,1,0,1,1,1,1,1,1
DATA 0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
DATA 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0


