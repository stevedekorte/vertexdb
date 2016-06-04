FROM 		ruby:latest

ENV		OPTS ""
RUN		apt-get update && apt-get install --force-yes -y cmake 		\
							git			\
							apt-utils 		\
							libtokyocabinet-dev 	\
							zip			\
							libyajl-dev		\
							libyajl2		\
							libevent-dev

RUN		gem update  --no-rdoc --no-ri && gem install  --no-rdoc --no-ri \
							yajl			\
							yajl-ruby		\
							zipruby			\
							libevent


RUN		git clone https://github.com/stevedekorte/vertexdb.git vertexdb

WORKDIR		vertexdb

RUN		cmake .		&&\
		make		&&\
		make install

EXPOSE		8080

ENTRYPOINT	["/usr/local/bin/vertexdb"]
CMD		["-db", "/opt/database", "--pid", "/var/run/vertex.pid", "-H", "0.0.0.0", "-p", "80", "$OPTS"]
