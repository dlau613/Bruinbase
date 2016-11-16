
SRC = main.cc SqlParser.tab.c lex.sql.c SqlEngine.cc BTreeIndex.cc BTreeNode.cc RecordFile.cc PageFile.cc 
HDR = Bruinbase.h PageFile.h SqlEngine.h BTreeIndex.h BTreeNode.h RecordFile.h SqlParser.tab.h

SRC_TEST = btreeTests.cc SqlParser.tab.c lex.sql.c SqlEngine.cc BTreeIndex.cc BTreeNode.cc RecordFile.cc PageFile.cc 
HDR_TEST = Bruinbase.h PageFile.h SqlEngine.h BTreeIndex.h BTreeNode.h RecordFile.h SqlParser.tab.h catch.hpp
bruinbase: $(SRC) $(HDR)
	g++ -ggdb -o $@ $(SRC)

testexe: $(SRC_TEST) $(HDR_TEST)
	g++ -ggdb -o $@ $(SRC_TEST)

lex.sql.c: SqlParser.l
	flex -Psql $<

SqlParser.tab.c: SqlParser.y
	bison -d -psql $<

clean:
	rm -f bruinbase bruinbase.exe *.o *~ lex.sql.c SqlParser.tab.c SqlParser.tab.h testexe testexe.exe *.o 
