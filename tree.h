/**
 * VUT FIT Brno: PDS
 *
 * Longest-Prefix Match
 *
 * Jiri Petruzelka
 * <xpetru07>
 * 2012/2013
 */
#ifndef TREE_H
#define	TREE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <cstring>

using std::string;
using std::vector;
using std::ostream;
using std::istream;
using std::cerr;
using std::cout;
using std::endl;

double getTime();

typedef struct node {
	string content;
	string prefix;
	struct node* children[2];
	struct node* parent;
	unsigned char childrenCount;
	unsigned int as;
	bool data;
} node;

typedef struct staticNode {
	char staticPrefix[116];
	unsigned char prefixSize;

	struct staticNode* children[2];
	struct staticNode* staticParent;
	unsigned char childrenCount;
	char as[9];

	bool isData;
} staticNode;

const unsigned int allockBlock = sizeof(node) * 3;

class RadixTrie {

	public:
		static const char SEP = '|';
		static const char SEP_META = '_';
		static const char SEP_CHILD_START = '<';
		static const char SEP_CHILD_END = '>';

		RadixTrie();
		virtual ~RadixTrie();

		node* insert(const string data, const unsigned int as);
		node* insert(const string data, const unsigned int as, node* parent);
		void clear();
		void dump();
		void dumpFull();
		void dumpStaticNode(staticNode* node, unsigned int level, bool full);

		staticNode* find(const char* data);
		staticNode* findNode(staticNode* from, const char* data, const unsigned char precalcMatchedParent);
		node* getRoot();
		staticNode* getStaticRoot();
		int count();
		unsigned int nodeCount();
		int size();
		void serialize(ostream& stream, node* root, unsigned int* total);
		void parseFrom(istream& stream, const unsigned short bufferSize);

		void printAsNodes(unsigned int as, node* root);

	private:
		void clearNode(node* node);

		void dumpNode(node* node, unsigned int level, bool full);
		void addChild(node* parent, node* child);
		void addChildFast(node* parent, node* child);
		void detachChild(node* parent, node* child);

		unsigned int matchingCharacters(const string& first, const string& second);
		unsigned int matchingPrefix(const string& first, const string& second, const unsigned int start);

		void parseElement(istream& stream, staticNode* parent, const unsigned short bufferSize);

		node* root;
		staticNode* staticRoot;
		int _size;
		unsigned int _nodes;
		unsigned int _alloc;
		staticNode* allocation;

};

inline staticNode* RadixTrie::find(const char* data) {
	return findNode(this->staticRoot, data, 0);
}

inline node* RadixTrie::insert(const string data, const unsigned int as) {
	return insert(data, as, this->root);
}

inline int RadixTrie::count() {
	return this->_size;
}

inline unsigned int RadixTrie::nodeCount() {
	return this->_nodes;
}

inline int RadixTrie::size() {
	return this->_alloc * sizeof(node);
}

inline node* RadixTrie::getRoot() {
	return this->root;
}

inline staticNode* RadixTrie::getStaticRoot() {
	return this->staticRoot;
}

inline void RadixTrie::addChildFast(node* parent, node* child) {
	parent->children[parent->childrenCount] = child;
	child->parent = parent;
	parent->childrenCount++;
	this->_size++;
}

inline unsigned int RadixTrie::matchingPrefix(const string& first, const string& second, const unsigned int start) {
	unsigned int length = first.size();
	if( second.size() < length ) {
		length = second.size();
	}

	const char* str1 = first.c_str();
	const char* str2 = second.c_str();

	unsigned int i;
	for(i = start; i < length; ++i) {
		if( str1[i] != str2[i] ) {
			return i;
		}
	}

	return length;
}

inline staticNode* RadixTrie::findNode(staticNode* from, const char* data, const unsigned char precalcMatchedParent) {
	staticNode* node = NULL;

	// compare
	for(unsigned char i = 0; i < from->childrenCount; ++i)
		node = from->children[i];

		if( data[precalcMatchedParent] == node->staticPrefix[0] ) {
			unsigned int matchedChild = precalcMatchedParent + 1;

			if( node->prefixSize > 1 ) {
				unsigned int m = 1;
				for(; m < node->prefixSize; ++matchedChild) {
					if( data[matchedChild] != node->staticPrefix[m++] ) {
						matchedChild--;
						break;
					}
				}
			}

			const bool match = matchedChild - precalcMatchedParent >= node->prefixSize;

			if( !match ) {
				return NULL;
			}

			if( node->childrenCount > 0 ) {
				staticNode* found = findNode(node, data, matchedChild);
				if( found != NULL && found->isData ) {
					return found;
				} else {
					return node->isData ? node : NULL;
				}
			} else {
				return node;
			}

			break;
		}
	}

	return from;
}

#endif	/* TREE_H */
