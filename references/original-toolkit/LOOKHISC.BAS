SCREEN 0: WIDTH 80, 43: CLS
numboards% = 30
OPEN "hiscore.dat" FOR BINARY AS #1
st$ = STRING$(16, 0)
FOR z% = 0 TO numboards% - 1
   IF (z% MOD 10) <> 9 THEN
      zz% = 0
      LOCATE ((z% MOD 10) \ 3) * 10 + zz% + 1, ((z% MOD 10) MOD 3) * 26 + 1
      PRINT "ÚÄÄÄÄÄÄÄ Level ÄÄÄÄÄÄÄÄÄÄ¿";
      LOCATE ((z% MOD 10) \ 3) * 10 + zz% + 10, ((z% MOD 10) MOD 3) * 26 + 1
      PRINT "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ";
      LOCATE ((z% MOD 10) \ 3) * 10 + zz% + 1, ((z% MOD 10) MOD 3) * 26 + 15
      PRINT z% + 1;
      PRINT
      FOR zz% = 1 TO 8
         GET #1, , st$
         LOCATE ((z% MOD 10) \ 3) * 10 + zz% + 1, ((z% MOD 10) MOD 3) * 26 + 1
         PRINT "³"; RTRIM$(LTRIM$(STR$(zz%))); ". "; LEFT$(st$, 12); " "; CVL(RIGHT$(st$, 4));
         LOCATE ((z% MOD 10) \ 3) * 10 + zz% + 1, ((z% MOD 10) MOD 3) * 26 + 26
         PRINT "³";
      NEXT zz%
   ELSE
      FOR zz% = 1 TO 8
         GET #1, , st$
      NEXT zz%
      DO: LOOP WHILE INKEY$ = ""
      CLS
   END IF
NEXT z%
CLOSE #1

