setlocal EnableDelayedExpansion
set _SRC=C:\Users\clackn\Desktop\ForPaper\171\whisker_data_0171.seq
set _DST=C:\Users\clackn\Desktop\ForPaper\171\seeds\%%05d.png
echo %_SRC%
echo %_DST%
set /A _MAXFRAME=249
for /L %%F in (0,250,4500) DO (
  set /A _MAXFRAME=%%F+249
  start "render" /low "cmd /c python qc_seed.py -minframe=%%F -maxframe=!_MAXFRAME! -maxr 7 %_SRC% %_DST%"
  )
endlocal
