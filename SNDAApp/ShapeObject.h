#pragma once
#include <string>
#include <vector>

#include "shapefil.h"

#include "Primitives.h"

class ShapeObject
{
	public:
		struct Part
		{
			int type;
			int offset;
			int length;

			Part() : type(0), offset(0), length(0) {}
		};

	protected:
		int					index;
		int					type;
		int					vertexCount;
		sPoint<double>		*vertices;
		Box<double>			bounds;
		std::vector<Part>	parts;

	public:
		ShapeObject();
		ShapeObject(const SHPObject* obj);
		ShapeObject(const ShapeObject& other);
		~ShapeObject();

		ShapeObject& operator = (const ShapeObject& other);

		void Assign(const SHPObject* obj);

		bool IsValid() const;

		protected:
			void Destroy();
			void Assign(const ShapeObject& obj);

		public:
			int						GetIndex() const;
			int						GetType() const;
			std::string				GetTypeString() const;

			int						GetVertexCount() const;
			const sPoint<double>*	GetVertices() const;
			Box<double>				GetBounds() const;

			std::vector<Part>		GetParts() const;
};
std::string ShapeTypeAsString(int shapetype);

