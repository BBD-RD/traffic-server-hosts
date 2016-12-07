#ATSSRCDIR=../../traffic-server
ATSSRCDIR=../../git/liliang/white-shark/traffic-server/

TSXS=${ATSSRCDIR}/tools/tsxs
FLAGS=-I ${ATSSRCDIR}/lib  -I ${ATSSRCDIR}/proxy/api
LIBS=#-L ${ATSSRCDIR}/lib/tsconfig/ -ltsconfig
OUTFILE=ws_hosts.so
FILE_EX=cc

all:
	chmod +x ${TSXS} && ${TSXS} ${FLAGS} ${LIBS} -o ${OUTFILE} -c *.$(FILE_EX)

install:
	chmod +x ${TSXS} && ${TSXS} -o ${OUTFILE} -i

clean:
	rm -rf *.lo *.so
