//
// Created by xzl on 12/24/17.
//
/*

 g++ serialization.cpp -o serialization.bin -I../ -lboost_serialization -std=c++11


 cf http://www.boost.org/doc/libs/1_62_0/libs/serialization/doc/serialization.html
 */

#include <sstream> // std::ostringstream

#include "vs-types.h"
#include "log.h"

using namespace std;

std::string se()
{
	chunk_desc desc(100, 1000);
	ostringstream ss;

	// save data to archive
	boost::archive::text_oarchive oa(ss);
	oa << desc;

	string s = ss.str();

	I("total %lu bytes", s.size());

	return s;
}

void de(std::string & s)
{
	chunk_desc desc;

	istringstream ss(s);
	boost::archive::text_iarchive ia(ss);

	ia >> desc;

	I("key %lu length_ms %lu", desc.id, desc.length_ms);

}

int main(int ac, char * av[])
{
	auto s = se();
	de(s);
}
