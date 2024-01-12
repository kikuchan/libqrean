#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cp932.h"

#define N 256

typedef struct TrieNode TrieNode;
typedef uint16_t data_t;

struct TrieNode {
	int n;
	uint8_t charlist[N];

	TrieNode *children[N];
	data_t data;
};

TrieNode *make_trienode()
{
	TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
	node->data = -1;
	node->n = 0;
	return node;
}

TrieNode *insert_trie(TrieNode *root, uint8_t *word, data_t data)
{
	TrieNode *temp = root;

	int idx;
	for (int i = 0; word[i] != '\0'; i++) {
		uint8_t *p = (uint8_t *)strchr((char *)temp->charlist, word[i]);
		if (!p) {
			idx = temp->n++;
			temp->charlist[idx] = word[i];
			temp->children[idx] = make_trienode();
		} else {
			idx = p - temp->charlist;
		}
		temp = temp->children[idx];
	}

	temp->data = data;
	return root;
}

size_t write_8(uint8_t *buf, int val)
{
	*buf = val;
	return 1;
}

size_t write_value(uint8_t *buf, int val)
{
	buf[0] = (val >> 0) & 0xff;
	buf[1] = (val >> 8) & 0xff;
	return 2;
}

int dump_trie(TrieNode *root, uint8_t *buf)
{
	TrieNode *temp = root;

	int pos = 0;
	pos += write_8(buf + pos, temp->n);
	for (int i = 0; i < temp->n; i++) {
		pos += write_8(buf + pos, temp->charlist[i]);
	}

	int flag_size = (temp->n + 7) / 8;
	for (int i = 0; i < flag_size; i++) {
		uint8_t v = 0;
		for (int j = 0; j < 8 && i * 8 + j < temp->n; j++) {
			if (temp->children[i * 8 + j]->n == 0) v |= (0x80 >> j); // leaf
		}
		pos += write_8(buf + pos, v);
	}

	uint8_t *ebuf = buf + pos + temp->n * 2;
	size_t children_pos = 0;
	for (int i = 0; i < temp->n; i++) {
		if (temp->children[i]->n == 0) {
			// leaf
			pos += write_value(buf + pos, temp->children[i]->data);
		} else {
			pos += write_value(buf + pos, children_pos);
			children_pos += dump_trie(temp->children[i], ebuf + children_pos);
		}
	}
	pos += children_pos;

	return pos;
}

uint16_t read_16le(uint8_t *buf)
{
	return buf[0] | (buf[1] << 8);
}

static int search_trie(uint8_t *root, const char *word, uint16_t *data)
{
	uint8_t *temp = root;
	for (int i = 0; word[i]; i++) {
		int n = *temp++;
		uint8_t *p = (uint8_t *)memchr((char *)temp, word[i], n);
		if (!p) return -1;

		int idx = (p - temp);
		temp += n;

		int is_leaf = temp[idx / 8] & (0x80 >> (idx % 8));
		temp += (n + 7) / 8;

		if (is_leaf) {
			if (data) *data = read_16le(temp + idx * 2);
			return i + 1;
		}

		temp += n * 2 + read_16le(temp + idx * 2);
	}
	return -1;
}

const uint8_t *unicode_to_utf8(uint16_t code, uint8_t *dst)
{
	if (code < 0x80) {
		dst[0] = code;
		dst[1] = 0;
	} else if (code < 0x800) {
		dst[0] = 0xc0 | (code >> 6);
		dst[1] = 0x80 | (code & 0x3f);
		dst[2] = 0;
	} else {
		dst[0] = 0xe0 | (code >> 12);
		dst[1] = 0x80 | ((code >> 6) & 0x3f);
		dst[2] = 0x80 | (code & 0x3f);
		dst[3] = 0;
	}
	return dst;
}

int main()
{
	uint8_t buf[5] = {};
	TrieNode *root = make_trienode();

	for (int i = 0; i < sizeof(cp932_to_unicode_tbl) / sizeof(cp932_to_unicode_tbl[0]) && i < 0x1fff; i++) {
		unicode_to_utf8(cp932_to_unicode_tbl[i], buf);
		insert_trie(root, buf, i);
	}

	uint8_t *trie = malloc(64 * 1024);
	int len = dump_trie(root, trie);

#if 0
	char *src = "日本語の文章";

	for (; src; ) {
		data_t data;
		int len = search_trie(trie, src, &data);
		if (len <= 0) break;

		fprintf(stderr, "idx: %d (len %d)\n", data, len);
		src += len;
	}
#endif

	printf("#include <stdint.h>\n\n");
	printf("// clang-format off\n");
	printf("static uint8_t utf8_to_qrkanji_trie[] = {");
	for (int i = 0; i < len; i++) {
		if (i % 16 == 0) printf("\n   ");
		printf(" 0x%02x,", trie[i]);
	}
	printf("\n};\n");
	printf("// clang-format on\n");
}
