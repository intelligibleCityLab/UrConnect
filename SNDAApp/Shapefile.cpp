#include "stdafx.h"
#include "Shapefile.h"


Shapefile::Shapefile()
	: handle(0)
	, entityCount(0)
	, shapeType(0)
{
}


Shapefile::~Shapefile()
{
	Close();
}


int Shapefile::Open(std::string filepath)
{
	Close();
	handle = SHPOpen(filepath.c_str(), "rb");
	if (handle == 0) {
		return -1;
	}
	else {
		SHPGetInfo(handle, &entityCount, &shapeType, 0, 0);
	}
	return 0;
}

void Shapefile::Close()
{
	if (handle != 0) {
		SHPClose(handle);
	}
	handle = 0;
}

std::string Shapefile::GetTypeString()
{
	return ShapeTypeAsString(shapeType);
}


int Shapefile::GetShape(int index, ShapeObject &object)
{
	if (handle == 0) {
		return -1;
	}
	SHPObject *obj = SHPReadObject(handle, index);
	object.Assign(obj);
	SHPDestroyObject(obj);
	return 0;
}
