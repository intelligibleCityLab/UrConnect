// depthmapX - spatial network analysis platform
// Copyright (C) 2017, Petros Koutsolampros

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "glshapemap.h"
#include "mainwindow.h"


inline PafColor getColor(int red, int green, int blue) {
	int color_num = red * 16 * 16 * 16 * 16 + green * 16 * 16 + blue;
	PafColor fixed_color(color_num);
	return std::move(fixed_color);
}

void GLShapeMap::loadGLObjects(ShapeMap &shapeMap) {

	//extern std::map<int, PafColor> FixedColoredLinesMap;
	//if (FixedColoredLinesMap.size() > 0) {
	//	m_lines.loadNetGeoData(shapeMap, FixedColoredLinesMap);
	//}
	//else {
	//	m_lines.loadLineData(shapeMap.getAllLinesWithColour());
	//	m_polygons.loadPolygonData(shapeMap.getAllPolygonsWithColour());
	//}

	//extern std::vector<int> Ref_number_list;
	//PafColor yellow = getColor(255, 255, 0);
	//m_lines.loadLineData(shapeMap.getAllLinesWithColour(yellow, Ref_number_list));

	//TODO:±¸·Ý
	m_lines.loadLineData(shapeMap.getAllLinesWithColour());


	////m_polygons.loadPolygonData(shapeMap.getAllPolygonsWithColour());
}
