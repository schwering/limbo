if [ ! -f "textinterface/jquery-3.1.1.min.js" ]
then
        wget -O "textinterface/jquery-3.1.1.min.js" "https://code.jquery.com/jquery-3.1.1.min.js"
fi
if [ ! -f "minesweeper/jquery-3.1.1.min.js" ]
then
        wget -O "minesweeper/jquery-3.1.1.min.js" "https://code.jquery.com/jquery-3.1.1.min.js"
fi
scp minesweeper/mw-js* textinterface/jquery-*.min.js unsw:public_html/demo/minesweeper/
scp textinterface/ti-js* textinterface/example*.txt textinterface/test*.txt textinterface/jquery-*.min.js unsw:public_html/demo/textinterface/

