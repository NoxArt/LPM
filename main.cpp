/**
 * VUT FIT Brno: PDS
 *
 * Longest-Prefix Match
 *
 * Jiri Petruzelka
 * <xpetru07>
 * 2012/2013
 */
#define EXIT_HELP 2
#define EXIT_MAPPING_EMPTY 3

#define INPUT_BUFFER_SIZE 64
#define IPV4_BUFFER_SIZE 16
#define IPV4_BIN_BUFFER_SIZE 32
#define IPV6_BUFFER_SIZE 39
#define IPV6_BUFFER_SIZE_1 40
#define IPV6_BIN_BUFFER_SIZE 128
#define OUTPUT_BUFFER_SIZE 512
#define OUTPUT_BUFFER_SIZE_SAFE 500

#define IPV6_DISABLED 0

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif //WIN32

// std
#include <stdlib.h>
#include <stdio.h>

// string
#include <cstring>
#include <string>

// streams
#include <iostream>
#include <fstream>
#include <math.h>

// data structures
#include <bitset>
#include "tree.h"

#define MAX(a,b) (a > b ? a : b)
#define MIN(a,b) (a < b ? a : b)
#define ROUND(x, n) (floor(MAX(0,x) * pow(10,n)) / pow(10,n))

using namespace std;

double treetime = 0;
double converttime = 0;

/**
 * Prints help message onto stderr
 */
void printHelp(void) {
	cerr << "VUT FIT Brno 2012/2012" << endl;
	cerr << "PDS: Longest-Prefix Match" << endl;
	cerr << "Jiri Petruzelka <xpetru07>" << endl << endl;
	cerr << "Usage:" << endl;
	cerr << "\tlpm -i mapping_file_path < ip.txt\t\t... IP matching" << endl;
	cerr << "\tlpm -g mapping_file_path\t\t\t... generate trees" << endl;
	cerr << "See https://wis.fit.vutbr.cz/FIT/st/course-sl.php?id=503602&item=41654";
}


void ipv4_char(const char* ip, char* out, const unsigned int limit) {
	unsigned int n = 0;
	unsigned int m = 0;
	char buffer[4];
	bitset<8> bits;

	const unsigned int length = strlen(ip);
	for(unsigned int i = 0; i <= length; ++i) {
		if( ip[i] == '.' || i == length ) {
			buffer[n] = '\0';
			bits = bitset<8>(atol(buffer));
			for(int j=7;j >= 0; --j) {
				out[m++] = bits[j] + 48;
				if( m >= limit ) {
					i = length;
					break;
				}
			}

			n = 0;
			continue;
		}

		buffer[n++] = ip[i];
	}

	out[m] = '\0';
}


inline void ipv4_char_opt(const char* ip, char* out, const unsigned int length) {
	unsigned int n = 0;
	unsigned int m = 0;
	int j;
	char buffer[4];
	bitset<8> bits;

	for(unsigned int i = 0; i < length; ++i) {
		if( ip[i] == '.' ) {
			buffer[n] = '\0';
			bits = bitset<8>(atol(buffer));
			for(j=7;j >= 0;--j) {
				out[m++] = bits[j] + 48;
			}

			n = 0;
			continue;
		}

		buffer[n++] = ip[i];
	}

	// i == length
	buffer[n] = '\0';
	bits = bitset<8>(atol(buffer));
	for(j=7;j >= 0;--j) {
		out[m++] = bits[j] + 48;
	}

	out[IPV4_BIN_BUFFER_SIZE] = '\0';
}

inline string ipv4_string(const string input) {
	char output[IPV4_BIN_BUFFER_SIZE + 1];
	ipv4_char(input.c_str(), output, IPV4_BIN_BUFFER_SIZE);
	return string(output);
}

void ipv6_char(const char* ip, char* out, const unsigned int limit) {
	unsigned int n = 0;
	unsigned int m = 0;
	unsigned int dotsTotal = 0;
	char buffer[4];
	bitset<16> bits;

	unsigned int i;
	const unsigned int length = strlen(ip);

	for(i = 0; i <= length; i++) {
		if( ip[i] == ':' ) {

			if( ip[i - 1] == ':' ) {
				dotsTotal--;
			} else {
				dotsTotal++;
			}

		}
	}

	for(i = 0; i <= length; i++) {
		if( ip[i] == ':' || i == length ) {

			// handle double dot
			if( ip[i] == ':' && ip[i - 1] == ':' ) {

				if( i + 1 == length ) {
					for(;m < limit; m++ ) {
						out[m] = '0';
					}
					i = length;
					break;
				} else {

					const unsigned int dec = i == length ? 0 : 16;
					const unsigned int zerosRemaining = MIN( m + (7 - dotsTotal) * 16 - dec, limit ) - m;
					for(unsigned int dd = 0; dd < zerosRemaining; dd++) {
						out[m++] = '0';
					}

					continue;
				}
			}

			// parse normal data
			buffer[n] = '\0';
			bits = bitset<16>(strtol(buffer, NULL, 16));

			for(int j=15;j >= 0;--j) {
				out[m++] = bits[j] + 48;
				if( m >= limit ) {
					i = length;
					break;
				}
			}

			n = 0;
			continue;
		}

		buffer[n++] = ip[i];
	}

	out[m] = '\0';
}

inline string ipv6_string(const string input) {
	char output[IPV6_BIN_BUFFER_SIZE + 1];
	ipv6_char(input.c_str(), output, IPV6_BIN_BUFFER_SIZE);
	return string(output);
}

inline void ipv6_char_opt(const char* ip, char* out, const unsigned int length) {
	unsigned int n = 0;
	unsigned int m = 0;
	unsigned int dotsTotal = 0;
	char buffer[4];
	bitset<16> bits;

	unsigned int i;

	for(i = 0; i <= length; i++) {
		if( ip[i] == ':' ) {

			if( ip[i - 1] == ':' ) {
				dotsTotal--;
			} else {
				dotsTotal++;
			}

		}
	}

	for(i = 0; i <= length; i++) {
		if( ip[i] == ':' || i == length ) {

			// handle double dot
			if( ip[i] == ':' && i > 0 && ip[i - 1] == ':' ) {

				const unsigned int dec = i == length ? 0 : 16;
				const unsigned int zerosRemaining = MIN( m + (7 - dotsTotal) * 16 - dec, IPV6_BIN_BUFFER_SIZE ) - m;
				for(unsigned int dd = 0; dd < zerosRemaining; dd++) {
					out[m++] = '0';
				}

				continue;
			}

			// parse normal data
			buffer[n] = '\0';
			bits = bitset<16>(strtol(buffer, NULL, 16));

			for(int j=15;j >= 0;--j) {
				out[m++] = bits[j] + 48;
			}

			n = 0;
			continue;
		}

		if( n < 4 ) {
			buffer[n++] = ip[i];
		}
	}

	out[IPV6_BIN_BUFFER_SIZE] = '\0';
}



/**
 * Loads data from given file path
 * @param filePath
 */
void loadMappingFile(const string filePath, RadixTrie& tree, RadixTrie& tree6) {
	ifstream file;

	char buffer[INPUT_BUFFER_SIZE];
	unsigned int bufferLength;

	char buffer2[IPV4_BIN_BUFFER_SIZE + 1];
	char buffer3[IPV6_BIN_BUFFER_SIZE + 1];

	char ip[IPV6_BUFFER_SIZE_1];

	char mask[8];
	unsigned char maskLength;
	char as[9];
	unsigned char asLength;

	bool ipv6;
	char phase;
	double start;

	try {
		file.open(filePath.c_str(), ifstream::in);

		while(file.eof() == false) {
			file.getline(buffer, INPUT_BUFFER_SIZE);

			ipv6 = false;
			phase = 0;
			maskLength = 0;
			asLength = 0;
			bufferLength = strlen(buffer);

			if( bufferLength == 0 ) {
				break;
			}

			for(unsigned int i = 0; i < bufferLength; i++) {

				if( buffer[i] == ':' ) {
					ipv6 = true;
				}

				if( phase == 0 ) {
					if( buffer[i] == '/' ) {
						phase = 1;
						ip[i] = '\0';
					} else {
						ip[i] = buffer[i];
					}
				} else if( phase == 1 ) {
					if( buffer[i] == ' ' ) {
						phase = 2;
						mask[maskLength] = '\0';
					} else {
						mask[maskLength++] = buffer[i];
					}
				} else if( buffer[i] >= '0' && buffer[i] <= '9' ) {
					as[asLength++] = buffer[i];
				}
			}
			as[asLength] = '\0';

			if( ipv6 == true) {
				start = getTime();
				ipv6_char(ip, buffer3, atoi(mask));
				converttime += getTime() - start;

				start = getTime();
				tree6.insert(buffer3, (unsigned int)atoi(as));
				treetime += getTime() - start;
			} else {
				start = getTime();
				ipv4_char(ip, buffer2, atoi(mask));
				converttime += getTime() - start;

				start = getTime();
				tree.insert(buffer2, (unsigned int)atoi(as));
				treetime += getTime() - start;
			}

			buffer[0] = '\0';
			bufferLength = 0;
		}
	} catch (const ifstream::failure &e) {
		cerr << "Exception opening/reading file: ";
		cerr << e.what();
	}

	file.close();
}


/*
 * Run matching
 */
int main(int argc, char** argv) {

	// break sync
	ios_base::sync_with_stdio(false);

	// source data
	string inputFilePath, inputTempPath4, inputTempPath6;
	RadixTrie tree, tree6;


	// debugging
	bool debug = false;
	bool simpleDebug = false;
	bool forceGenerate = false;
	double time, sstart;
	unsigned int mapped = 0;

	// io
	ifstream inputFile;
	unsigned int buffered = 0;

	// handle command line options
	if( argc < 3 || (strcmp(argv[1], "-i") != 0 && strcmp(argv[1], "-d") != 0 && strcmp(argv[1], "-g") != 0 && strcmp(argv[1], "-s") != 0)) {
		printHelp();
		return EXIT_HELP;
	} else {
		inputFilePath = string(argv[2]);
		inputTempPath4 = string(inputFilePath + ".tree4");
		inputTempPath6 = string(inputFilePath + ".tree6");

		// set debug mode
		if( strcmp(argv[1], "-d") == 0 ) {
			debug = true;
		} else if( strcmp(argv[1], "-g") == 0 ) {
			forceGenerate = true;
		}  else if( strcmp(argv[1], "-s") == 0 ) {
			simpleDebug = true;
		}
	}

	// init measure load time
	if( debug ) {
		time = getTime();
		sstart = getTime();
	}

	// load data
	ifstream serialized4(inputTempPath4.c_str());
	ifstream serialized6(inputTempPath6.c_str());

	if( forceGenerate == false && serialized4 && serialized6 ) {
		tree.parseFrom(serialized4, IPV4_BIN_BUFFER_SIZE + 1);
		tree6.parseFrom(serialized6, IPV6_BIN_BUFFER_SIZE + 1);

		if( debug ) {
			cerr << endl << "Deserialization time: " << ROUND((getTime() - sstart)*1000,5) << " ms" << endl;
		}

		serialized4.close();
		serialized6.close();
	} else {
		serialized4.close();
		serialized6.close();

		// load
		loadMappingFile(inputFilePath, tree, tree6);

		// serialize
		unsigned int total4 = 0;
		ofstream serialize4(inputTempPath4.c_str(), ios_base::trunc);
		tree.serialize(serialize4, tree.getRoot(), &total4);
		serialize4.close();

		unsigned int total6 = 0;
		ofstream serialize6(inputTempPath6.c_str(), ios_base::trunc);
		tree6.serialize(serialize6, tree6.getRoot(), &total6);
		serialize6.close();

		if( debug ) {
			cerr << endl << "Serialization time: " << ROUND((getTime() - sstart)*1000,5) << " ms" << endl;
			cerr << "Total IPv4: " << total4 << " " << tree.count() << endl;
			cerr << "Total IPv6: " << total6 << " " << tree6.count() << endl << endl;
		}

		return 0;
	}

	// measure load time
	if( debug ) {
		cerr << "Loaded size: " << (tree.count() + tree6.count()) << endl;
		cerr << " - IPV4: "  << tree.count() << endl;
		cerr << " - IPV6: "  << tree6.count() << endl;
		cerr << "Data load time: " << ROUND((getTime() - time) * 1000, 5) << " ms" << endl;
	}

	// mapping empty
	if( tree.count() == 0 && tree6.count() == 0 ) {
		return EXIT_MAPPING_EMPTY;
	}

	// init matching loop
	if( debug || simpleDebug ) {
		time = getTime();
	}

	// init
	staticNode* located;
	char tbuffer[INPUT_BUFFER_SIZE];
	char ip4[IPV4_BIN_BUFFER_SIZE + 1];
	char ip6[IPV6_BIN_BUFFER_SIZE + 1];

	unsigned char length;
	bool ipv4;
	unsigned int i;

	char obuffer[OUTPUT_BUFFER_SIZE];

	// matching loop
	while( fgets(tbuffer, INPUT_BUFFER_SIZE, stdin) != NULL ) {

		// fetch input
		length = strlen(tbuffer);
		if( length == 0 ) {
			break;
		}

		// detect ipv6
		ipv4 = false;
		for(i = 1; i < length; ++i) { // 1 intentional, should not start with delimiter
			if( tbuffer[i] == '.' ) {
				ipv4 = true;
				break;
			} else if( tbuffer[i] == ':' ) {
				break;
			}
		}

		// perform matching
		if( ipv4 ) {
			ipv4_char_opt(tbuffer, ip4, length);
			located = tree.find(ip4);
		} else {
			ipv6_char_opt(tbuffer, ip6, length);
			located = tree6.find(ip6);
		}

		// set output
		if( located == NULL || located->isData == false ) {
			obuffer[buffered++] = '-';
			obuffer[buffered++] = '\n';
		} else {
			for(i=0;i<8;++i) {
				if( located->as[i] != '\0' ) {
					obuffer[buffered++] = located->as[i];
				} else {
					break;
				}
			}
			obuffer[buffered++] = '\n';
		}

		mapped++;

		if( buffered >= OUTPUT_BUFFER_SIZE_SAFE ) {
			obuffer[buffered] = '\0';
			cout << obuffer;
			buffered = 0;
			obuffer[0] = '\0';
		}
	}

	// output remaining contents of buffer
	if( buffered > 0 ) {
		obuffer[buffered] = '\0';
		cout << obuffer;
		buffered = 0;
		obuffer[0] = '\0';
	}

	// measure mapping time
	if( debug ) {
		double totalTime = getTime() - time;
		cerr << endl;
		cerr << "Mapped entries: " << mapped << endl;
		cerr << "Mapping time: " << ROUND(totalTime*1000, 6) << " ms" << endl;
		cerr << "Mapping 80K: " << ROUND(totalTime*1000*((float)80000 / (float)mapped), 6) << " ms" << endl;
		cerr << "Mapping 1M: " << ROUND(totalTime*1000*((float)1000000 / (float)mapped), 6) << " ms" << endl;
		cerr << "Mapping speed: " << fixed << floor(mapped / totalTime) << " entry/sec" << endl;
	} else if( simpleDebug ) {
		cerr << fixed << floor(mapped / (getTime() - time)) << endl;
	}


	tree.clear();
	tree6.clear();

	return EXIT_SUCCESS;
}

