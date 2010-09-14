# About

Vertex is a high performance graph database that supports automatic garbage collection, built on libevent and tokyocabinet. It uses HTTP as it's communication protocol and JSON as it's response data format. It's BSD licensed and was written by Steve Dekorte and Rich Collins. 
See docs/manual.html for API and more details.

# Status

Currently in production use and working well so far. The API is still not set in stone though. 

# Usage
Compiling on Mac OS X

    sudo port install yajl tokyocabinet libevent libzip
    # or if you are using homebrew
    brew install yajl tokyo-cabinet libevent
    cmake .
    make
    sudo make install

If all was fine, in build/vertexdb will be bin file

Running

    vertexdb -db blog1.db
    
or as daemon

    sudo vertexdb -db /opt/database -d --pid /var/run/vertex.pid

Possible options:

    --database <file> -db <file> Database file
    --port <num>      -p <num>   TCP port number to listen on (default: 8080)
    --host <ip>       -H <ip>    Network interface to listen (default: 127.0.0.1)
    --daemon          -d         Run as a daemon
    --log <file>                 Log file location
    --pid <file>                 Pid file location
    --debug                      Be more verbose
    --hardsync                   Run with hard syncronization
    --help            -h         Show this help

# Future

May add support for more complex queries and dynamic/automatic index creation based on observed queries.

# Dynamic languages support

* ruby - comming soon (or ask in pm)