#include <iostream>
#include <string>
#include "isam.hpp"

using namespace std;

int main()
{
	isam<int, int> index0(1, 2);
	index0[5] = 6;
	index0[2] = 3;
	index0[7] = 14; //any records in the overflow space?

	//cout << index0[2] << index0[5] << index0[7] << endl;
	for (auto it = index0.begin(); it != index0.end(); ++it)
	{
		cout << it->first << ":" << it->second << " ";
	}

	isam<int, string*> index(1, 2);
	index[5] = new string("5");
	index[2] = new string("2");
	index[4] = new string("4"); //any records in the overflow space?
	for (auto it = index.begin(); it != index.end(); ++it)
	{
		cout << it->first << ":" << *(it->second) << " ";
	}
	//output: 2:2 4:4 5:5

	//cout << *index[2] << *index[4] << *index[5] << endl;

	isam<int, double> index2(1, 1);
	index2[1] = 1;
	{ auto it = index2.begin(); it->second = 2; }
	cout << index2[1] << endl;
	//output: 2
	
	isam<int, string*> index3(3, 1);
	index3[5] = new string("E");
	index3[2] = new string("B");
	index3[4] = new string("D");
	index3[1] = new string("A");
	index3[6] = new string("F");
	index3[7] = new string("G");
	index3[3] = new string("C");
	for (auto&& e : index3)
	{
		*(e.second) += string("X");
	}
	cout << *(index3[4]) << endl;
	for (auto it = index3.begin(); it != index3.end(); ++it)
	{
		cout << it->first << ":" << *(it->second) << " ";
	}

	isam<float, string*> index4(2, 1);
	index4[2.5] = new string("hi");
	for (auto&& e : index4)
	{
		cout << e.first << ":" << *(e.second) << " ";
		*(e.second) = string("kek");
		cout << e.first << ":" << *(e.second) << " ";
	}
	
    return 0;
}
