# Include SDL 1.2.8 include files and Windows system headers.
CFLAGS=/c /MD /Ic:\SDL-1.2.8\include /I"c:\Program Files\Microsoft Platform SDK for Windows XP SP2\Include" /O2 /Gy /nologo /wd4996

# Link to SDL 1.2.8 library and Windows system library.
labsdl.exe: adlibemu.obj graphx.obj init.obj lab3d.obj oldlab3d.obj opengl.obj setup.obj software.obj subs.obj; link /out:ken.exe adlibemu graphx init lab3d oldlab3d opengl setup software subs ken.res winmm.lib kernel32.lib user32.lib glu32.lib opengl32.lib glu32.lib ole32.lib shell32.lib sdlmain.lib sdl.lib uuid.lib /opt:nowin98 /entry:mainCRTStartup /subsystem:windows /nologo /libpath:"c:\program files\microsoft platform sdk for windows xp sp2\lib" /libpath:c:\sdl-1.2.8\lib

ken.res: ken.rc; "c:\program files\microsoft platform sdk for windows xp sp2\bin\rc" /fo ken.res /r ken.rc
