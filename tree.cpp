/**
 * VUT FIT Brno: PDS
 *
 * Longest-Prefix Match
 *
 * Jiri Petruzelka
 * <xpetru07>
 * 2012/2013
 */
#include "tree.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string.h>
#include <exception>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif //WIN32
#include <math.h>

#define ALLOC_BLOCK 3
#define DEBUG 1

using std::cout;
using std::endl;
using std::cerr;
using std::exception;
using std::ios_base;

#define MAX(a,b) (a > b ? a : b)
#define ROUND(x, n) (floor(MAX(0,x) * pow(10,n)) / pow(10,n))

/* zdroj: gmu1 */
double getTime(void)
{
#if _WIN32  															/* toto jede na Windows */
    static int initialized = 0;
    static LARGE_INTEGER frequency;
    LARGE_INTEGER value;

    if (!initialized) {                         							/* prvni volani */
        initialized = 1;
        if (QueryPerformanceFrequency(&frequency) == 0) {                   /* pokud hi-res pocitadlo neni podporovano */
            //assert(0 && "HiRes timer is not available.");
            exit(-1);
        }
    }

    //assert(QueryPerformanceCounter(&value) != 0 && "This should never happen.");  /* osetreni chyby */
    QueryPerformanceCounter(&value);
    return (double)value.QuadPart / (double)frequency.QuadPart;  			/* vrat hodnotu v sekundach */

#else                                         							/* toto jede na Linux/Unixovych systemech */
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {        							/* vezmi cas */
        //assert(0 && "gettimeofday does not work.");  						/* osetri chyby */
        exit(-2);
    }
    return (double)tv.tv_sec + (double)tv.tv_usec/1000000.;  				/* vrat cas v sekundach */
#endif
}
/* /zdroj: gmu1 */


RadixTrie::RadixTrie() {
	this->root = new node;
	this->root->prefix = string("");
	this->root->parent = NULL;
	this->root->childrenCount = 0;
	this->root->content = string("");
	this->root->data = false;
	this->root->as = 0;
	this->allocation = NULL;

	this->_size = 0;
#if DEBUG
	this->_nodes = 0;
	this->_alloc = 0;
#endif
}

RadixTrie::~RadixTrie() {
	this->clear();
}

void RadixTrie::clear() {
	if( this->allocation != NULL ) {
		delete[] this->allocation;
		this->allocation = NULL;
		delete this->root;
	} else if( this->root != NULL ) {
		this->clearNode(this->root);
	}

	this->root = NULL;
}

void RadixTrie::clearNode(node* node) {
	if( node == NULL ) {
#if DEBUG
		this->_size--;
#endif
		return;
	}

	for(unsigned char i=0;i<node->childrenCount;i++) {
		this->clearNode(node->children[i]);
	}

	delete node;
#if DEBUG
	this->_size--;
#endif
}


void RadixTrie::serialize(ostream& stream, node* root, unsigned int* total) {
	(*total)++;

	if( root == this->root ) {
		stream << RadixTrie::SEP_META << this->count() << RadixTrie::SEP_META;
	}

	stream << root->prefix << RadixTrie::SEP;
	stream << root->as << RadixTrie::SEP_CHILD_START;
	for(unsigned char i=0;i<root->childrenCount;++i) {
		serialize(stream, root->children[i], total);
	}
	stream << RadixTrie::SEP_CHILD_END;
}

void RadixTrie::parseFrom(istream& stream, const unsigned short bufferSize) {

	// parse meta
	char sep;
	unsigned int size;
	stream >> sep >> size >> sep;

	// optimize
	this->allocation = new staticNode[size];

	// parse data
	parseElement(stream, NULL, bufferSize);
}


void RadixTrie::parseElement(istream& stream, staticNode* parent, const unsigned short bufferSize) {
	staticNode* newNode;

	while(stream.good()) {

		if( parent != NULL ) {

			newNode = &(this->allocation[this->_size]);
			newNode->childrenCount = 0;
			newNode->staticParent = parent;

			stream.getline(newNode->staticPrefix, bufferSize, RadixTrie::SEP);
			newNode->prefixSize = strlen(newNode->staticPrefix);
			stream.getline(newNode->as, 9, RadixTrie::SEP_CHILD_START);
			newNode->isData = (bool)(newNode->as[0] != '0');

			parent->children[parent->childrenCount++] = newNode;
			this->_size++;
			parent = newNode;

			while( stream.peek() == RadixTrie::SEP_CHILD_END ) {
				stream.ignore(1, RadixTrie::SEP_CHILD_END);
				parent = parent->staticParent;
			}

		} else {
			stream.ignore(1000, RadixTrie::SEP_CHILD_START);
			if( stream.peek() == RadixTrie::SEP_CHILD_END ) {
				return;
			}

			this->staticRoot = new staticNode;
			this->staticRoot->childrenCount = 0;
			strcpy(this->staticRoot->as, "0");
			this->staticRoot->prefixSize = 0;
			this->staticRoot->isData = false;

			parent = this->staticRoot;
		}
	}
}




void RadixTrie::addChild(node* parent, node* child) {
	if( child->parent != NULL || parent == NULL || parent == child || child == NULL ) {
		throw exception();
	}

	parent->children[parent->childrenCount] = child;
	child->parent = parent;
	parent->childrenCount++;
#if DEBUG
	this->_size++;
	this->_nodes++;
#endif
}


void RadixTrie::detachChild(node* parent, node* child) {
	unsigned char index = 0;
	bool found = false;
	for(unsigned char i=0; i < parent->childrenCount; i++) {
		if( parent->children[i] == child ) {
			index = i;
			found = true;
			break;
		}
	}

	if( found ) {
		for(unsigned char i = index; i < parent->childrenCount - 1; i++) {
			parent->children[i] = parent->children[i + 1];
		}

		child->parent = NULL;
		parent->childrenCount--;
	}
}

unsigned int RadixTrie::matchingCharacters(const string& first, const string& second) {
	unsigned int length = first.size();
	if( second.size() < length ) {
		length = second.size();
	}

	const char* str1 = first.c_str();
	const char* str2 = second.c_str();

	unsigned int i;
	for(i = 0; i < length; ++i) {
		if( str1[i] != str2[i] ) {
			return i;
		}
	}

	return length;
}

node* RadixTrie::insert(const string data, const unsigned int as, node* parent) {
	if( parent->content == data ) {
		parent->as = as;
		parent->data = true;
		return NULL;
	}

	node* newNode;
	newNode = new node;
	newNode->parent = NULL;
	newNode->childrenCount = 0;
	newNode->as = as;
	newNode->content = data;
	newNode->data = true;

	unsigned int matchingParent = matchingCharacters(newNode->content, parent->content);

	// no match
	if( matchingParent == 0 ) {

		for(unsigned char i = 0; i < parent->childrenCount; i++) {
			// matches child - insert lower
			if( matchingCharacters(newNode->content, parent->children[i]->content) > 0 ) {
				delete newNode;
				return insert(data, as, parent->children[i]);
			}
		}

		// no match in children nor parent
		newNode->prefix = data;
		addChild(parent, newNode);
	}
	// matching but smaller
	else if( matchingParent == data.size() && data.size() < parent->content.size() ){

		for(unsigned char i = 0; i < parent->childrenCount; i++) {
			// matches child - insert lower
			if( matchingParent < matchingCharacters(newNode->content, parent->children[i]->content) ) {
				delete newNode;
				return insert(data, as, parent->children[i]);
			}
		}

		parent->prefix = parent->content.substr(data.size(), parent->prefix.size());

		node *originalParent = parent->parent;
		detachChild(originalParent, parent);
		addChild(originalParent, newNode);
		newNode->prefix = data.substr(newNode->parent->content.size(), data.size() - newNode->parent->content.size());
		addChild(newNode, parent);
	}
	// matching but larger
	else if( matchingParent == parent->content.size() && parent->content.size() < newNode->content.size() ) {

		for(unsigned char i = 0; i < parent->childrenCount; i++) {
			// matches child - insert lower
			if( matchingParent < matchingCharacters(newNode->content, parent->children[i]->content) ) {
				delete newNode;
				return insert(data, as, parent->children[i]);
			}
		}

		// larger but matches no child - insert and split
		newNode->prefix = newNode->content.substr(parent->content.size(), newNode->content.size()).c_str();
		addChild(parent, newNode);
	}
	// matching but partially
	else {

		node *originalParent = parent->parent;
		detachChild(originalParent, parent);

		node* newParent = new node;
		newParent->parent = NULL;
		newParent->childrenCount = 0;
		newParent->as = 0;
		newParent->content = parent->content.substr(0, matchingParent);
		newParent->data = false;

		addChild(originalParent, newParent);
		int size = matchingParent - parent->content.size() + parent->prefix.size();

		newParent->prefix = parent->prefix.substr(0, size);
		parent->prefix = parent->prefix.substr(size, parent->prefix.size());
		newNode->prefix = newNode->content.substr(matchingParent, newNode->content.size());

		addChild(newParent, parent);
		addChild(newParent, newNode);
#if DEBUG
		this->_size -= 1;
		this->_nodes -= 2;
#endif
	}

	return newNode;
}



void RadixTrie::dump() {
	this->dumpNode(this->root, 0, false);
}

void RadixTrie::dumpNode(node* node, unsigned int level, bool full) {
	if( node == NULL ) {
		return;
	}

	for(unsigned int i=0;i<level;i++) {
		cout << "-";
	}

	if( node == this->root ) {
		cout << "ROOT" << endl;
	} else if( node->data == false ) {
		cout << " " << (full ? node->content : node->prefix) << " [node]" << std::endl;
	} else {
		cout << " " << (full ? node->content : node->prefix) << " => " << node->as << std::endl;
	}

	for(unsigned char i=0;i<node->childrenCount;i++) {
		this->dumpNode(node->children[i], level + 1, full);
	}
}

void RadixTrie::dumpStaticNode(staticNode* node, unsigned int level, bool full) {
	if( node == NULL ) {
		return;
	}

	for(unsigned int i=0;i<level;i++) {
		cout << "-";
	}

	if( node == this->staticRoot ) {
		cout << "ROOT" << endl;
	} else if( !node->as ) {
		cout << " " << node->staticPrefix << " [node]" << std::endl;
	} else {
		cout << " " << node->staticPrefix << " => " << node->as << std::endl;
	}

	for(unsigned char i=0;i<node->childrenCount;i++) {
		this->dumpStaticNode(node->children[i], level + 1, full);
	}
}

void RadixTrie::dumpFull() {
	this->dumpNode(this->root, 0, true);
}


void RadixTrie::printAsNodes(unsigned int as, node* root) {
	if( root == NULL ) {
		return;
	}

	if( root->as == as ) {
		cout << root->content << endl;
	}

	for(unsigned char i=0;i<root->childrenCount;i++) {
		this->printAsNodes(as, root->children[i]);
	}
}
