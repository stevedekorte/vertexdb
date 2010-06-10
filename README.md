# About

Vertex is a high performance graph database that supports automatic garbage collection, built on libevent and tokyocabinet. It uses HTTP as it's communication protocol and JSON as it's response data format. It's BSD licensed and was written by Steve Dekorte and Rich Collins. 
See docs/manual.html for API and more details.

# Status

Currently in production use and working well so far. The API is still not set in stone though. 

# Usage
Compiling on Mac OS X

    sudo port install yajl tokyocabinet libevent tokyocabinet libzip
    git clone http://github.com/paxa/vertexdb.git
    cd vertexdb
    ./bin/compile

If all was fine, in build/vertexdb will be bin file

Running

    ./vertexdb -db blog1.db

Possible options:

* **-db *file*** - database file
* **-port *port*** - port of http api server
* **-host *ip*** - listening host
* **-daemon** - run as daemon
* **-log *file*** - write strerr to log file
* **-debug** - be more verbose
* **-hardsync** - hard syncronization

# Future

May add support for more complex queries and dynamic/automatic index creation based on observed queries.