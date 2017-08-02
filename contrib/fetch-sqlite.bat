set SQLITE_NAME=sqlite-amalgamation-3200000
set SQLITE_URL=https://www.sqlite.org/2017/%SQLITE_NAME%.zip
wget %SQLITE_URL%

7z -y x %SQLITE_NAME%.zip
del %SQLITE_NAME%.zip
rename %SQLITE_NAME% sqlite-win32