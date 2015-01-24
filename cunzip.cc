/*****
cunzip.cc
cunzip stands for 'Carter Unzip'.
File zipper which uncompresses data based on the probabilities and structure within the data.
This program is designed to achieve the maximum compression possible with little regard for execution speed.
*****/

#include <iostream>
#include <string>
#include <fstream>
#include "table.h"
#include "arithmetic.h"
#include "file_io.h"

using namespace std;

void decompress_using_static_markov_chain(arithmetic_reader &r, unsigned chain_len, vector<unsigned> &result) {
	//Calculate table dimensions
	unsigned min = 0;
	unsigned max = 256;
	vector<dim_type> table_dims;
	table_dims.resize(chain_len);
	for(unsigned i=0;i < chain_len;i++) {
		table_dims[i].ub = max;
		table_dims[i].lb = min;
	}
	
	//Create and fill table
	table T(table_dims);
	unsigned index = 0;
	while(true) {
		constraint_set initial_constraints;
		if(index < chain_len - 1) {
			gen_even_distribution(min, max, initial_constraints);
		} else {
			gen_table_distribution(T, result, index, chain_len, min, max, initial_constraints);
		}
		while(initial_constraints.size() != 0) {
			if(r.pop_bit(initial_constraints[0].prob))
				break;
			
			remove_distribution_element(initial_constraints, 0); //This is ugly, but easier to generalize than doing it the simple / fast way
			if(initial_constraints.size() == 0) { cout << "EMPTY CONSTRAINT SET" << endl; return; }
		}
		unsigned c = initial_constraints[0].p[initial_constraints[0].p.size() - 1];
		if(c == 256) //Encountered stop character
			break;
		else {
			result.push_back(c);
			if(index >= chain_len - 1)
				T.add_sample(initial_constraints[0].p);
		}
		
		index++;
	}
}

int main(int argc, char* argv[]) {
	//Grab filename
	if(argc < 2) {cout << "Please provide a filename." << endl; return 0;}
	string filename = argv[1];
	if(filename.size() < 3 || filename.substr(filename.size() - 3, filename.size()) != string(".cz")) { cout << "Filename must end in .cz" << endl; return 0;}
	cout << "Decompressing " << filename << " into " << filename.substr(0, filename.size() - 3) << "..." << endl;
	
	//Get decompressed data stream
	arithmetic_reader r(filename);
	vector<unsigned> decompressed_data;
	decompress_using_static_markov_chain(r, 1, decompressed_data);
	
	//Write decompressed file
	string outp_filename = filename.substr(0, filename.size() - 3);
	write_file(outp_filename, decompressed_data);
	
	//Remove original file.  FIXME: this is kind of dangerous without some better error reporting above.
	remove(filename.c_str());
	
	cout << "done." << endl;
	return 0;
}
