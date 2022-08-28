echo #pragma once >  ../src/html.h
echo inline const char* index_html = R^"multiline^( >> ../src/html.h
START /B /WAIT cmd /c "minify index.html" >> ../src/html.h
echo )multiline^"; >> ../src/html.h
