#include "ShapeBase.h"

namespace SP_QSP_IO{

ShapeBase::ShapeBase()

{
}


ShapeBase::~ShapeBase()
{
}


/*
void ShapeBase::printShape(){
	using std::cout;
	using std::endl;

	cout << "Size:" << _size << endl;
	cout << "self: " << endl;
	for (auto it = _self.begin(); it != _self.end(); it++)
	{
		cout << *it << endl;
	}

	cout << "nrMoveDir: " << _move.size() << "; " << _moveNewOccupy.size() << "; " << _moveRelocate.size() << endl;
	for (size_t i = 0; i < _move.size(); i++)
	{
		cout << "move: " << i << ":" << _move[i] << endl;
		cout << "occupy:" <<  _moveNewOccupy[i].size() << endl;
		/*for (size_t j = 0; j < _moveNewOccupy[i].size(); j++)
		{
			cout << "\t" << _moveNewOccupy[i][j] << endl;
		}
		cout << "relocate:" << _moveRelocate[i].size() << endl;
		/*for (size_t j = 0; j < _moveRelocate[i].size(); j++)
		{
			cout << "\t" << _moveRelocate[i][j] << endl;
		}
		cout << endl;
	}

	cout << "nrProlifDir: " << _prolif.size() << "; " << _prolifNewOccupy.size() << "; " << _prolifRelocate.size() << endl;
	for (size_t i = 0; i < _prolif.size(); i++)
	{
		cout << "prolif: " << i << ":" << _prolif[i] << endl;
		cout << "occupy:" <<  _prolifNewOccupy[i].size() << endl;
		for (size_t j = 0; j < _prolifNewOccupy[i].size(); j++)
		{
			cout << "\t" << _prolifNewOccupy[i][j] << endl;
		}
		cout << "relocate:" << _prolifRelocate[i].size() << endl;
		
		{
			cout << "\t" << _prolifRelocate[i][j] << endl;
		}
		cout << endl;
	}

	cout << "nrMoveSearch locations: " << _moveSearchSequence.size() << "; " << _moveOccupyInvolvement.size() 
		<< "; " << _moveRelocateInvolvement.size() << endl;
	for (size_t i = 0; i < _moveSearchSequence.size(); i++)
	{
		cout << i << ": " << _moveSearchSequence[i] << "; ";
		for (size_t j = 0; j < _moveOccupyInvolvement[i].size(); j++)
		{
			cout << _moveOccupyInvolvement[i][j] << ",";

		}
		cout << "; ";
		for (size_t j = 0; j < _moveRelocateInvolvement[i].size(); j++)
		{
			cout << _moveRelocateInvolvement[i][j] << ",";
		}
		cout << endl;
	}
	cout << endl;
	cout << "nrProlifSearch locations: " << _prolifSearchSequence.size()
		<< "; " << _prolifOccupyInvolvement.size()
		<< "; " << _prolifRelocateInvolvement.size() << endl;
	for (size_t i = 0; i < _prolifSearchSequence.size(); i++)
	{
		cout << i << ": " << _prolifSearchSequence[i] << "; ";
		for (size_t j = 0; j < _prolifOccupyInvolvement[i].size(); j++)
		{
			cout << _prolifOccupyInvolvement[i][j] << ",";

		}
		cout << "; ";
		for (size_t j = 0; j < _prolifRelocateInvolvement[i].size(); j++)
		{
			cout << _prolifRelocateInvolvement[i][j] << ",";
		}
		cout << endl;
	}
	cout << endl;
}
*/
};