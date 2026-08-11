@echo off
if not exist walls.kzp goto nowalls
kzp2dat walls
if not exist boards.kzp goto noboards
kzp2dat boards
if not exist story.kzp goto nostory
kzp2dat story
if not exist songs.kzp goto nosongs
kzp2dat songs
if not exist sounds.kzp goto nosounds
kzp2dat sounds
if not exist lab3d.kzp goto nolab3d
kzp2dat lab3d
goto end

:nowalls
echo WALLS.KZP not found. Please copy it from the Ken's Labyrinth game directory.
goto end

:noboards
echo BOARDS.KZP not found. Please copy it from the Ken's Labyrinth game directory.
goto end

:nostory
echo STORY.KZP not found. Please copy it from the Ken's Labyrinth game directory.
goto end

:nosongs
echo SONGS.KZP not found. Please copy it from the Ken's Labyrinth game directory.
goto end

:nosounds
echo SOUNDS.KZP not found. Please copy it from the Ken's Labyrinth game directory.
goto end

:nolab3d
echo LAB3D.KZP not found. Please copy it from the Ken's Labyrinth game directory.
goto end

:end
