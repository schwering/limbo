if [ ! -f "textinterface/jquery-3.1.1.min.js" ]
then
        wget -O "textinterface/jquery-3.1.1.min.js" http://code.google.com/p/jqueryjs/downloads/detail?name=jquery-1.3.1.min.js
fi
scp minesweeper/index.html minesweeper/lela.js* unsw:public_html/demo/minesweeper/
scp textinterface/index.html textinterface/lela.js* textinterface/example-*.txt textinterface/jquery-*.min.js unsw:public_html/demo/textinterface/

