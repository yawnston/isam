#include <iostream>
#include <string>
#include "isam.hpp"

using namespace std;

int main()
{
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
	}
	{
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
	}

	{
		isam<int, double> index2(1, 1);
		index2[1] = 1;
		{ auto it = index2.begin(); it->second = 2; }
		cout << index2[1] << endl;
		//output: 2
	}

	{
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
	}
	{
		isam<float, string*> index4(2, 1);
		index4[2.5] = new string("hi");
		index4[2.4] = new string("h");
		index4[2.3] = new string("_");
		index4[2.2] = new string(".");
		for (auto&& e : index4)
		{
			cout << e.first << ":" << *(e.second) << " ";
			*(e.second) = string("kek");
			cout << e.first << ":" << *(e.second) << " ";
		}
		index4[2.2] = new string(".");
	}

	{
		isam<int, string*> index5(1, 3);
		index5[3] = new string("C");
		index5[1] = new string("A");
		index5[2] = new string("B");
		index5[5] = new string("E");
		index5[4] = new string("D");
		{
			auto it = index5.begin();
			cout << endl;
			cout << it->first << ":" << *(it->second) << " "; ++it;
			cout << it->first << ":" << *(it->second) << " "; ++it;
			{
				auto it2 = it;
				for (; it2 != index5.end(); ++it2)
				{
					cout << it2->first << ":" << *(it2->second) << " ";
					it2->second = new string("XXX");
				}
			}
			cout << endl;
			for (; it != index5.end(); ++it)
			{
				cout << it->first << ":" << *(it->second) << " ";
			}
		}
		for (auto&& e : index5)
		{
			cout << e.first << ":" << *(e.second) << " ";
		}
		for (int i = 1; i <= 5; ++i)
		{
			cout << *index5[i] << " ";
		}
	}

	{
		isam<int, string*> index6(10, 1);
		index6[3] = new string("C");
		index6[1] = new string("A");
		index6[2] = new string("B");
		index6[5] = new string("E");
		index6[4] = new string("D");
		index6[0] = new string("Z");
		index6[-3] = new string("W");
		index6[-2] = new string("X");
		index6[-1] = new string("Y");
		{
			auto it = index6.begin();
			cout << endl << "-------------------------" << endl;
			cout << it->first << ":" << *(it->second) << " "; ++it;
			cout << it->first << ":" << *(it->second) << " "; ++it;
			{
				auto it2 = it;
				for (; it2 != index6.end(); ++it2)
				{
					cout << it2->first << ":" << *(it2->second) << " ";
					if(*(it2->second) == "B")it2->second = new string("XXX");
				}
			}
			cout << endl;
			for (; it != index6.end(); ++it)
			{
				cout << it->first << ":" << *(it->second) << " ";
			}
			cout << endl;
		}
		index6[0] = new string("0");
		for (auto&& e : index6)
		{
			cout << e.first << ":" << *(e.second) << " ";
		}
		cout << endl;
		index6.begin()->second = new string("beg");
		for (int i = -3; i <= 5; ++i)
		{
			cout << *index6[i] << " ";
		}
	}
	
    return 0;
}
