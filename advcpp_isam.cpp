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

	cout << index0[2] << index0[5] << index0[7] << endl;

	isam<int, string*> index(1, 2);
	index[5] = new string("5");
	index[2] = new string("2");
	index[4] = new string("4"); //any records in the overflow space?
	/*for (auto&& it : index)
	{
		cout << it.first << ":" << *it.second << " ";
	}*/
	//output: 2:2 4:4 5:5

	cout << *index[2] << *index[4] << *index[5] << endl;

	isam<int, double> index2(1, 1);
	index2[1] = 1;
	/*{ auto it = index2.begin(); it->second = 2; }*/
	cout << index2[1] << endl;
	//output: 2
	
    return 0;
}
