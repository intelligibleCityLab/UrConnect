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

#pragma once

#include "depthmapX/GraphDoc.h"

#include "depthmapX/views/mapview.h"
#include "depthmapX/views/glview/gllines.h"
#include "depthmapX/views/glview/gllinesuniform.h"
#include "depthmapX/views/glview/glrastertexture.h"
#include "depthmapX/views/glview/glpolygons.h"
#include "depthmapX/views/glview/glpointmap.h"
#include "depthmapX/views/glview/glshapegraph.h"
#include "depthmapX/views/glview/gldynamicrect.h"
#include "depthmapX/views/glview/gldynamicline.h"

#include <QOpenGLWidget>
#include <QFile>
#include <QOpenGLFunctions>
#include <QMatrix4x4>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLView : public MapView, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLView(QGraphDoc &pDoc,
           Settings &settings,
           QWidget *parent = Q_NULLPTR);
    ~GLView();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
	void notifyDatasetChanged() { m_datasetChanged = true; update(); }
    void matchViewToCurrentMetaGraph();
	void setScale(double scale_value);

	//Ôö¼Ó
	void UpdateBackgroundColor(QRgb new_color);

    virtual void OnModeJoin() override;
    virtual void OnModeUnjoin() override;
    virtual void OnViewPan() override;
    virtual void OnViewZoomIn() override;
    virtual void OnViewZoomOut() override;
    virtual void OnEditFill() override;
    virtual void OnEditSemiFill() override;
    virtual void OnEditAugmentFill() override;
    virtual void OnEditPencil() override;
    virtual void OnModeIsovist() override;
    virtual void OnModeTargetedIsovist() override;
    virtual void OnEditLineTool() override;
    virtual void OnEditPolygonTool() override;
    virtual void OnModeSeedAxial() override;
    virtual void OnEditSelect() override;
    virtual void OnViewZoomsel() override;
    void OnModeStepDepth();
    virtual void postLoadFile() override;
    virtual void OnEditCopy() override;
    virtual void OnEditSave() override;
    virtual void OnViewZoomToRegion(QtRegion region) override;

	////////////////////////////////////////////////////////////////////
	//QCursor m_cursor;

	QPoint m_mouse_point;
	Point2f m_mouse_location;
	int m_fillmode;
	QRect m_drag_rect_a;
	QRect m_drag_rect_b;

	QRgb m_selected_color;

	bool ModeOk();

	// lots of screen drawing booleans... to be tidied up...
	bool m_drawing;
	bool m_continue_drawing;
	//
	int m_invalidate; // <- includes the mode
	bool m_queued_redraw;
	bool m_internal_redraw;
	bool m_clear;
	bool m_redraw;

	bool m_resize_viewport;
	bool m_viewport_set;
	bool m_redraw_all;
	bool m_redraw_no_clear;

	bool m_right_mouse_drag;
	bool m_alt_mode;

	// logical units:
	Point2f m_centre;
	double m_unit;
	// keep tabs on the screen size:
	QSize m_physical_centre;

	// record any previous find location loc:
	Point2f m_lastfindloc;

	// Snap to control (must be in map units for accuracy)
	Point2f m_snap_point;
	Point2f m_old_snap_point;
	QRgb m_cross_pixels[18];
	bool m_snap;
	int m_repaint_tag;

	// Line start and end (must be in map units in case you zoom out / pan while you're drawing!
	Line m_line;
	Line m_old_line;
	std::vector<QRgb> m_line_pixels;
	std::vector<Point2f> m_point_handles;
	int m_active_point_handle;

	//int m_currentlyEditingShapeRef = -1;

	// polygon drawing utilities
	Point2f m_poly_start;
	int m_poly_points;

	//QRgb m_background;
	//QRgb m_foreground;

	///////////////////////////////////////////////////////////////////////

	QtRegion LogicalViewport(const QRect& phys_bounds, QGraphDoc *pDoc);

	Point2f LogicalUnits(const QPoint& p);
	QPoint PhysicalUnits(const Point2f& p);

	int GetSpacer(QGraphDoc *pDoc);

	void OutputEPS(std::ofstream& stream, QGraphDoc *pDoc, bool includeScale = true);
	void OutputEPSMap(std::ofstream& stream, ShapeMap& map, QtRegion& logicalviewport, QRect& rect, float spacer);
	void OutputEPSLine(std::ofstream& stream, SimpleLine& line, int spacer, QtRegion& logicalviewport, QRect& rect);
	void OutputEPSPoly(std::ofstream& stream, const SalaShape& shape, int spacer, QtRegion& logicalviewport, QRect& rect);

    void OutputPNGbyScreenShot(QFile* file, QGraphDoc* pDoc);

    void OutputPNG(const QString& filename, QGraphDoc* pDoc);
    void OutputPNGMap(QPainter& painter, ShapeMap& map, QtRegion& logicalviewport, int h);
	void OutputPNGLine(QPainter& painter, SimpleLine& line, QtRegion& logicalviewport, int h);
	void OutputPNGPoly(QPainter& painter, const SalaShape& shape, QtRegion& logicalviewport, int h);
    QPoint ConvertToPixels(const Point2f& point, QtRegion& logicalviewport, int h);


	void OutputSVG(QFile *file, QGraphDoc *pDoc);
	void OutputSVGMap(QTextStream& stream, ShapeMap& map, QtRegion& logicalviewport, int h);
	void OutputSVGLine(QTextStream& stream, SimpleLine& line, QtRegion& logicalviewport, int h);
	void OutputSVGPoly(QTextStream& stream, const SalaShape& shape, QtRegion& logicalviewport, int h);

signals:
	void sizeChanged(int height, int width);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *object, QEvent *e) override;
    void closeEvent(QCloseEvent *event) override;

private:
    bool m_perspectiveView = false;
    bool m_core;
    QMatrix4x4 m_mProj;
    QMatrix4x4 m_mView;
    QMatrix4x4 m_mModel;

    QRgb m_foreground;
    QRgb m_background;
    QSize m_initialSize;

    GLDynamicRect m_selectionRect;
    GLDynamicLine m_dragLine;
    GLLines m_axes;
    GLShapeGraph m_visibleShapeGraph;
    GLLinesUniform m_visibleDrawingLines;
    GLPointMap m_visiblePointMap;
    GLShapeMap m_visibleDataMap;

    QPoint m_mouseLastPos;
    float m_eyePosX;
    float m_eyePosY;
    float m_minZoomFactor = 1;
    float m_zoomFactor = 20;
    float m_maxZoomFactor = 200;
    GLfloat m_screenRatio;
    int m_screenWidth;
    int m_screenHeight;
    bool m_wasPanning = false;

    int m_antialiasingSamples = 0; // set this to 0 if rendering is too slow

    Point2f getWorldPoint(const QPoint &screenPoint);
    QPoint getScreenPoint(const Point2f &worldPoint);

    bool m_datasetChanged = false;

    void panBy(int dx, int dy);
    void recalcView();
    void zoomBy(float dzf, int mouseX, int mouseY);
    void resetView();

	QCursor m_cursor;
	void SetCursor();

    void loadAxes();
    void loadDrawingGLObjects();

    enum {
        MOUSE_MODE_NONE = 0x0000,
        MOUSE_MODE_SELECT = 0x10000,
        MOUSE_MODE_PAN = 0x0101,
        MOUSE_MODE_ZOOM_IN = 0x0202,
        MOUSE_MODE_ZOOM_OUT = 0x0204,
        MOUSE_MODE_FILL_FULL = 0x0001,
        MOUSE_MODE_FILL_SEMI = 0x0002,
        MOUSE_MODE_FILL_AUGMENT = 0x0003,
        MOUSE_MODE_PENCIL = 0x0801,
        MOUSE_MODE_SEED_ISOVIST = 0x4001,
        MOUSE_MODE_SEED_TARGETED_ISOVIST = 0x4002,
        MOUSE_MODE_SEED_AXIAL = 0x0004,
        MOUSE_MODE_LINE_TOOL = 0x0008,
        MOUSE_MODE_POLYGON_TOOL = 0x0010,
        MOUSE_MODE_POINT_STEP_DEPTH = 0x5000,
        MOUSE_MODE_JOIN = 0x20001,
        MOUSE_MODE_UNJOIN = 0x20002,
        MOUSE_MODE_SECOND_POINT = 0x00400,
    };

    int m_mouseMode = MOUSE_MODE_SELECT;

    QRectF m_mouseDragRect = QRectF(0,0,0,0);

    Point2f m_tempFirstPoint;
    Point2f m_tempSecondPoint;

    int m_currentlyEditingShapeRef = -1;

    Point2f m_polyStart;
    int m_polyPoints = 0;

    inline int PixelDist(QPoint a, QPoint b)
    {
       return (int)sqrt(double((b.x()-a.x())*(b.x()-a.x())+(b.y()-a.y())*(b.y()-a.y())));
    }
};

