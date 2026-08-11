CLS
OPEN "savgame2.dat" FOR BINARY AS #1
z$ = STRING$(16, 1): GET #1, , z$: PRINT "hiscorenam: "; z$
z$ = STRING$(1, 1): GET #1, , z$: PRINT "hiscorenamstat: "; ASC(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "boardnum: "; CVI(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "scorecount: "; CVL(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "scoreclock: "; CVL(z$)
z$ = STRING$(8192, 1): GET #1, , z$
PRINT "board: "
FOR z% = 1 TO 64
	FOR zz% = 1 TO 64
		PRINT MID$(z$, zz% * 128 + z% * 2 + 2, 1);
	NEXT zz%
	PRINT
	IF (z% AND 15) = 15 THEN zx$ = INPUT$(1)
NEXT z%
z$ = STRING$(2, 1): GET #1, , z$: PRINT "skilevel: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "life: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "death: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "lifevests: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "lightnings: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "firepowers(0): "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "firepowers(1): "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "firepowers(2): "; CVI(z$)
z$ = INPUT$(1)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "bulchoose: "; CVI(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "keys: "; CVL(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "coins: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "compass: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "cheated: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate2: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate3: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate4: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "oscillate3: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "oscillate5: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate6: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate7: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate8: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate10: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate11: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "animate15: "; CVI(z$)
z$ = INPUT$(1)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "statusbar: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "statusbargoal: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "posx: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "posy: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "posz: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "ang: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "startx: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "starty: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "startang: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "angvel: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "vel: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "mxvel: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "myvel: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "svel: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "hvel: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "oldposx: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "oldposy: "; CVI(z$)
z$ = INPUT$(1)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "bulnum: "; CVI(z$): bulnum% = CVI(z$)
z$ = STRING$(bulnum% * 2, 1): GET #1, , z$: PRINT "bulang: ";
FOR z% = 1 TO bulnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(bulnum% * 2, 1): GET #1, , z$: PRINT "bulang: ";
FOR z% = 1 TO bulnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(bulnum% * 2, 1): GET #1, , z$: PRINT "bulx: ";
FOR z% = 1 TO bulnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(bulnum% * 2, 1): GET #1, , z$: PRINT "buly: ";
FOR z% = 1 TO bulnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(bulnum% * 4, 1): GET #1, , z$: PRINT "bulstat: ";
FOR z% = 1 TO bulnum%: PRINT CVL(MID$(z$, z% * 4 - 3, 4)); : NEXT z%: PRINT
z$ = STRING$(4, 1): GET #1, , z$: PRINT "lastbulshoot: "; CVL(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "mnum: "; CVI(z$): mnum% = CVI(z$)
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "mposx: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "mposy: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "mgolx: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "mgoly: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "moldx: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "moldy: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "mstat: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum% * 2, 1): GET #1, , z$: PRINT "mshock: ";
FOR z% = 1 TO mnum%: PRINT CVI(MID$(z$, z% * 2 - 1, 2)); : NEXT z%: PRINT
z$ = STRING$(mnum%, 1): GET #1, , z$: PRINT "mshot: ";
FOR z% = 1 TO mnum%: PRINT ASC(MID$(z$, z%, 1)); : NEXT z%: PRINT
z$ = INPUT$(1)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "doorx: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "doory: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "doorstat: "; CVI(z$)
z$ = STRING$(1, 1): GET #1, , z$: PRINT "numwarps: "; ASC(z$): numwarps% = ASC(z$)
z$ = STRING$(1, 1): GET #1, , z$: PRINT "justwarped: "; ASC(z$)
z$ = STRING$(numwarps%, 1): GET #1, , z$: PRINT "xwarp: ";
FOR z% = 1 TO numwarps%: PRINT ASC(MID$(z$, z%, 1)); : NEXT z%: PRINT
z$ = STRING$(numwarps%, 1): GET #1, , z$: PRINT "ywarp: ";
FOR z% = 1 TO numwarps%: PRINT ASC(MID$(z$, z%, 1)); : NEXT z%: PRINT
z$ = STRING$(4, 1): GET #1, , z$: PRINT "totalclock: "; CVL(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "purpletime: "; CVL(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "greentime: "; CVL(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "capetime(0): "; CVI(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "capetime(1): "; CVI(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "musicstatus: "; CVL(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "clockspeed: "; CVI(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "count: "; CVL(z$)
z$ = STRING$(4, 1): GET #1, , z$: PRINT "countstop: "; CVL(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "nownote: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "junk: "; CVI(z$)
z$ = INPUT$(1)
z$ = STRING$(18 * 4, 1): GET #1, , z$: PRINT "chanage: ";
FOR z% = 1 TO 18: PRINT CVL(MID$(z$, z% * 4 - 3, 4)); : NEXT z%: PRINT
z$ = STRING$(18, 1): GET #1, , z$: PRINT "chanfreq: ";
FOR z% = 1 TO 18: PRINT ASC(MID$(z$, z%, 1)); : NEXT z%: PRINT
z$ = STRING$(2, 1): GET #1, , z$: PRINT "midiinst: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "mute: "; CVI(z$)
z$ = STRING$(1, 1): GET #1, , z$: PRINT "namrememberstat: "; ASC(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "fadewarpval: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "fadehurtval: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "slottime: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "slotpos(0): "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "slotpos(1): "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "slotpos(2): "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "owecoins: "; CVI(z$)
z$ = STRING$(2, 1): GET #1, , z$: PRINT "owecoinwait: "; CVI(z$)
CLOSE #1

