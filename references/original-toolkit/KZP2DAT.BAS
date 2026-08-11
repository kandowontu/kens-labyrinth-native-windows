SELECT CASE UCASE$(COMMAND$)
   CASE "WALLS", "WALLS.KZP": z$ = "1"
   CASE "BOARDS", "BOARDS.KZP": z$ = "2"
   CASE "STORY", "STORY.KZP": z$ = "3"
   CASE "SONGS", "SONGS.KZP": z$ = "4"
   CASE "SOUNDS", "SOUNDS.KZP": z$ = "5"
   CASE "LAB3D", "LAB3D.KZP": z$ = "6"
   CASE ELSE
      PRINT "Press 1 to convert  WALLS.KZP ->  WALLS.DAT"
      PRINT "Press 2 to convert BOARDS.KZP -> BOARDS.DAT"
      PRINT "Press 3 to convert  STORY.KZP ->  STORY.DAT"
      PRINT "Press 4 to convert  SONGS.KZP ->      *.KSM"
      PRINT "Press 5 to convert SOUNDS.KZP ->      *.WAV"
      PRINT "Press 6 to convert  LAB3D.KZP ->      *.GIF"
      PRINT "Press ESC to cancel"
      DO
         z$ = INKEY$
         IF z$ = CHR$(27) THEN END
      LOOP WHILE z$ <> "1" AND z$ <> "2" AND z$ <> "3" AND z$ <> "4" AND z$ <> "5" AND z$ <> "6"
      PRINT
END SELECT

DIM pow2%(13), wallen%(512), tempstack%(4100), suffix%(4100), prefix%(4100)
FOR i% = 0 TO 12: pow2%(i%) = 2 ^ i%: NEXT

IF z$ = "1" OR z$ = "2" THEN
   IF z$ = "1" THEN
      OPEN "walls.kzp" FOR BINARY AS #1
      OPEN "walls.dat" FOR BINARY AS #2
      PRINT "Generating WALLS.DAT...";
      numitems% = 448
      header$ = STRING$(numitems%, 0): GET #1, , header$
      PUT #2, , header$: zx$ = STRING$(1024 - LEN(header$), 0): PUT #2, , zx$
   ELSE
      OPEN "boards.kzp" FOR BINARY AS #1
      OPEN "boards.dat" FOR BINARY AS #2
      PRINT "Generating BOARDS.DAT...";
      numitems% = 30 * 2
   END IF
   z$ = STRING$(numitems% * 2, 0): GET #1, , z$
   FOR z% = 0 TO numitems% - 1: wallen%(z%) = CVI(MID$(z$, z% * 2 + 1, 2)): NEXT
   FOR z% = 0 TO 255: suffix%(z%) = z%: prefix%(z%) = z%: NEXT
   pic$ = STRING$(4096, 0)
   FOR zxz% = 0 TO numitems% - 1
      p% = 0: bitcnt% = 0: currstr% = 256: numbits% = 9
      GET #1, , strtot%: compleng% = wallen%(zxz%)
      filbuffer$ = STRING$(compleng%, 0): GET #1, , filbuffer$
      filbuffer$ = filbuffer$ + "   ": bytecnt% = 1
      DO
         zx& = ASC(MID$(filbuffer$, bytecnt%, 1)) + 256& * ASC(MID$(filbuffer$, bytecnt% + 1, 1))
         dat% = (zx& \ pow2%(bitcnt%)) AND (pow2%(numbits%) - 1)
         IF bitcnt% + numbits% > 16 THEN
            zx% = ASC(MID$(filbuffer$, bytecnt% + 2, 1))
            dat% = dat% + (zx% AND (pow2%((bitcnt% + numbits%) AND 7) - 1)) * pow2%(16 - bitcnt%)
         END IF
         bitcnt% = bitcnt% + numbits%: bytecnt% = bytecnt% + bitcnt% \ 8
         bitcnt% = bitcnt% AND 7: prefix%(currstr%) = dat%
         leng% = 0
         DO WHILE dat% >= 256 AND leng% < 4100
            tempstack%(leng%) = suffix%(dat%): leng% = leng% + 1
            dat% = prefix%(dat%)
         LOOP
         tempstack%(leng%) = prefix%(dat%): leng% = leng% + 1
         FOR z% = leng% - 1 TO 0 STEP -1
            IF p% < 4096 THEN MID$(pic$, p% + 1, 1) = CHR$(tempstack%(z%))
            p% = p% + 1
         NEXT z%
         suffix%(currstr% - 1) = dat%: suffix%(currstr%) = dat%
         currstr% = currstr% + 1
         IF currstr% >= pow2%(numbits%) THEN numbits% = numbits% + 1
      LOOP WHILE currstr% <= strtot%
      PUT #2, , pic$
   NEXT zxz%
   CLOSE #2: CLOSE #1
ELSEIF z$ = "3" THEN
   DIM storyoffs%(128)
   OPEN "story.kzp" FOR BINARY AS #1
   OPEN "story.dat" FOR BINARY AS #2
   PRINT "Generating STORY.DAT...";
   z$ = STRING$(256, 0): GET #1, , z$
   FOR i% = 0 TO 127: storyoffs%(i%) = CVI(MID$(z$, i% * 2 + 1, 2)): NEXT
   last% = 0
   FOR zxz% = 0 TO 127
      IF storyoffs%(zxz% + 1) = 0 THEN
         last% = 1: leng% = LOF(1) - storyoffs%(zxz%)
      ELSE
         leng% = storyoffs%(zxz% + 1) - storyoffs%(zxz%)
      END IF
      z$ = STRING$(leng%, 0): GET #1, storyoffs%(zxz%) + 1, z$
      dat% = 0
      FOR i% = 1 TO leng% - 1
         odat% = dat%: dat% = ASC(MID$(z$, i%, 1))
         j% = odat% XOR dat%
         zx$ = CHR$(j%): PUT #2, , zx$
         IF j% = 13 THEN zx$ = CHR$(10): PUT #2, , zx$
      NEXT
      IF last% THEN EXIT FOR
      zx$ = "\" + CHR$(13) + CHR$(10): PUT #2, , zx$
   NEXT
   CLOSE #2: CLOSE #1
ELSEIF z$ = "4" THEN
   DIM songname$(64), offs&(64)
   OPEN "songs.kzp" FOR BINARY AS #1
   GET #1, , numsongs%
   FOR z% = 0 TO numsongs% - 1
      z$ = STRING$(12, 0): GET #1, , z$
      zz% = 1: songname$(z%) = ""
      DO WHILE MID$(z$, zz%, 1) <> CHR$(0) AND zz% <= 8
         songname$(z%) = songname$(z%) + MID$(z$, zz%, 1)
         zz% = zz% + 1
      LOOP
      offs&(z%) = CVL(RIGHT$(z$, 4))
   NEXT z%
   FOR z% = 0 TO numsongs% - 1
      OPEN songname$(z%) + ".KSM" FOR BINARY AS #2
      PRINT "Generating "; songname$(z%) + ".KSM"
      IF z% = numsongs% - 1 THEN
         leng& = LOF(1) - offs&(z%)
      ELSE
         leng& = offs&(z% + 1) - offs&(z%)
      END IF
      z$ = STRING$(leng&, 0)
      GET #1, , z$
      PUT #2, , z$
      CLOSE #2
   NEXT z%
   CLOSE #2: CLOSE #1
ELSEIF z$ = "5" THEN
   DIM offset&(50), leng%(50)
   OPEN "sounds.kzp" FOR BINARY AS #1
   GET #1, , num%
   FOR z% = 0 TO num% - 1
      GET #1, , offset&(z%)
      GET #1, , leng%(z%)
   NEXT z%
   FOR z% = 0 TO num% - 1
      SEEK #1, offset&(z%) + 1

      READ filename$: filename$ = UCASE$(filename$ + ".wav")
      OPEN filename$ FOR BINARY AS #2
      PRINT "Generating "; filename$
    
      z$ = "RIFF" + MKL$(leng%(z%) + 36) + "WAVEfmt "
      z$ = z$ + MKL$(16) + MKI$(1) + MKI$(1) + MKL$(11025) + MKL$(11025)
      z$ = z$ + MKI$(1) + MKI$(8) + "data" + MKL$(leng%(z%))
      PUT #2, , z$
       
      snd$ = STRING$(leng%(z%) \ 2, 0)
      GET #1, , snd$: PUT #2, , snd$
      snd$ = STRING$(leng%(z%) - leng%(z%) \ 2, 0)
      GET #1, , snd$: PUT #2, , snd$
      CLOSE #2
   NEXT z%
   CLOSE #1

   DATA "batdie","blowup","bounce","bullseye","closdoor","death","fall"
   DATA "getstuff","hitenemy","hitfan","killcube","nicejob","notnow"
   DATA "opendoor","ouch","shoot","unlock","warp","welcome","cheat"
   DATA "diearth","dienow","shoot2","wingame","getsoda","clink","nothing"
   DATA "select","water","updowndr","crumble","clock","bark"
ELSEIF z$ = "6" THEN
   OPEN "lab3d.kzp" FOR BINARY AS #1
  
   z$ = STRING$(7649, 0): GET #1, , z$
   OPEN "1.gif" FOR BINARY AS #2
   MID$(z$, 1, 6) = "GIF87a": PUT #2, , z$
   CLOSE #2
  
   z$ = STRING$(8258, 0): GET #1, , z$
   OPEN "2.gif" FOR BINARY AS #2
   MID$(z$, 1, 6) = "GIF87a": PUT #2, , z$
   CLOSE #2
  
   z$ = STRING$(1033, 0): GET #1, , z$
   OPEN "3.gif" FOR BINARY AS #2
   MID$(z$, 1, 6) = "GIF87a": PUT #2, , z$
   z$ = STRING$(10000, 0)
   FOR i% = 1 TO 9: GET #1, , z$: PUT #2, , z$: NEXT
   CLOSE #2
  
   CLOSE #1
END IF
PRINT "Done.": END

