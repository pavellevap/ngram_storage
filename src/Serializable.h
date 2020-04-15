//
// Created by pavel on 01.01.18.
//

#ifndef COMPACTNGRAMSTORAGE_SERIALIZABLE_H
#define COMPACTNGRAMSTORAGE_SERIALIZABLE_H

#include <iostream>
#include <fstream>
#include <sstream>

using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;
using std::istringstream;
using std::ostringstream;
using std::string;


class Serializable {
public:
    virtual void dump(ostream& out) const = 0;
    virtual void load(istream& in) = 0;

    void dumpf(const string& filename) const {
        ofstream fout(filename);
        dump(fout);
    }

    void loadf(const string filename) {
        ifstream fin(filename);
        load(fin);
    }

    string dumps() const {
        ostringstream sout(std::ios::out | std::ios::binary);
        dump(sout);
        return sout.str();
    }

    void loads(const string& state) {
        istringstream sin(state, std::ios::in | std::ios::binary);
        load(sin);
    }
};


#endif //COMPACTNGRAMSTORAGE_SERIALIZABLE_H
