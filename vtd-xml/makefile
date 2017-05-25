CC=gcc
 
CFLAGS= -c -O3 -Wall -Winline -fgnu89-inline -fomit-frame-pointer -fforce-addr -frerun-cse-after-loop -fexpensive-optimizations -fregmove -frerun-loop-opt -fmerge-all-constants -fno-branch-count-reg -funroll-loops -fpeephole -march=core2 -falign-functions -falign-loops -falign-jumps -freorder-blocks -freorder-functions -fprefetch-loop-arrays -funswitch-loops -fbranch-target-load-optimize2 -fvpt --param inline-unit-growth=300 --param max-inline-recursive-depth=2 --param large-function-growth=600

CFLAGS2 = -c -ggdb
LDFLAGS = -O3 -fomit-frame-pointer -fforce-addr -frerun-cse-after-loop -fexpensive-optimizations -fregmove -frerun-loop-opt -march=core2 -lm 
LDFLAGS2  = -ggdb

#all :  vtd-xml RSSReader stats soap benchmark_vtdxml update indexWriter indexReader testNodeRecorder testOverwrite testBookMark  FragmentTest ModifyXML
all :  vtd-xml RSSReader stats soap benchmark_vtdxml update indexWriter indexReader testNodeRecorder testOverwrite testBookMark  FragmentTest 

benchmark_vtdxml: benchmark_vtdxml.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o -lm

FragmentTest: FragmentTest.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o  -lm

#ModifyXML: ModifyXML.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o

erase: erase.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o -lm

soap: soap.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o -lm

stats: stats.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o -lm

RSSReader: RSSReader.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o -lm

update: update.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o variableExpr.o cachedExpr.o -lm

indexWriter: indexWriter.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o  variableExpr.o cachedExpr.o -lm

indexReader: indexReader.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o  variableExpr.o cachedExpr.o -lm
	
testNodeRecorder: testNodeRecorder.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o  textIter.o  variableExpr.o cachedExpr.o -lm

testOverwrite: testOverwrite.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o  variableExpr.o cachedExpr.o -lm

testBookMark: testBookMark.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o  variableExpr.o cachedExpr.o -lm

vtd-xml: vtd-xml.o arrayList.o fastIntBuffer.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o  variableExpr.o cachedExpr.o -lm


	${CC} ${LDFLAGS} -o vtd-xml vtd-xml.o fastIntBuffer.o arrayList.o fastLongBuffer.o contextBuffer.o vtdNav.o vtdGen.o autoPilot.o XMLChar.o helper.o lex.yy.o l8.tab.o literalExpr.o numberExpr.o pathExpr.o filterExpr.o binaryExpr.o unaryExpr.o funcExpr.o locationPathExpr.o intHash.o unionExpr.o decoder.o XMLModifier.o nodeRecorder.o indexHandler.o bookMark.o elementFragmentNs.o transcoder.o textIter.o  variableExpr.o cachedExpr.o -lm

clean:
	-rm *.o soap RSSReader vtd-xml stats

benchmark_vtdxml.o: example/benchmark_vtdxml.c
	${CC} ${CFLAGS} example/benchmark_vtdxml.c

erase.o : example/erase.c
	${CC} ${CFLAGS} example/erase.c

FragmentTest.o: example/FragmentTest.c
	${CC} ${CFLAGS} example/FragmentTest.c

#ModifyXML.o: example/ModifyXML.c
#	${CC} ${CFLAGS} example/ModifyXML.c

vtd-xml.o: example/vtd-xml.c
	${CC} ${CFLAGS} example/vtd-xml.c

RSSReader.o : example/RSSReader.c
	${CC} ${CFLAGS} example/RSSReader.c

update.o : example/update.c
	${CC} ${CFLAGS} example/update.c

stats.o : example/stats.c
	${CC} ${CFLAGS} example/stats.c

soap.o	: example/soap.c
	${CC} ${CFLAGS} example/soap.c

indexWriter.o	: example/indexWriter.c 
	${CC} ${CFLAGS} example/indexWriter.c

indexReader.o	: example/indexReader.c 
	${CC} ${CFLAGS} example/indexReader.c

testNodeRecorder.o	: example/testNodeRecorder.c
	${CC} ${CFLAGS} example/testNodeRecorder.c

testOverwrite.o	: example/testOverwrite.c
	${CC} ${CFLAGS} example/testOverwrite.c

testBookMark.o 	: example/testBookMark.c
	${CC} ${CFLAGS} example/testBookMark.c

elementFragmentNs.o  : elementFragmentNs.c
	${CC} ${CFLAGS} elementFragmentNs.c

bookMark.o : bookMark.c
	${CC} ${CFLAGS} bookMark.c

nodeRecorder.o	: nodeRecorder.c
	${CC} ${CFLAGS} nodeRecorder.c	

indexHandler.o	: indexHandler.c
	${CC} ${CFLAGS} indexHandler.c
	
cachedExpr.o	: cachedExpr.c
	${CC} ${CFLAGS} cachedExpr.c

decoder.o : decoder.c
	${CC} ${CFLAGS} decoder.c

intHash.o : intHash.c
	${CC} ${CFLAGS} intHash.c

arrayList.o : arrayList.c
	${CC} ${CFLAGS} arrayList.c
	
fastIntBuffer.o : fastIntBuffer.c
	${CC} ${CFLAGS} fastIntBuffer.c

fastLongBuffer.o : fastLongBuffer.c
	${CC} ${CFLAGS} fastLongBuffer.c

contextBuffer.o : contextBuffer.c
	${CC} ${CFLAGS} contextBuffer.c    

vtdGen.o : vtdGen.c
	${CC} ${CFLAGS} vtdGen.c
	
vtdNav.o : vtdNav.c
	${CC} ${CFLAGS} vtdNav.c

autoPilot.o : autoPilot.c
	${CC} ${CFLAGS} autoPilot.c

XMLModifier.o : XMLModifier.c
	${CC} ${CFLAGS} XMLModifier.c
	

XMLChar.o : XMLChar.c
	${CC} ${CFLAGS} XMLChar.c	

unionExpr.o : unionExpr.c
	${CC} ${CFLAGS} unionExpr.c

filterExpr.o : filterExpr.c
	${CC} ${CFLAGS} filterExpr.c

literalExpr.o : literalExpr.c
	${CC} ${CFLAGS} literalExpr.c

numberExpr.o : numberExpr.c
	${CC} ${CFLAGS} numberExpr.c

unaryExpr.o : unaryExpr.c
	${CC} ${CFLAGS} unaryExpr.c

locationPathExpr.o : locationPathExpr.c
	${CC} ${CFLAGS} locationPathExpr.c


pathExpr.o : pathExpr.c
	${CC} ${CFLAGS} pathExpr.c

lex.yy.o : lex.yy.c
	${CC} ${CFLAGS} lex.yy.c

l8.tab.o : l8.tab.c
	${CC} ${CFLAGS} l8.tab.c

funcExpr.o : funcExpr.c
	${CC} ${CFLAGS} funcExpr.c

variableExpr.o : variableExpr.c
	${CC} ${CFLAGS} variableExpr.c


helper.o : helper.c
	${CC} ${CFLAGS} helper.c

transcoder.o : transcoder.c
	${CC} ${CFLAGS} transcoder.c

textIter.o  : textIter.c
	${CC} ${CFLAGS} textIter.c
