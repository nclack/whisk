for /L %%F in (0,250,3748) DO (
  set /A MAXFRAME=%%F+749
  start "render" /low "cmd /c python render.py C:\Users\clackn\Desktop\ForPaper\Leo0061\LTPJF41501_081909_C1_B_0061.seq C:\Users\clackn\Desktop\ForPaper\Leo0061\render\%%05d.png -minframe=%%F -maxframe=%MAXFRAME%"
  )
