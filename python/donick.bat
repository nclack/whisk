for /L %%F in (0,750,5000) DO (
  set /A MAXFRAME=%%F+749
  start "render" /low "cmd /c python render.py C:\Users\clackn\Desktop\ForPaper\nick\test.mp4 C:\Users\clackn\Desktop\ForPaper\nick\render\%%05d.png -minframe=%%F -maxframe=%MAXFRAME%"
  )
