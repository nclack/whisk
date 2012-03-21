for /L %%F in (0,250,4500) DO (
  set /A MAXFRAME=%%F+249
  start "render" /low "cmd /c python render.py C:\Users\clackn\Desktop\ForPaper\29\whisker_data_0029.seq C:\Users\clackn\Desktop\ForPaper\29\render\%%05d.png -minframe=%%F -maxframe=%MAXFRAME%"
  )
