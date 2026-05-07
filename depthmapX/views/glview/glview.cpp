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


#include "glview.h"
#include "depthmapX/views/depthmapview/depthmapview.h"
#include "salalib/linkutils.h"
#include "salalib/geometrygenerators.h"
#include "mainwindow.h"
#include <QMouseEvent>
#include <QCoreApplication>
#include <QGLWidget>
#include <QPainter>
#include <qfiledialog.h>
#include <QtWidgets/QMessageBox>

double scale_value = 0, real_width = 0, screen_width = 1;
double init_scale = -1, init_screenRatio = -1;

static QRgb colorMerge(QRgb color, QRgb mergecolor)
{
   return (color & 0x006f6f6f) | (mergecolor & 0x00a0a0a0);
}

inline Point2f GLView::LogicalUnits(const QPoint& p)
{
	
	return m_centre + Point2f(m_unit * double(p.x() - m_physical_centre.width()),
		m_unit * double(m_physical_centre.height() - p.y()));
}

inline QPoint GLView::PhysicalUnits(const Point2f& p)
{
	return QPoint(m_physical_centre.width() + int((p.x - m_centre.x) / m_unit + 0.4999),
		m_physical_centre.height() - int((p.y - m_centre.y) / m_unit + 0.4999));
}

inline int PixelDist(QPoint a, QPoint b)
{
	return (int)sqrt(double((b.x() - a.x())*(b.x() - a.x()) + (b.y() - a.y())*(b.y() - a.y())));
}

//static QRgb colorMerge(QRgb color, QRgb mergecolor)
//{
//	return (color & 0x006f6f6f) | (mergecolor & 0x00a0a0a0);
//}

GLView::GLView(QGraphDoc &pDoc,
               Settings &settings,
               QWidget *parent)
    : MapView(pDoc, settings, parent),
      m_eyePosX(0),
      m_eyePosY(0)
{
    m_core = QCoreApplication::arguments().contains(QStringLiteral("--coreprofile"));

    m_foreground = settings.readSetting(SettingTag::foregroundColour, qRgb(128,255,128)).toInt();
    m_background = settings.readSetting(SettingTag::backgroundColour, qRgb(0,0,0)).toInt();
    m_initialSize = m_settings.readSetting(SettingTag::depthmapViewSize, QSize(2000, 2000)).toSize();
    m_antialiasingSamples = settings.readSetting(SettingTag::antialiasingSamples, 0).toInt();

    if(m_antialiasingSamples) {
        QSurfaceFormat format;
        format.setSamples(m_antialiasingSamples);    // Set the number of samples used for multisampling
        setFormat(format);
    }

    loadDrawingGLObjects();

    loadAxes();

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
        m_visibleShapeGraph.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedShapeGraph());
    }
    m_visiblePointMap.setGridColour(colorMerge(m_foreground, m_background));
    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
        m_visiblePointMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedPointMap());
    }

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
        m_visibleDataMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedDataMap());
    }

    m_dragLine.setStrokeColour(m_foreground);
    m_selectionRect.setStrokeColour(m_background);

    matchViewToCurrentMetaGraph();

    installEventFilter(this);
    setMouseTracking(true);
    m_pDoc.m_view[QGraphDoc::VIEW_MAP_GL] = this;


	m_drag_rect_a.setRect(0, 0, 0, 0);
	m_drag_rect_b.setRect(0, 0, 0, 0);

	// Several screen drawing booleans:
	m_continue_drawing = false;
	m_drawing = false;
	m_queued_redraw = false;

	m_viewport_set = true;
	m_clear = false;
	m_redraw = false;

	m_redraw_all = false;
	m_redraw_no_clear = false;

	m_resize_viewport = false;
	m_invalidate = false; // our own invalidation

	m_right_mouse_drag = false;
	m_alt_mode = false;

	//m_current_mode = NONE;

	m_snap = false;
	m_repaint_tag = 0;
	//m_showlinks = false;
	//m_mouse_mode = SELECT;
	//m_fillmode = FULLFILL;

	m_active_point_handle = -1;
	m_poly_points = 0;
	PafColor selcol(SALA_SELECTED_COLOR);

	m_selected_color = qRgb(selcol.redb(), selcol.greenb(), selcol.blueb());

	m_initialSize = m_settings.readSetting(SettingTag::depthmapViewSize, QSize(2000, 2000)).toSize();
}

void GLView::UpdateBackgroundColor(QRgb new_color) {
	m_background = new_color;
	m_settings.writeSetting(SettingTag::backgroundColour, new_color);

	m_visiblePointMap.setGridColour(colorMerge(m_foreground, m_background));
	m_dragLine.setStrokeColour(m_foreground);
	m_selectionRect.setStrokeColour(m_background);

	std::unique_ptr<QDepthmapView> tmp(new QDepthmapView(m_pDoc, m_settings));
	tmp->updateBackgroundColor(new_color);

}

GLView::~GLView()
{
    makeCurrent();
    m_selectionRect.cleanup();
    m_dragLine.cleanup();
    m_axes.cleanup();
    m_visibleDrawingLines.cleanup();
    m_visiblePointMap.cleanup();
    m_visibleShapeGraph.cleanup();
    m_visibleDataMap.cleanup();
    doneCurrent();
    m_settings.writeSetting(SettingTag::depthmapViewSize, size());
}

QSize GLView::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLView::sizeHint() const
{
    return m_initialSize;
}

void GLView::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(qRed(m_background)/255.0f, qGreen(m_background)/255.0f, qBlue(m_background)/255.0f, 1);  //设置背景颜色

    m_selectionRect.initializeGL(m_core);
    m_dragLine.initializeGL(m_core);
    m_axes.initializeGL(m_core);
    m_visibleDrawingLines.initializeGL(m_core);
    m_visiblePointMap.initializeGL(m_core);
    m_visibleShapeGraph.initializeGL(m_core);
    m_visibleDataMap.initializeGL(m_core);

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
         m_visiblePointMap.loadGLObjectsRequiringGLContext(m_pDoc.m_meta_graph->getDisplayedPointMap());
    }

    m_mModel.setToIdentity();

    m_mView.setToIdentity();
    m_mView.translate(0, 0, -1);
}

void GLView::paintGL()
{
    glEnable(GL_MULTISAMPLE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

	glClearColor(qRed(m_background) / 255.0f, qGreen(m_background) / 255.0f, qBlue(m_background) / 255.0f, 1);  //设置背景颜色

    if(m_datasetChanged) {

        loadDrawingGLObjects();
        m_visibleDrawingLines.updateGL(m_core);

        if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL
                && m_pDoc.m_meta_graph->getDisplayedMapRef() != -1) {
            m_visibleShapeGraph.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedShapeGraph());
            m_visibleShapeGraph.updateGL(m_core);
        }

        if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
            m_visibleDataMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedDataMap());
            m_visibleDataMap.updateGL(m_core);
        }

        if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
            m_visiblePointMap.loadGLObjects(m_pDoc.m_meta_graph->getDisplayedPointMap());
            m_visiblePointMap.updateGL(m_core);
            m_visiblePointMap.loadGLObjectsRequiringGLContext(m_pDoc.m_meta_graph->getDisplayedPointMap());
        }

        m_datasetChanged = false;
    }

    m_axes.paintGL(m_mProj, m_mView, m_mModel);

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
        m_visiblePointMap.showGrid(m_pDoc.m_meta_graph->m_showgrid);
        m_visiblePointMap.paintGL(m_mProj, m_mView, m_mModel);
    }

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
        m_visibleShapeGraph.paintGL(m_mProj, m_mView, m_mModel);
    }

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
        m_visibleDataMap.paintGL(m_mProj, m_mView, m_mModel);
    }

    m_visibleDrawingLines.paintGL(m_mProj, m_mView, m_mModel);

    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
        m_visiblePointMap.paintGLOverlay(m_mProj, m_mView, m_mModel);
    }
    if(m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
        m_visibleShapeGraph.paintGLOverlay(m_mProj, m_mView, m_mModel);
    }


    float pos [] = {
        float(std::min(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x())),
        float(std::min(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y())),
        float(std::max(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x())),
        float(std::max(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y()))
    };
    m_selectionRect.paintGL(m_mProj, m_mView, m_mModel, QMatrix2x2(pos));

    if((m_mouseMode & MOUSE_MODE_SECOND_POINT) == MOUSE_MODE_SECOND_POINT) {
        float pos [] = {
            float(m_tempFirstPoint.x),
            float(m_tempFirstPoint.y),
            float(m_tempSecondPoint.x),
            float(m_tempSecondPoint.y)
        };
        m_dragLine.paintGL(m_mProj, m_mView, m_mModel, QMatrix2x2(pos));
    }

	//opengl坐标系:原点位于窗口中心，坐标数值是范围-1~1，1就是窗口大小，(x,y,z)其中x是横向距离，y是纵向距离，z是轴向距离(平面图保持z=0)

	//计算占用像素大小
	extern std::map<std::string, Point2f> RealMap;
	std::map<std::string, QPoint> ScreenMap;
	for (auto it = RealMap.begin(); it != RealMap.end(); it++) {
		ScreenMap[it->first] = getScreenPoint(it->second);
	}
	if (ScreenMap.size() > 0) {
		real_width = RealMap["right"].x - RealMap["left"].x;
		screen_width = ScreenMap["right"].x() - ScreenMap["left"].x();
	}

	//GL窗口的宽高
	int width = m_screenWidth;
	int height = m_screenHeight;

	//设置当前绘图使用的颜色
	extern PafColor global_color;
	//glColor3f(255.0f, 0.0f, 0.0f);
	glColor3f(global_color.redf(), global_color.greenf(), global_color.bluef());
	
	float left_x = -0.992f, left_y = -0.992f;
	float right_x = -0.85f, right_y = -0.988f;
	float line_len = 0.01f;

	//计算比例尺
	double screen_scale = 0.5*width * (right_x - left_x);
	scale_value = (real_width / screen_width)*screen_scale;
	string scale_str = std::to_string(int(scale_value)) + "m";
	if (init_scale == -1 && scale_value > 0) {
		init_scale = scale_value;
		init_screenRatio = m_screenRatio;
	}

	//绘制矩形
	glRectf(left_x, left_y, right_x, right_y);   //参数分别为左下角坐标 和 右上角坐标 坐标系为opengl坐标系

	//画直线
	glBegin(GL_LINES);
	glVertex3f(left_x, left_y, 0);
	glVertex3f(left_x, left_y + line_len, 0);
	glVertex3f(right_x, left_y, 0);
	glVertex3f(right_x, left_y + line_len, 0);
	glEnd();

	//刷新缓冲，保证绘图命令能被执行
	glFlush();

	QPainter painter(this);
	painter.setRenderHint(QPainter::TextAntialiasing, true);
	painter.setPen(QColor::fromRgbF(global_color.redf(), global_color.greenf(), global_color.bluef()));
	painter.setFont(QFont("Times", 8));
	const int text_x = int(((-0.94f + 1.0f) * 0.5f) * width);
	const int text_y = std::max(12, std::min(height - 6, int(((1.0f - (-0.98f)) * 0.5f) * height)));
	painter.drawText(text_x, text_y, QString::fromStdString(scale_str));

}

void GLView::setScale(double scale_value) {

	zoomBy(float(scale_value) / m_screenRatio, m_screenWidth*0.5, m_screenHeight*0.5);
}

void GLView::loadAxes() {
    std::vector<std::pair<SimpleLine, PafColor>> axesData;
    axesData.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(0,0,1,0), PafColor(1,0,0)));
    axesData.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(0,0,0,1), PafColor(0,1,0)));
    m_axes.loadLineData(axesData);
}

void GLView::loadDrawingGLObjects() {
    auto lock = m_pDoc.m_meta_graph->getLock();
    m_visibleDrawingLines.loadLineData(m_pDoc.m_meta_graph->getVisibleDrawingLines(), m_foreground);
}

void GLView::resizeGL(int w, int h)
{
    m_screenWidth = w;
    m_screenHeight = h;
	//触发修改进度条长度

    m_screenRatio = GLfloat(w) / h;
    recalcView();
}

void GLView::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_wasPanning) {
        m_wasPanning = false;
        return;
    }
    QPoint mousePoint = event->pos();
    Point2f worldPoint = getWorldPoint(mousePoint);
    if (!m_pDoc.m_communicator) {
        QtRegion r;
        if(m_mouseDragRect.isNull())
        {
            r.bottom_left = worldPoint;
            r.top_right = worldPoint;
        }
        else
        {
            r.bottom_left.x = std::min(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x());
            r.bottom_left.y = std::min(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y());
            r.top_right.x = std::max(m_mouseDragRect.bottomRight().x(),m_mouseDragRect.topLeft().x());
            r.top_right.y = std::max(m_mouseDragRect.bottomRight().y(),m_mouseDragRect.topLeft().y());
        }
        bool selected = false;
        switch(m_mouseMode)
        {
        case MOUSE_MODE_NONE:
        {
            // nothing, deselect
            m_pDoc.m_meta_graph->clearSel();
            break;
        }
        case MOUSE_MODE_SELECT:
        {
            // typical selection
            Qt::KeyboardModifiers keyMods = QApplication::keyboardModifiers();
            //m_pDoc.m_meta_graph->setCurSel( r, keyMods & Qt::ShiftModifier );
			m_pDoc.m_meta_graph->setCurSel(r, keyMods & Qt::ControlModifier);
            ((MainWindow *) m_pDoc.m_mainFrame)->updateToolbar();
            break;
        }
        case MOUSE_MODE_ZOOM_IN:
        {
            if(r.width() > 0)
            {
                OnViewZoomToRegion(r);
                recalcView();
            }
            else
            {
                zoomBy(0.8f, mousePoint.x(), mousePoint.y());
            }
            break;
        }
        case MOUSE_MODE_ZOOM_OUT:
        {//缩小
            zoomBy(1.2f, mousePoint.x(), mousePoint.y());
            break;
        }
        case MOUSE_MODE_FILL_FULL:
        {
            m_pDoc.OnFillPoints( worldPoint, 0 );
            break;
        }
        case MOUSE_MODE_PENCIL:
        {
            m_pDoc.m_meta_graph->getDisplayedPointMap().fillPoint(worldPoint, event->button() == Qt::LeftButton);
            break;
        }
        case MOUSE_MODE_SEED_ISOVIST:
        {
            m_pDoc.OnMakeIsovist( worldPoint );
            break;
        }
        case MOUSE_MODE_SEED_TARGETED_ISOVIST:
        {
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_mouseMode = MOUSE_MODE_SEED_TARGETED_ISOVIST | MOUSE_MODE_SECOND_POINT;
            break;
        }
        case MOUSE_MODE_SEED_TARGETED_ISOVIST | MOUSE_MODE_SECOND_POINT:
        {
            Line directionLine(m_tempFirstPoint,worldPoint);
            Point2f vec = directionLine.vector();
            vec.normalise();
            m_pDoc.OnMakeIsovist( m_tempFirstPoint, vec.angle() );
            m_mouseMode = MOUSE_MODE_SEED_TARGETED_ISOVIST;
            break;
        }
        case MOUSE_MODE_SEED_AXIAL:
        {
            m_pDoc.OnToolsAxialMap( worldPoint );
            break;
        }
        case MOUSE_MODE_LINE_TOOL:
        {
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_mouseMode = MOUSE_MODE_LINE_TOOL | MOUSE_MODE_SECOND_POINT;
            break;
        }
        case MOUSE_MODE_LINE_TOOL | MOUSE_MODE_SECOND_POINT:
        {
            if (m_pDoc.m_meta_graph->makeShape(Line(m_tempFirstPoint,worldPoint))) {
               m_pDoc.modifiedFlag = true;
               m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL, QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA );
            }
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_mouseMode = MOUSE_MODE_LINE_TOOL;

            break;
        }
        case MOUSE_MODE_POLYGON_TOOL:
        {
            m_tempFirstPoint = worldPoint;
            m_tempSecondPoint = worldPoint;
            m_polyPoints = 0;
            m_mouseMode = MOUSE_MODE_POLYGON_TOOL | MOUSE_MODE_SECOND_POINT;
            break;
        }
        case MOUSE_MODE_POLYGON_TOOL | MOUSE_MODE_SECOND_POINT:
        {
            if (m_polyPoints == 0) {
               m_currentlyEditingShapeRef = m_pDoc.m_meta_graph->polyBegin(Line(m_tempFirstPoint,worldPoint));
               m_polyStart = m_tempFirstPoint;
               m_tempFirstPoint = m_tempSecondPoint;
               m_polyPoints += 2;
            }
            else if (m_polyPoints > 2 && PixelDist(mousePoint, getScreenPoint(m_polyStart)) < 6) {
               // check to see if it's back to the original start point, if so, close off
               m_pDoc.m_meta_graph->polyClose(m_currentlyEditingShapeRef);
               m_polyPoints = 0;
               m_currentlyEditingShapeRef = -1;
               m_mouseMode = MOUSE_MODE_POLYGON_TOOL;
            }
            else {
               m_pDoc.m_meta_graph->polyAppend(m_currentlyEditingShapeRef, worldPoint);
               m_tempFirstPoint = m_tempSecondPoint;
               m_polyPoints += 1;
            }
            break;
        }
        case MOUSE_MODE_POINT_STEP_DEPTH:
        {
            m_pDoc.m_meta_graph->setCurSel( r, false );
            m_pDoc.OnToolsPD();
            break;
        }
        case MOUSE_MODE_JOIN:
        {
            selected = m_pDoc.m_meta_graph->setCurSel( r, false );
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if(selectedCount > 0) {
                Point2f selectionCentre;
                if(selectedCount > 1) {
                    QtRegion selBounds = m_pDoc.m_meta_graph->getSelBounds();
                    selectionCentre.x = (selBounds.bottom_left.x + selBounds.top_right.x)*0.5;
                    selectionCentre.y = (selBounds.bottom_left.y + selBounds.top_right.y)*0.5;
                } else {
                    const std::set<int> &selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
                        selectionCentre = m_pDoc.m_meta_graph->getDisplayedPointMap().depixelate(*selectedSet.begin());
                    } else if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                        selectionCentre = m_pDoc.m_meta_graph->getDisplayedShapeGraph().getAllShapes()[*selectedSet.begin()].getCentroid();
                    }
                }
                m_tempFirstPoint = selectionCentre;
                m_tempSecondPoint = selectionCentre;
                m_mouseMode = MOUSE_MODE_JOIN | MOUSE_MODE_SECOND_POINT;
            }
            break;
        }
        case MOUSE_MODE_JOIN | MOUSE_MODE_SECOND_POINT:
        {
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if (selectedCount > 0) {
                if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
                    m_pDoc.m_meta_graph->getDisplayedPointMap().mergePoints( worldPoint );
                } else if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL && selectedCount == 1) {
                    m_pDoc.m_meta_graph->setCurSel( r, true ); // add the new one to the selection set
                    const auto& selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    if (selectedSet.size() == 2) {
                        std::set<int>::iterator it = selectedSet.begin();
                        int axRef1 = *it;
                        it++;
                        int axRef2 = *it;
                        // axial is only joined one-by-one
                        m_pDoc.modifiedFlag = true;
                        m_pDoc.m_meta_graph->getDisplayedShapeGraph().linkShapes(axRef1, axRef2, true);
                        m_pDoc.m_meta_graph->clearSel();
                    }
                }
                m_pDoc.m_meta_graph->clearSel();
                m_mouseMode = MOUSE_MODE_JOIN;
            }
            break;
        }
        case MOUSE_MODE_UNJOIN:
        {
            m_pDoc.m_meta_graph->setCurSel( r, false );
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if(selectedCount > 0) {
                if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
                    if (m_pDoc.m_meta_graph->getDisplayedPointMap().unmergePoints()) {
                        m_pDoc.modifiedFlag = true;
                        m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_GRAPH, QGraphDoc::NEW_DATA);
                    }
                } else if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
                    const auto& selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    Point2f selectionCentre = m_pDoc.m_meta_graph->getDisplayedShapeGraph().getAllShapes()[*selectedSet.begin()].getCentroid();
                    m_tempFirstPoint = selectionCentre;
                    m_tempSecondPoint = selectionCentre;
                    m_mouseMode = MOUSE_MODE_UNJOIN | MOUSE_MODE_SECOND_POINT;
                }
            }
            break;
        }
        case MOUSE_MODE_UNJOIN | MOUSE_MODE_SECOND_POINT:
        {
            int selectedCount = m_pDoc.m_meta_graph->getSelCount();
            if (selectedCount > 0) {
                if (m_pDoc.m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL && selectedCount == 1) {
                    m_pDoc.m_meta_graph->setCurSel( r, true ); // add the new one to the selection set
                    const auto& selectedSet = m_pDoc.m_meta_graph->getSelSet();
                    if (selectedSet.size() == 2) {
                        std::set<int>::iterator it = selectedSet.begin();
                        int axRef1 = *it;
                        it++;
                        int axRef2 = *it;
                        // axial is only joined one-by-one
                        m_pDoc.modifiedFlag = true;
                        m_pDoc.m_meta_graph->getDisplayedShapeGraph().unlinkShapes(axRef1, axRef2, true);
                        m_pDoc.m_meta_graph->clearSel();
                    }
                }
                m_pDoc.m_meta_graph->clearSel();
                m_mouseMode = MOUSE_MODE_UNJOIN;
            }
            break;
        }
        }

        m_pDoc.SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION);
    }
    m_mouseDragRect.setWidth(0);
    m_mouseDragRect.setHeight(0);
}

void GLView::mousePressEvent(QMouseEvent *event)
{
    m_mouseLastPos = event->pos();
	//m_pDoc.UpdateLinesID();
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_mouseLastPos.x();
    int dy = event->y() - m_mouseLastPos.y();

    Point2f worldPoint = getWorldPoint(event->pos());

    if (event->buttons() & Qt::RightButton
            || (event->buttons() & Qt::LeftButton && m_mouseMode == MOUSE_MODE_PAN)) {
        panBy(dx, dy);
        m_wasPanning = true;
    } else if (event->buttons() & Qt::LeftButton) {
        Point2f lastWorldPoint = getWorldPoint(m_mouseLastPos);

        if(m_mouseDragRect.isNull()) {
            m_mouseDragRect.setX(lastWorldPoint.x);
            m_mouseDragRect.setY(lastWorldPoint.y);
        }
        m_mouseDragRect.setWidth(worldPoint.x - m_mouseDragRect.x());
        m_mouseDragRect.setHeight(worldPoint.y - m_mouseDragRect.y());
        update();
    }
    if((m_mouseMode & MOUSE_MODE_SECOND_POINT) == MOUSE_MODE_SECOND_POINT)
    {
        m_tempSecondPoint = worldPoint;
        update();
    }
    m_mouseLastPos = event->pos();
    m_pDoc.m_position = worldPoint;
    m_pDoc.UpdateMainframestatus();
}

void GLView::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;

    int x = static_cast<int>(event->position().x());
    int y = static_cast<int>(event->position().y());

    zoomBy(1 - 0.25f * numDegrees.y() / 15.0f, x, y);

    event->accept();
}

inline double round(double number, unsigned int bits)
{
	double integerpart = floor(number);
	number -= integerpart;
	for (unsigned int i = 0; i < bits; ++i)
	{
		number *= 10;
	}
	number = floor(number + 0.5);
	for (unsigned int i = 0; i < bits; ++i)
	{
		number /= 10;
	}
	return integerpart + number;
}

bool GLView::eventFilter(QObject *object, QEvent *e)
{
    if(e->type() == QEvent::ToolTip)
    {
        if (!m_pDoc.m_communicator)
        {
            if(m_pDoc.m_meta_graph)
            {
                if (m_pDoc.m_meta_graph->viewingProcessed() && m_pDoc.m_meta_graph->getSelCount() > 1) {
                    float val = m_pDoc.m_meta_graph->getSelAvg();
                    int count = m_pDoc.m_meta_graph->getSelCount();
                    if (val == -1.0f)
                        setToolTip("Null selection");
                    else if (val != -2.0f)
                        setToolTip(QString("Selection\nAverage: %1\nCount: %2").arg(val).arg(count));
                    else setToolTip("");
                }
                else if (m_pDoc.m_meta_graph->viewingProcessed()) {
                    // and that it has an appropriate state to display a hover wnd
                    QHelpEvent *helpEvent = static_cast<QHelpEvent*>(e); // Tool tip events come as the type QHelpEvent
                    float val = m_pDoc.m_meta_graph->getLocationValue(getWorldPoint(helpEvent->pos()));
                    if (val == -1.0f)
                        setToolTip("No value");
                    else if (val != -2.0f) {
                        QString s;
                        QTextStream txt(&s);
                        txt.setRealNumberNotation(QTextStream::FixedNotation);
						if (val == int(val))
							txt << int(val);
						else
							txt << qSetRealNumberPrecision(4) << val;
                        setToolTip(s);
                    }
                    else setToolTip("");
                }
            }
        }
    }

    return QObject::eventFilter(object, e);
}
//滚轮缩放时调用
void GLView::zoomBy(float dzf, int mouseX, int mouseY)
{
    float pzf = m_zoomFactor;
    m_zoomFactor = m_zoomFactor * dzf;
    if(m_zoomFactor < m_minZoomFactor) m_zoomFactor = m_minZoomFactor;
    else if(m_zoomFactor > m_maxZoomFactor) m_zoomFactor = m_maxZoomFactor;
    m_eyePosX += (m_zoomFactor - pzf) * m_screenRatio * GLfloat(mouseX - m_screenWidth*0.5f) / GLfloat(m_screenWidth);
    m_eyePosY -= (m_zoomFactor - pzf) * GLfloat(mouseY - m_screenHeight*0.5f) / GLfloat(m_screenHeight);
    recalcView();
}
void GLView::panBy(int dx, int dy)
{
    m_eyePosX += m_zoomFactor * GLfloat(dx) / m_screenHeight;
    m_eyePosY -= m_zoomFactor * GLfloat(dy) / m_screenHeight;

    recalcView();
}
void GLView::recalcView()
{
    m_mProj.setToIdentity();

    if(m_perspectiveView)
    {
        m_mProj.perspective(45.0f, m_screenRatio, 0.01f, 100.0f);
        m_mProj.scale(1.0f, 1.0f, m_zoomFactor);
    }
    else
    {
        m_mProj.ortho(-m_zoomFactor * 0.5f * m_screenRatio, m_zoomFactor * 0.5f * m_screenRatio, -m_zoomFactor * 0.5f, m_zoomFactor * 0.5f, 0, 10);
    }
    m_mProj.translate(m_eyePosX, m_eyePosY, 0.0f);
    update();
}

Point2f GLView::getWorldPoint(const QPoint &screenPoint) {
    return Point2f(+ m_zoomFactor * float(screenPoint.x() - m_screenWidth*0.5)  / m_screenHeight - m_eyePosX,
                   - m_zoomFactor * float(screenPoint.y() - m_screenHeight*0.5) / m_screenHeight - m_eyePosY);
}

QPoint GLView::getScreenPoint(const Point2f &worldPoint) {
    return QPoint((worldPoint.x + m_eyePosX) * m_screenHeight / m_zoomFactor + m_screenWidth*0.5 ,
                   - (worldPoint.y + m_eyePosY) * m_screenHeight / m_zoomFactor + m_screenHeight*0.5);

}

void GLView::matchViewToCurrentMetaGraph() {
    const QtRegion &region = m_pDoc.m_meta_graph->getBoundingBox();
    OnViewZoomToRegion(region);
    recalcView();
}

void GLView::OnViewZoomToRegion(QtRegion region) {
    if((region.top_right.x == 0 && region.bottom_left.x == 0)
            || (region.top_right.y == 0 && region.bottom_left.y == 0))
        // region is unset, don't try to change the view to it
        return;
    m_eyePosX = - (region.top_right.x + region.bottom_left.x)*0.5f;
    m_eyePosY = - (region.top_right.y + region.bottom_left.y)*0.5f;
    if(region.width() > region.height())
    {
        m_zoomFactor = region.top_right.x - region.bottom_left.x;
    }
    else
    {
        m_zoomFactor = region.top_right.y - region.bottom_left.y;
    }
    m_minZoomFactor = m_zoomFactor * 0.001;
    m_maxZoomFactor = m_zoomFactor * 10;
}

void GLView::resetView() {
    m_visiblePointMap.showLinks(false);
    m_visibleShapeGraph.showLinks(false);
    m_pDoc.m_meta_graph->clearSel();
    update();
}

void GLView::OnModeJoin()
{
    if (m_pDoc.m_meta_graph->getViewClass() & (MetaGraph::VIEWVGA | MetaGraph::VIEWAXIAL)) {
        resetView();
        m_mouseMode = MOUSE_MODE_JOIN;
        m_visiblePointMap.showLinks(true);
        m_visibleShapeGraph.showLinks(true);
        m_pDoc.m_meta_graph->clearSel();
        notifyDatasetChanged();
    }
}

void GLView::OnModeUnjoin()
{
    if (m_pDoc.m_meta_graph->getState() & (MetaGraph::VIEWVGA | MetaGraph::VIEWAXIAL)) {
        resetView();
        m_mouseMode = MOUSE_MODE_UNJOIN;
        m_visiblePointMap.showLinks(true);
        m_visibleShapeGraph.showLinks(true);
        m_pDoc.m_meta_graph->clearSel();
        notifyDatasetChanged();
    }
}
void GLView::OnViewPan()
{
    m_mouseMode = MOUSE_MODE_PAN;
	SetCursor();
}

void GLView::OnViewZoomIn()
{
    m_mouseMode = MOUSE_MODE_ZOOM_IN;
	SetCursor();
}

void GLView::OnViewZoomOut()
{
    m_mouseMode = MOUSE_MODE_ZOOM_OUT;
	SetCursor();
}

void GLView::OnEditFill()
{
    resetView();
    m_mouseMode = MOUSE_MODE_FILL_FULL;
	SetCursor();
}

void GLView::OnEditSemiFill()
{
    resetView();
    m_mouseMode = MOUSE_MODE_FILL_SEMI;
	SetCursor();
}

void GLView::OnEditAugmentFill()
{
    resetView();
    m_mouseMode = MOUSE_MODE_FILL_AUGMENT;
	SetCursor();
}

void GLView::OnEditPencil()
{
    resetView();
    m_mouseMode = MOUSE_MODE_PENCIL;
	SetCursor();
}

void GLView::OnModeIsovist()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SEED_ISOVIST;
	SetCursor();
}

void GLView::OnModeTargetedIsovist()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SEED_TARGETED_ISOVIST;
	SetCursor();
}

void GLView::OnModeSeedAxial()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SEED_AXIAL;
	SetCursor();
}

void GLView::OnModeStepDepth()
{
    resetView();
    m_mouseMode = MOUSE_MODE_POINT_STEP_DEPTH;
	SetCursor();
}

void GLView::OnEditLineTool()
{
    resetView();
    m_mouseMode = MOUSE_MODE_LINE_TOOL;
	SetCursor();
}

void GLView::OnEditPolygonTool()
{
    resetView();
    m_mouseMode = MOUSE_MODE_POLYGON_TOOL;
	SetCursor();
}

void GLView::OnEditSelect()
{
    resetView();
    m_mouseMode = MOUSE_MODE_SELECT;
	SetCursor();
}

void GLView::SetCursor() {
	switch (m_mouseMode)
	{
	case MOUSE_MODE_SELECT:
		m_cursor = QCursor(Qt::ArrowCursor);
		break;
	case MOUSE_MODE_PAN:
		m_cursor = QCursor(QPixmap("ico/view_icons/cur00007.png"));
		break;
	case MOUSE_MODE_ZOOM_IN:
		m_cursor = QCursor(QPixmap("ico/view_icons/cur00008.png"));
		break;
	case MOUSE_MODE_ZOOM_OUT:
		m_cursor = QCursor(QPixmap("ico/view_icons/cur00009.png"));
		break;
	default:
		m_cursor = QCursor(Qt::ArrowCursor);
		break;
	}
	setCursor(m_cursor);
}

void GLView::postLoadFile()
{
    matchViewToCurrentMetaGraph();
    //setWindowTitle(m_pDoc.m_base_title+":Map View (GL)");
	setWindowTitle(m_pDoc.m_base_title);
}

void GLView::OnViewZoomsel()
{
   if (m_pDoc.m_meta_graph && m_pDoc.m_meta_graph->isSelected()) {
      OnViewZoomToRegion(m_pDoc.m_meta_graph->getSelBounds());
   }
}

void GLView::closeEvent(QCloseEvent *event)
{
    m_pDoc.m_view[QGraphDoc::VIEW_MAP_GL] = NULL;
    if (!m_pDoc.OnCloseDocument(QGraphDoc::VIEW_MAP_GL))
    {
        m_pDoc.m_view[QGraphDoc::VIEW_MAP_GL] = this;
        event->ignore();
    }
}

void GLView::OnEditCopy()
{
    std::unique_ptr<QDepthmapView> tmp(new QDepthmapView(m_pDoc, m_settings));
    Point2f topLeftWorld = getWorldPoint(QPoint(0,0));
    Point2f bottomRightWorld = getWorldPoint(QPoint(width(),height()));

    tmp->setAttribute(Qt::WA_DontShowOnScreen);
    tmp->show();
    tmp->postLoadFile();
    tmp->OnViewZoomToRegion(QtRegion(topLeftWorld, bottomRightWorld));
    tmp->repaint();
    tmp->OnEditCopy();
    tmp->close();
}

void GLView::OnEditSave()
{
    m_unit = (m_zoomFactor * 0.001);
    m_centre = getWorldPoint(QPoint(m_screenWidth / 2, m_screenHeight / 2));
    QString saveas;
    QFilePath path(windowFilePath());
    saveas = path.m_path + (path.m_name.isEmpty() ? windowTitle() : path.m_name);
    saveas = saveas.mid(0, saveas.length() - 4);
    extern QString data_map_name;

    QString template_string = tr("Portable Network Graphics (*.png)\nScalable Vector Graphics (*.svg)");
    QSettings settings("SZU", "URCONNECT");
    QString lastDirectory = settings.value("lastDirectory", QDir::homePath()).toString();
	QFileDialog::Options options;
    QString selectedFilter;
    QString outfile = QFileDialog::getSaveFileName(
        this, tr("Save Screen As"),
        lastDirectory + "/" + saveas + ".png",
        template_string,
        &selectedFilter,
        options);

    if (outfile.isEmpty()) return;

    QFileInfo fileInfo(outfile);
    QString ext = fileInfo.suffix().toLower();

    if (ext != "svg" && ext != "png") {
        QMessageBox::warning(this, tr("Unsupported File Type"),
            tr("The file type ") + ext + tr(" is not supported."), QMessageBox::Ok);
        return;
    }

    QFile file(outfile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Warning"), tr("Sorry, unable to open ") + outfile + tr(" for writing"), QMessageBox::Ok);
        return;
    }

    if (ext == "svg") {
        OutputSVG(&file, &m_pDoc);
    } else if (ext == "png") {
        OutputPNGbyScreenShot(&file, &m_pDoc);
    }
    file.close();

    // Update the last directory setting
    settings.setValue("lastDirectory", fileInfo.absoluteDir().absolutePath());
}

QtRegion GLView::LogicalViewport(const QRect& phys_bounds, QGraphDoc *pDoc)
{
	m_physical_centre = QSize(phys_bounds.width() / 2, phys_bounds.height() / 2);
	/*return QtRegion(LogicalUnits(QPoint(phys_bounds.left(), phys_bounds.bottom())),
		LogicalUnits(QPoint(phys_bounds.right(), phys_bounds.top())));*/
	return QtRegion(getWorldPoint(QPoint(phys_bounds.left(), phys_bounds.bottom())),
		getWorldPoint(QPoint(phys_bounds.right(), phys_bounds.top())));
	
}

int GLView::GetSpacer(QGraphDoc *pDoc)
{
	int spacer = 1;
	int viewclass = pDoc->m_meta_graph->getViewClass();
	if (viewclass & (MetaGraph::VIEWVGA | MetaGraph::VIEWBACKVGA)) {
		spacer = int(ceil(5.0 * pDoc->m_meta_graph->getDisplayedPointMap().getSpacing() / (m_unit * 10.0)));
	}
	else if (viewclass & MetaGraph::VIEWAXIAL) {
		spacer = int(ceil(pDoc->m_meta_graph->getDisplayedShapeGraph().getSpacing() / (m_unit * 10.0)));
	}
	else if (viewclass & MetaGraph::VIEWDATA) {
		spacer = int(ceil(pDoc->m_meta_graph->getDisplayedDataMap().getSpacing() / (m_unit * 10.0)));
	}
	return spacer;
}

static std::string SVGColor(PafColor color)
{
	std::stringstream text;
	int r = color.redb();
	int g = color.greenb();
	int b = color.blueb();
	text << std::setfill('0') << "#" << std::setw(2) << std::hex << r << std::setw(2) << std::hex << g << std::setw(2) << std::hex << b;
	return text.str();
}

static QPoint SVGPhysicalUnits(const Point2f& p, const QtRegion& r, int h)
{
	// converts to a 4800 unit wide QtRegion
	return QPoint(
		int(4800.0 * ((p.x - r.bottom_left.x) / r.width())), h - int(4800.0 * ((p.y - r.bottom_left.y) / r.width()))
	);
}

void GLView::OutputPNGbyScreenShot(QFile* file, QGraphDoc* pDoc) {
	if (!m_viewport_set) {
		QMessageBox::warning(this, tr("Depthmap"), tr("Can't save screen as the Depthmap window is not initialised"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	// 获取窗口的矩形区域
	QRect rect = this->rect(); // 使用窗口的矩形区域

	// 截取窗口内容
	QPixmap pixmap = this->grab();

	// 确保文件以二进制写入模式打开
	if (file->isOpen()) {
		// 将 QPixmap 保存到文件
		bool success = pixmap.save(file, "PNG");
		if (success) {
			qDebug() << "Success : File saved successfully.";
		}
		else {
			qDebug() << "Failed : Unable to save file.";
		}
	}
	else {
		qDebug() << "Failed : Unable to open file for writing.";
	}
}
void GLView::OutputPNG(const QString& filename, QGraphDoc* pDoc) {
	if (!m_viewport_set) {
		QMessageBox::warning(this, tr("Depthmap"), tr("Can't save screen as the Depthmap window is not initialised"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	QRect rect = QRect(0, 0, width(), height());

	int h = (4800 * rect.height()) / rect.width();
	QSize size(4800, h);
	QImage image(size, QImage::Format_ARGB32_Premultiplied);
	image.fill(m_background); // 假设背景色已在成员变量中定义

	QPainter painter(&image);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

	// 重用视图转换逻辑
	QtRegion logicalviewport = LogicalViewport(rect, pDoc);

	// 根据图形状态绘制元素
	int state = pDoc->m_meta_graph->getState();

	if (state & MetaGraph::POINTMAPS) {
		pDoc->m_meta_graph->getDisplayedPointMap().setScreenPixel(m_unit); // only used by points (at the moment!)
		pDoc->m_meta_graph->getDisplayedPointMap().makeViewportPoints(logicalviewport);
	}
	if (state & MetaGraph::SHAPEGRAPHS) {
		pDoc->m_meta_graph->getDisplayedShapeGraph().makeViewportShapes(logicalviewport);
	}
	if (state & MetaGraph::DATAMAPS) {
		pDoc->m_meta_graph->getDisplayedDataMap().makeViewportShapes(logicalviewport);
	}
	if (state & MetaGraph::LINEDATA) {
		pDoc->m_meta_graph->makeViewportShapes(logicalviewport);
	}

	if (state & MetaGraph::POINTMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {
		PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();
		double spacing = map.getSpacing();
		double spacer = 4800.0 * (spacing / logicalviewport.width()) / 2.0;
		//painter.setRenderHint(QPainter::Antialiasing);

		while (map.findNextPoint()) {
			Point2f logical = map.getNextPointLocation();
			PafColor color = map.getCurrentPointColor();

			if (color.alphab() != 0) { // alpha == 0 is transparent
				QPoint p = SVGPhysicalUnits(logical, logicalviewport, h);
				painter.setBrush(QColor(QString::fromLocal8Bit((SVGColor(color).c_str()))));
				painter.drawRect(p.x() - spacer, p.y() - spacer, 2 * spacer, 2 * spacer);
			}
		}

	}
	if (state & MetaGraph::SHAPEGRAPHS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {
		ShapeMap& map = pDoc->m_meta_graph->getDisplayedShapeGraph();
		OutputPNGMap(painter, map, logicalviewport,h);
	}
	if (state & MetaGraph::DATAMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {
		ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();
		OutputPNGMap(painter, map, logicalviewport,h);
	}
	// ... 其他图形状态的处理
	// 保存图像
	painter.save();
	if (!image.save(filename, "PNG")) {
		QMessageBox::warning(this, tr("PNG Save Error"), tr("Failed to save the image as PNG."), QMessageBox::Ok, QMessageBox::Ok);
	}
	painter.end();


}

void GLView::OutputPNGLine(QPainter& painter, SimpleLine& sim_line, QtRegion& logicalviewport, int h) {

    painter.setRenderHint(QPainter::Antialiasing);

    Line line(sim_line.start(), sim_line.end());
    if (line.crop(logicalviewport)) {
		QPoint start = SVGPhysicalUnits(line.start(), logicalviewport, h);
		QPoint end = SVGPhysicalUnits(line.end(), logicalviewport, h); 
        // 2.0 pixels is the minimum length to draw the line in the PNG output
        if (qAbs(start.x() - end.x()) > 2 || qAbs(start.y() - end.y()) > 2) {
            painter.drawLine(start, end);
        }
    }
}

void GLView::OutputPNGMap(QPainter& painter, ShapeMap& map, QtRegion& logicalviewport, int h) {

    painter.setRenderHint(QPainter::Antialiasing);

    // 获取显示参数
    bool showlines, showfill, showcentroids;
    map.getPolygonDisplay(showlines, showfill, showcentroids);

    if (!showlines) {
        return; // 如果不显示线条，则直接返回
    }

    // 假设monochrome目前没有实现，如果map是单色的，则不进行绘制
    bool monochrome = (map.getDisplayParams().colorscale == DisplayParams::MONOCHROME);
    if (monochrome) {
        return;
    }

    std::vector<std::pair<SimpleLine, PafColor>> shapes = map.getAllLinesWithColour();
    PafColor oldcolor;

    for (const auto& shape : shapes) {
        SimpleLine line = shape.first;
        PafColor color = shape.second;

        if (color != oldcolor) {
            // 设置新的颜色
            painter.setPen(QPen(QColor(color.redb(), color.greenb(), color.blueb()), 4)); // 假设stroke-width为4
            oldcolor = color;
        }

        // 绘制线段
        OutputPNGLine(painter, line, logicalviewport, h);
    }
}

// void GLView::OutputPNGPoly(QPainter& painter, const SalaShape& shape, QtRegion& logicalviewport, int h) {

//     painter.setRenderHint(QPainter::Antialiasing);

//     //// 辅助函数：将逻辑坐标转换为QImage的像素坐标
//     //auto SVGPhysicalUnits = [&](const Point2f& point) {
//     //    return SVGPhysicalUnits(point, logicalviewport, h);
//     //};

//     // 辅助函数：计算两个点之间的距离
//     auto dist = [](const Point2f& p1, const Point2f& p2) {
//         return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
//     };

//     QPoint bl = SVGPhysicalUnits(shape.getBoundingBox().bottom_left);
//     QPoint tr = SVGPhysicalUnits(shape.getBoundingBox().top_right);
//     if (dist(Point2f(bl.x(), bl.y()), Point2f(tr.x(), tr.y())) < 2.0f) {
//         return;
//     }

//     std::vector<QPoint> points;
//     if (shape.isOpen()) {
//         // 绘制开放形状
//         Point2f lastPoint = shape.m_points.front();
//         for (size_t i = 1; i < shape.m_points.size(); ++i) {
//             Line line(lastPoint, SVGPhysicalUnits(shape.m_points[i]));
//             if (line.crop(logicalviewport)) {
//                 QPoint currentPoint = SVGPhysicalUnits(line.t_end());
//                 if (dist(Point2f(lastPoint.x(), lastPoint.y()), Point2f(currentPoint.x(), currentPoint.y())) >= 2.0f) {
//                     painter.drawLine(lastPoint, currentPoint);
//                 }
//             }
//             lastPoint = currentPoint;
//         }
//     } else {
//         // 绘制闭合形状
//         std::vector<SalaEdgeU> eus = shape.getClippingSet(logicalviewport);
//         if (eus.size() >= 2) {
//             int entry = eus[0].entry ? 0 : 1;
//             while (entry < (int)eus.size()) {
//                 int exit = (entry + 1) % eus.size();
//                 QPoint lastPoint = SVGPhysicalUnits(logicalviewport.getEdgeUPoint(eus[entry]));
//                 points.push_back(lastPoint);

//                 for (size_t i = eus[entry].index + 1; i != eus[exit].index; ++i) {
//                     QPoint currentPoint = SVGPhysicalUnits(shape.getPointAt(i % shape.m_points.size()));
//                     if (dist(Point2f(lastPoint.x(), lastPoint.y()), Point2f(currentPoint.x(), currentPoint.y())) >= 2.0f) {
//                         painter.drawLine(lastPoint, currentPoint);
//                         points.push_back(currentPoint);
//                     }
//                     lastPoint = currentPoint;
//                 }

//                 QPoint exitPoint = SVGPhysicalUnits(logicalviewport.getEdgeUPoint(eus[exit]));
//                 painter.drawLine(lastPoint, exitPoint);
//                 points.push_back(exitPoint);

//                 if (ccwEdgeU(eus[entry], eus[entry + 1], eus[entry + 2]) != shape.isCCW()) {
//                     // 如果方向不一致，中断当前多边形的绘制，并开始一个新的多边形
//                     painter.drawPolygon(points.data(), points.size());
//                     points.clear();
//                 }

//                 entry += 2;
//             }
//         }
//     }

//     if (!points.empty()) {
//         painter.drawPolygon(points.data(), points.size());
//     }
// }

void GLView::OutputSVG(QFile* file, QGraphDoc* pDoc) {
	if (!file->isOpen()) {
		qDebug() << "File is not open for writing.";
		return;
	}

	QTextStream stream(file);
	if (!stream.status() == QTextStream::Ok) {
		qDebug() << "Unable to write to file.";
		return;
	}
	if (!m_viewport_set) {
		QMessageBox::warning(this, tr("Depthmap"), tr("Can't save screen as the Depthmap window is not initialised"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	QRect rect = QRect(0, 0, width(), height());

	// we'll make this 24cm wide whatever, and base the height on it:
	int h = (4800 * rect.height()) / rect.width();

	stream << "<?xml version=\"1.0\" standalone=\"no\"?>\n";
	stream << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n";
	stream << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
	stream << "<svg width=\"24cm\" height=\"" << (h / 200) << "cm\" viewBox=\"0 0 4800 " << h << "\"\n";
	stream << "xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\">\n";
	stream << "<desc>" << TITLE_BASE << "</desc>\n";

	// note, SVG draw completely overrides standard draw physical units to achieve hi-res output
	// (EPS should probably follow this model too)
	QtRegion logicalviewport = LogicalViewport(rect, pDoc);

	stream << "<rect x=\"0\" y=\"0\" width=\"4800\" height=\"" << h << "\" "
		<< "fill=\"" << QString::fromLocal8Bit(SVGColor(m_background).c_str()) << "\" stroke=\"none\" stroke-width=\"0\" />\n";

	int state = pDoc->m_meta_graph->getState();

	if (state & MetaGraph::POINTMAPS) {
		pDoc->m_meta_graph->getDisplayedPointMap().setScreenPixel(m_unit); // only used by points (at the moment!)
		pDoc->m_meta_graph->getDisplayedPointMap().makeViewportPoints(logicalviewport);
	}
	if (state & MetaGraph::SHAPEGRAPHS) {
		pDoc->m_meta_graph->getDisplayedShapeGraph().makeViewportShapes(logicalviewport);
	}
	if (state & MetaGraph::DATAMAPS) {
		pDoc->m_meta_graph->getDisplayedDataMap().makeViewportShapes(logicalviewport);
	}
	if (state & MetaGraph::LINEDATA) {
		pDoc->m_meta_graph->makeViewportShapes(logicalviewport);
	}

	if (state & MetaGraph::POINTMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {

		PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();

		double spacing = map.getSpacing();
		double spacer = 4800.0 * (spacing / logicalviewport.width()) / 2.0;

		stream << "<g stroke=\"none\">\n";

		stream << "<defs><rect id=\"a\" width=\"" << 2 * spacer << "\" height=\"" << 2 * spacer << "\" /></defs>\n";

		while (map.findNextPoint()) {

			Point2f logical = map.getNextPointLocation();
			PafColor color = map.getCurrentPointColor();

			if (color.alphab() != 0) { // alpha == 0 is transparent

				QPoint p = SVGPhysicalUnits(logical, logicalviewport, h);

				stream << "<use fill=\"" << QString::fromLocal8Bit(SVGColor(color).c_str()) << "\" x=\"" << p.x() - spacer << "\" y=\"" << p.y() - spacer << "\" xlink:href=\"#a\" />\n";
			}
		}
		stream << "</g>\n";
	}

	if (state & MetaGraph::SHAPEGRAPHS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {

		ShapeMap& map = pDoc->m_meta_graph->getDisplayedShapeGraph();

		OutputSVGMap(stream, map, logicalviewport, h);
	}

	if (state & MetaGraph::DATAMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {

		ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();

		OutputSVGMap(stream, map, logicalviewport, h);
	}


	stream << "</svg>\n";
}

void GLView::OutputSVGMap(QTextStream& stream, ShapeMap& map, QtRegion& logicalviewport, int h)
{
	bool monochrome = (map.getDisplayParams().colorscale == DisplayParams::MONOCHROME);
	// monochrome not implemented yet!
	if (monochrome) {
		QMessageBox::warning(this, tr("Depthmap"), tr("This map is displaying in monochrome, which is not yet supported.  Please note your SVG file will be empty.  Sorry for the inconvenience."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	// I haven't implemented all of these, but I hope I will get there:
	bool showlines, showfill, showcentroids;
	map.getPolygonDisplay(showlines, showfill, showcentroids);

	// arbitrary stroke width for now
	stream << "<g stroke-width=\"4\">\n";

	PafColor color, oldcolor;


	std::vector<std::pair<SimpleLine, PafColor>> shapes = map.getAllLinesWithColour();

	bool first = true;
	for (int i = 0; i < shapes.size(); i++) {
		color = shapes[i].second;

		if (first || color != oldcolor) {
			if (!first) {
				stream << "</g>\n";
			}
			else {
				first = false;
			}
			stream << "<g fill=\"none\" stroke=\"" << QString::fromLocal8Bit(SVGColor(color).c_str()) << "\">\n";
			oldcolor = color;
		}

		SimpleLine line = shapes[i].first;
		OutputSVGLine(stream, line, logicalviewport, h);
	}


	if (!first) {
		stream << "</g>\n";
	}

	stream << "</g>\n";
}

void GLView::OutputSVGLine(QTextStream& stream, SimpleLine& sim_line, QtRegion& logicalviewport, int h)
{
	bool drewit = false;
	Line line(sim_line.start(), sim_line.end());
	if (line.crop(logicalviewport)) {
		QPoint start = SVGPhysicalUnits(line.start(), logicalviewport, h);
		QPoint end = SVGPhysicalUnits(line.end(), logicalviewport, h);
		// 2.0 is about 0.1mm in a standard SVG output size
		if (dist(Point2f(start.x(), start.y()), Point2f(end.x(), end.y())) >= 2.0f) {
			stream << "<line x1=\"" << start.x() << "\" y1=\"" << start.y() << "\""
				<< " x2=\"" << end.x() << "\" y2=\"" << end.y() << "\" />\n";
		}
	}
}

void GLView::OutputSVGPoly(QTextStream& stream, const SalaShape& shape, QtRegion& logicalviewport, int h)
{
	QPoint bl = SVGPhysicalUnits(shape.getBoundingBox().bottom_left, logicalviewport, h);
	QPoint tr = SVGPhysicalUnits(shape.getBoundingBox().top_right, logicalviewport, h);
	if (dist(Point2f(bl.x(), bl.y()), Point2f(tr.x(), tr.y())) < 2.0f) {
		// 2.0 is about 0.1mm in standard SVG output size -- if this is too small, we won't bother trying to draw it at all
		return;
	}
	if (shape.isOpen()) {
		// open lines are fairly easy: simply chop lines as they enter and exit
		stream << "<polyline points=\"";
		bool starter = true, drawn = false;
		Point2f lastpoint = shape.m_points.front();
		auto iter = shape.m_points.begin();
		iter++;
		for (; iter != shape.m_points.end(); iter++) {
			Line line(lastpoint, *iter);
			if (line.crop(logicalviewport)) {
				// note: use t_start and t_end so that this line moves in the correct direction
				QPoint start = SVGPhysicalUnits(line.t_start(), logicalviewport, h);
				QPoint end = SVGPhysicalUnits(line.t_end(), logicalviewport, h);
				// always draw either from the first point or whenever you enter the viewport:
				if (starter) {
					stream << start.x() << "," << start.y() << " ";
					starter = false;
				}
				// 2.0 is about 0.1mm in a standard SVG output size
				if (dist(Point2f(start.x(), start.y()), Point2f(end.x(), end.y())) >= 2.0f || iter == shape.m_points.end() - 1) {
					// also, always draw the very last point regardless of distance
					stream << end.x() << "," << end.y() << " ";
					drawn = true;
				}
			}
			else {
				starter = true;
				drawn = true;
			}
			if (drawn) {
				lastpoint = *iter;
				drawn = false;
			}
		}
		stream << "\" />\n";
	}
	else {
		// polygons are hard... have to work out entry and exit points to the clipping frame
		// and wind according to their direction
		stream << "<polygon points=\"";
		std::vector<SalaEdgeU> eus = shape.getClippingSet(logicalviewport);
		if (eus.size() == 0) {
			// this should be a shape that is entirely within the viewport:
			QPoint last = SVGPhysicalUnits(shape.m_points[0], logicalviewport, h);
			stream << last.x() << "," << last.y() << " ";
			auto iter = shape.m_points.begin();
			iter++;
			for (; iter != shape.m_points.end(); iter++) {
				QPoint next = SVGPhysicalUnits(*iter, logicalviewport, h);
				if (dist(Point2f(last.x(), last.y()), Point2f(next.x(), next.y())) >= 2.0f) {
					stream << next.x() << "," << next.y() << " ";
					last = next;
				}
			}
		}
		else if (eus.size() == 1) {
			// dummy: getClippingSet deliberately adds a single empty EdgeU if the polygon is completely outside the frame
			// (this can happen when a polygon wraps around the frame)
		}
		else if (eus.size() >= 2) {
			int entry = eus[0].entry ? 0 : 1;
			// this can get very messy (sometimes have to split into separate polys)... here's hoping for the best
			while (entry < (int)eus.size()) {
				int exit = int((entry + 1) % eus.size());
				Point2f pt = logicalviewport.getEdgeUPoint(eus[entry]);
				QPoint last = SVGPhysicalUnits(pt, logicalviewport, h);
				QPoint next;
				stream << last.x() << "," << last.y() << " ";
				for (size_t i = eus[entry].index + 1; i != eus[exit].index; i++) {
					if (i >= shape.m_points.size()) {
						i = 0;
					}
					next = SVGPhysicalUnits(shape.m_points[i], logicalviewport, h);
					if (dist(Point2f(last.x(), last.y()), Point2f(next.x(), next.y())) >= 2.0f) {
						stream << next.x() << "," << next.y() << " ";
						last = next;
					}
				}
				pt = logicalviewport.getEdgeUPoint(eus[exit]);
				last = SVGPhysicalUnits(pt, logicalviewport, h);
				stream << last.x() << "," << last.y() << " ";
				bool breakup = false;
				if (entry + 2 < (int)eus.size() && ccwEdgeU(eus[entry], eus[entry + 1], eus[entry + 2]) != shape.isCCW()) {
					breakup = true;
				}
				EdgeU& nextentry = breakup ? eus[entry] : eus[(exit + 1) % eus.size()];
				if (shape.isCCW()) {
					if (nextentry.edge != eus[exit].edge || nextentry.u < eus[exit].u) {
						int edge = eus[exit].edge;
						do {
							edge++;
							if (edge > 3) {
								edge = 0;
							}
							next = SVGPhysicalUnits(logicalviewport.getEdgeUPoint(EdgeU(edge, 0)), logicalviewport, h);
							stream << next.x() << "," << next.y() << " ";
						} while (edge != nextentry.edge);
					}
				}
				else {
					if (nextentry.edge != eus[exit].edge || nextentry.u > eus[exit].u) {
						int edge = eus[exit].edge;
						do {
							edge--;
							if (edge < 0) {
								edge = 3;
							}
							next = SVGPhysicalUnits(logicalviewport.getEdgeUPoint(EdgeU(edge, 1)), logicalviewport, h);
							stream << next.x() << "," << next.y() << " ";
						} while (edge != nextentry.edge);
					}
				}
				if (breakup) {
					//if (entry + 2 < eus.size() && ccwEdgeU(eus[entry],eus[entry+1],eus[entry+2]) != shape.isCCW()) {
					stream << "\" />\n";
					stream << "<polygon points=\"";
				}
				entry += 2;
			}
		}
		stream << "\" />\n";
	}
}

void GLView::OutputEPS(std::ofstream& stream, QGraphDoc *pDoc, bool includeScale)
{
	// This output EPS is a copy of the standard output... obviously, if you change
	// standard output, remember to change this one too!

	// now the two are a little out of synch

	if (!m_viewport_set) {
		QMessageBox::warning(this, tr("Warning"), tr("Can't save screen as the Depthmap window is not initialised"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	QRect clrect, rect;
	clrect = QRect(0, 0, width(), height());//GetClientRect( clrect );

	stream << "%!PS-Adobe-3.0 EPSF-3.0\n"
		<< "%%BoundingBox: 0 0 " << clrect.width() << " " << clrect.height() << "\n"
		<< "%%Creator: " << TITLE_BASE << std::endl;

	// temporarily inflate resolution for EPS draw
	rect = QRect(clrect.left() * 10, clrect.top() * 10, clrect.width() * 10, clrect.height() * 10);
	QPoint oldcentre = QPoint(m_physical_centre.width(), m_physical_centre.height());
	double oldunit = m_unit;
	m_physical_centre = QSize(rect.width() / 2, rect.height() / 2);
	m_unit /= 10;

	int bg = (int)m_background;
	float bgr = float(GetRValue(bg)) / 255.0f;
	float bgg = float(GetGValue(bg)) / 255.0f;
	float bgb = float(GetBValue(bg)) / 255.0f;

	int fg = (int)m_foreground;
	float fgr = float(GetRValue(fg)) / 255.0f;
	float fgg = float(GetGValue(fg)) / 255.0f;
	float fgb = float(GetBValue(fg)) / 255.0f;

	stream << "/M {moveto} def\n";
	stream << "/L {lineto} def\n";
	stream << "/R {rlineto} def\n";
	stream << "/C {setrgbcolor} def\n";
	stream << "/W {setlinewidth} def\n";

	stream << "newpath\n"
		<< 0 << " " << 0 << " M\n"
		<< clrect.width() << " " << 0 << " R\n"
		<< 0 << " " << clrect.height() << " R\n"
		<< -clrect.width() << " " << 0 << " R\n"
		<< "closepath\n"
		<< bgr << " " << bgg << " " << bgb << " C\n"
		<< "fill\n";

	int state = pDoc->m_meta_graph->getState();

	QtRegion logicalviewport = LogicalViewport(rect, pDoc);

	if (state & MetaGraph::POINTMAPS) {
		pDoc->m_meta_graph->getDisplayedPointMap().setScreenPixel(m_unit); // only used by points (at the moment!)
		pDoc->m_meta_graph->getDisplayedPointMap().makeViewportPoints(logicalviewport);
	}
	if (state & MetaGraph::SHAPEGRAPHS) {
		pDoc->m_meta_graph->getDisplayedShapeGraph().makeViewportShapes(logicalviewport);
	}
	if (state & MetaGraph::DATAMAPS) {
		pDoc->m_meta_graph->getDisplayedDataMap().makeViewportShapes(logicalviewport);
	}
	if (state & MetaGraph::LINEDATA) {
		pDoc->m_meta_graph->makeViewportShapes(logicalviewport);
	}

	double spacer = GetSpacer(pDoc) / 10.0;
	if (spacer < 0.1) {
		spacer = 0.1;
	}

	if (state & MetaGraph::POINTMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWVGA) {

		// Define EPS box using spacer dimensions:
		stream << "/bx\n"
			<< " { newpath M\n"
			<< "   " << 2 * spacer << " " << 0 << " R\n"
			<< "   " << 0 << " " << 2 * spacer << " R\n"
			<< "   " << 2 * -spacer << " " << 0 << " R\n"
			<< "   closepath } def\n";
		stream << "/fbx\n"
			<< " { C fill } def\n";

		PointMap& map = pDoc->m_meta_graph->getDisplayedPointMap();

		while (map.findNextPoint()) {

			Point2f logical = map.getNextPointLocation();

			PafColor color = map.getCurrentPointColor();

			if (color.alphab() != 0) { // alpha == 0 is transparent

				QPoint p = PhysicalUnits(logical);

				// Now do EPS box... remember the coordinate system is the right way up!
				stream << p.x() / 10.0 - spacer << " " << (rect.height() - p.y()) / 10.0 - spacer << " bx\n";
				stream << color.redf() << " " << color.greenf() << " " << color.bluef() << " fbx\n";
			}
		}
	}

	if (state & MetaGraph::SHAPEGRAPHS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWAXIAL) {

		ShapeMap& map = pDoc->m_meta_graph->getDisplayedShapeGraph();

		OutputEPSMap(stream, map, logicalviewport, rect, spacer);
	}

	if (state & MetaGraph::DATAMAPS && pDoc->m_meta_graph->getViewClass() & MetaGraph::VIEWDATA) {

		ShapeMap& map = pDoc->m_meta_graph->getDisplayedDataMap();

		OutputEPSMap(stream, map, logicalviewport, rect, spacer);
	}

	if (state & MetaGraph::LINEDATA) {

		stream << "newpath\n";
		bool nextlayer = false;
		bool first = true;
		int style = 0;

		while (pDoc->m_meta_graph->findNextShape(nextlayer)) {

			const SalaShape& shape = pDoc->m_meta_graph->getNextShape();

			Line l;
			if (shape.isPoint()) {
			}
			else if (shape.isLine()) {
				SimpleLine line = shape.getLine();
				OutputEPSLine(stream, line, spacer, logicalviewport, rect);
			}
			else {
				OutputEPSPoly(stream, shape, spacer, logicalviewport, rect);
			}
		}

		int fg = (int)m_foreground;
		float fgr = float(GetRValue(fg)) / 255.0f;
		float fgg = float(GetGValue(fg)) / 255.0f;
		float fgb = float(GetBValue(fg)) / 255.0f;
		if (style == 1) {
			stream << "[" << spacer / 10 + 1 << "]";
		}
		else {
			stream << "[]";
		}
		stream << " 0 setdash\n";
		stream << fgr << " " << fgg << " " << fgb << " C\n";
		stream << spacer / 10 + 1 << " W\n";
		stream << "stroke\n";
	}

	// loaded paths
	if (pDoc->m_evolved_paths.size()) {

		stream << "newpath\n";

		for (size_t i = 0; i < pDoc->m_evolved_paths.size(); i++) {

			const std::vector<Point2f>& path = pDoc->m_evolved_paths[i];

			if (path.size() > 1) {

				QPoint last = PhysicalUnits(path[0]);
				stream << (last.x() - spacer / 40) / 10.0 << " " << (rect.height() - last.y() + spacer / 40) / 10.0 << " M\n\n";

				for (size_t j = 1; j < path.size(); j++) {
					QPoint next = PhysicalUnits(path[j]);
					stream << (next.x() - last.x() - spacer / 40) / 10.0 << " " << (last.y() - next.y() - spacer / 40) / 10.0 << " L\n";
					last = next;
				}

				stream << fgr << " " << fgg << " " << fgb << " C\n";
				stream << spacer / 20 + 1 << " W\n";
				stream << "stroke\n";
			}
		}
	}
	/*
	   if (pDoc->m_agent_manager && pDoc->m_agent_manager->isPaused())
	   {
		  if (pDoc->m_agent_manager->isEcomorphic()) {
			 // draw the ecomorphic artworks
			 Ecomorph& eco = pDoc->m_agent_manager->getEcomorph();
			 for (int i = 0; i < eco.arts().size(); i++)
			 {
				PixelRef pos = eco.arts()[i].getPos();
				Point2f logical = pDoc->m_meta_graph->getDisplayedPointMap().depixelate(pos);
				// this is in units of logical is unit based so this actually works okay:
				QPoint p = PhysicalUnits(Point2f(logical.x, logical.y));
				QPoint bottomleft = PhysicalUnits(Point2f(logical.x - 0.5, logical.y - 0.5));
				// Now do EPS box... remember the coordinate system is the right way up!
				stream << (p.x - spacer) / 10.0 << " " << (rect.Height() - p.y - spacer) / 10.0 << " bx\n";
				stream << bgr << " " << bgg << " " << bgb << " fbx\n";
				// And cover with line:
				stream << (bottomleft.x - spacer/20) / 10.0 << " " << (rect.Height() - bottomleft.y + spacer/20) / 10.0 << " M\n"
					   << (spacer * 2 - spacer/20) / 10.0 << " " << 0 << " L\n"
					   << 0 << " " << (spacer * 2 - spacer/20) / 10.0 << " L\n"
					   << (-spacer * 2 + spacer/20) / 10.0 << " " << 0 << " L\n"
					   << 0 << " " << (-spacer * 2 + spacer/20) / 10.0 << " L\n";
				stream << fgr << " " << fgg << " " << fgb << " setrgbcolor\n";
				stream << spacer/10+1 << " setlinewidth\n";
				stream << "stroke\n";
			 }
		  }
	   }
	*/

	if (includeScale) {
		// add the scale to the bottom lefthand corner
		double logicalwidth = m_unit * rect.width();
		if (logicalwidth > 10) {
			int workingwidth = floor(log10(logicalwidth / 2)*2.0);
			int barwidth = (int)pow(10.0, (double)(workingwidth / 2)) * ((workingwidth % 2 == 0) ? 1 : 5);
			double physicalbar = double(barwidth) / m_unit;
			stream << "newpath\n";
			stream << "0 0 M 0 18 R\n";
			stream << physicalbar / 10.0 << " 0 R 0 -18 R closepath\n";
			stream << bgr << " " << bgg << " " << bgb << " C\n";
			stream << "fill newpath\n";
			stream << fgr << " " << fgg << " " << fgb << " C\n";
			stream << "0 12 M\n";
			stream << physicalbar / 10.0 << " 0 R\n";
			stream << "3 W stroke\n";
			stream << "0 6 M 0 7.5 R\n";
			stream << physicalbar / 10.0 << " 6 M 0 7.5 R\n";
			stream << "1.5 W stroke\n";
			stream << "/Arial findfont 12 scalefont setfont\n";
			// assume metres!
			if (barwidth > 1000) {
				stream << "(" << (barwidth / 1000) << "km) stringwidth pop 2 div\n";
			}
			else {
				stream << "(" << barwidth << "m) stringwidth pop 2 div\n";
			}
			stream << physicalbar / 20.0 << " exch sub\n";
			stream << "0 M\n";
			stream << "(" << barwidth << "m) show\n";
		}
	}

	stream << "showpage\n";

	// undo temporary unit setting
	m_unit = oldunit;
	m_physical_centre = QSize(oldcentre.x(), oldcentre.y());
}
void GLView::OutputEPSMap(std::ofstream& stream, ShapeMap& map, QtRegion& logicalviewport, QRect& rect, float spacer)
{
	bool monochrome = (map.getDisplayParams().colorscale == DisplayParams::MONOCHROME);
	double thickness = 1.0, oldthickness = 1.0;
	bool closed, oldclosed = false;
	PafColor color, oldcolor;

	int fg = (int)m_foreground;
	float fgr = float(GetRValue(fg)) / 255.0f;
	float fgg = float(GetGValue(fg)) / 255.0f;
	float fgb = float(GetBValue(fg)) / 255.0f;

	stream << "newpath\n";
	stream << fgr << " " << fgg << " " << fgb << " C\n";
	bool dummy;
	while (map.findNextShape(dummy)) {

		// note: getNextLine clears current line, so getLineColor before line
		color = map.getShapeColor();
		const SalaShape& shape = map.getNextShape();
		closed = shape.isClosed();

		if (monochrome) {
			thickness = 3.0 * (color.blueb() * spacer) / 255.0;
			// note: anything below 'thin' threshold in monochrome note drawn
			if (thickness < 0.25) {
				continue;
			}
			if (thickness != oldthickness || closed != oldclosed) {
				stream << oldthickness << " W\n";
				stream << (oldclosed ? "fill" : "stroke") << std::endl;
				oldthickness = thickness;
				oldclosed = closed;
			}
		}
		else if (color != oldcolor || closed != oldclosed) {
			stream << oldcolor.redf() << " " << oldcolor.greenf() << " " << oldcolor.bluef() << " C\n";
			stream << (oldclosed ? "fill" : "stroke") << std::endl;
			oldcolor = color;
			oldclosed = closed;
		}

		Line l;
		if (shape.isPoint()) {
		}
		else if (shape.isLine()) {
			SimpleLine line = shape.getLine();
			OutputEPSLine(stream, line, spacer, logicalviewport, rect);
		}
		else {
			OutputEPSPoly(stream, shape, spacer, logicalviewport, rect);
		}
	}
	stream << thickness << " W\n";
	if (!monochrome) {
		stream << color.redf() << " " << color.greenf() << " " << color.bluef() << " C\n";
	}
	stream << (closed ? "fill" : "stroke") << std::endl;
}

void GLView::OutputEPSLine(std::ofstream& stream, SimpleLine& sim_line, int spacer, QtRegion& logicalviewport, QRect& rect)
{
	bool drewit = false;
	Line line(sim_line.start(), sim_line.end());
	if (line.crop(logicalviewport)) {
		QPoint start = PhysicalUnits(line.start());
		QPoint end = PhysicalUnits(line.end());
		// 10 units corresponds to 1 pixel on the screen
		if (sqrt(sqr(start.x() - end.x()) + sqr(start.y() - end.y())) > 5.0)
		{
			stream << (start.x() / 10.0) << " " << (rect.height() - start.y()) / 10.0 << " M ";
			stream << (end.x() / 10.0) << " " << (rect.height() - end.y()) / 10.0 << " L\n";
		}
	}
}

void GLView::OutputEPSPoly(std::ofstream& stream, const SalaShape& shape, int spacer, QtRegion& logicalviewport, QRect& rect)
{
	bool starter = true;
	Point2f lastpoint = shape.m_points[0];
	int count = shape.isClosed() ? static_cast<int>(shape.m_points.size()) + 1 : static_cast<int>(shape.m_points.size());
	int size = static_cast<int>(shape.m_points.size());
	for (int i = 1; i < count; i++) {
		Line line(lastpoint, shape.m_points[i%size]);
		if (line.crop(logicalviewport)) {
			// note: use t_start and t_end so that this line moves in the correct direction
			QPoint start = PhysicalUnits(line.t_start());
			QPoint end = PhysicalUnits(line.t_end());
			// 5.0 is about 1/2 pixel width
			if (sqrt(sqr(start.x() - end.x()) + sqr(start.y() - end.y())) > 5.0)
			{
				if (starter) {
					stream << start.x() / 10.0 << " " << (rect.height() - start.y()) / 10.0 << " M ";
				}
				stream << end.x() / 10.0 << " " << (rect.height() - end.y()) / 10.0 << " L\n";
				// note: you must use t_end (true end) so that it takes the end point from the shape[i] end:
				lastpoint = line.t_end();
				starter = false;
			}
		}
		else {
			lastpoint = shape.m_points[i];
			starter = true;
		}
	}
}
