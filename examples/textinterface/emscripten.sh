if [ ! -f "jquery-3.1.1.min.js" ]
then
        wget -O "jquery-3.1.1.min.js" "https://code.jquery.com/jquery-3.1.1.min.js"
fi
em++ -std=c++11 -I../../src -O3 -DNDEBUG ti-js.cc -o ti-js.js -s EXPORTED_FUNCTIONS="['_lela_parse']"

