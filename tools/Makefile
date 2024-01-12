all: qrkanji-table.h

cp932.h: kanji.c
	cc kanji.c -o kanji
	./kanji > cp932.h

qrkanji-table.h: trie.c cp932.h
	cc trie.c -o trie
	./trie | cat cp932.h - > qrkanji-table.h

clean:
	rm trie kanji qrkanji-table.h cp932.h

update: qrkanji-table.h
	cp qrkanji-table.h ../src
