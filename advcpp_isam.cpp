#include <iostream>
#include "isam.hpp"

using namespace std;

int main()
{/*
	isam<int, string*> index(1, 2);
	index[5] = new string("5");
	index[2] = new string("2");
	index[4] = new string("4"); //any records in the overflow space?
	for (auto&& it : index)
	{
		cout << it.first << ":" << *it.second << " ";
	}
	//output: 2:2 4:4 5:5

	isam<int, double> index2(1, 1);
	index2[1] = 1;
	{ auto it = index2.begin(); it->second = 2; }
	cout << index2[1] << endl;
	//output: 2
	*/
    return 0;
}
