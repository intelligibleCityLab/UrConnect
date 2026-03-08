#pragma once
#include <string>
#include "shapefil.h"
#include "ShapeObject.h"

class Shapefile
{
public:
	Shapefile();
	~Shapefile();
	int			Open(std::string filepath);
	void		Close();

	int			GetEntityCount() { return entityCount; }
	int			GetType() { return shapeType; }
	std::string	GetTypeString();

	int			GetShape(int index, ShapeObject &object);

public:
	class Bounds {
	public:
		int x;
		int y;
		int z;
		int m;
	};

protected:
	SHPHandle	handle;
	int			entityCount;
	int         shapeType;
};

