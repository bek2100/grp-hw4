// CGWorkView.cpp : implementation of the CCGWorkView class
//
#include "stdafx.h"
#include "CGWork.h"
#include <limits>
#include "CGWorkDoc.h"
#include "CGWorkView.h"

#include <iostream>
#include <functional>
#include <string>
using std::cout;
using std::endl;
#include "MaterialDlg.h"
#include "LightDialog.h"
#include <algorithm> 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PngWrapper.h"
#include "iritSkel.h"
#include "MouseSensetiveDialog.h"
#include "FileRenderDlg.h"
#include "mat4.h"
#include "ColorSelectionDialog.h"
#include "PrespectiveControlDialog.h"
#include "OtherOptionsDialog.h"
#include <math.h>
#include "line.h"
#include <unordered_map>
#include <vector>
// For Status Bar access
#include "MainFrm.h"

extern std::vector<model> models;
extern IPFreeformConvStateStruct CGSkelFFCState;


// Use this macro to display text messages in the status bar.
#define STATUS_BAR_TEXT(str) (((CMainFrame*)GetParentFrame())->getStatusBar().SetWindowText(str))

#define IN_RANGE(x, y) ((1 <= x) && (x < (m_WindowWidth - 1)) && (1 <= y) && (y < (m_WindowHeight - 1)))
#define IN_SHADOWMAP_RANGE(x, y) ((1 <= x) && (x < (m_shadow_size - 1)) && (1 <= y) && (y < (m_shadow_size - 1)))
#define SCREEN_SPACE(x, y) (x + m_WindowWidth * (y))
#define SCREEN_SPACE_ALIASING(x, y) (x + (m_WindowWidth / m_nAntiAliasingDim) * (y))
#define SCREEN_SPACE_SHADOWMAP(x, y) (x + (m_shadow_size) * (y))
#define BGR(x) RGB(GetBValue(x), GetGValue(x), GetRValue(x));
#define SET_RGB(r,g,b) ((r)<<24|(g)<<16|(b)<<8|0)
#define SET_RGBA(r,g,b,a) ((r)<<24|(g)<<16|(b)<<8|a)
#define GET_R(x) (((x)&0xff000000)>>24)
#define GET_G(x) (((x)&0x00ff0000)>>16)
#define GET_B(x) (((x)&0x0000ff00)>>8)
#define GET_A(x) ((x)&0x000000ff)
#define RENDER_LIGHT 10
#define ANTI_ALIASING_DIM_5X5 5
#define ANTI_ALIASING_DIM_3X3 3

COLORREF marble_colors[33] = { RGB(255, 248, 220), RGB(255, 235, 205), RGB(255, 228, 196), RGB(218, 165, 32), RGB(255, 222, 173), RGB(245, 222, 179), RGB(222, 184, 135), RGB(210, 180, 140), RGB(222, 184, 135), RGB(222, 184, 135), RGB(218, 165, 32), RGB(205, 133, 63), RGB(210, 105, 30), RGB(139, 69, 19), RGB(160, 82, 45), RGB(165, 42, 42), RGB(128, 0, 0), RGB(165, 42, 42), RGB(160, 82, 45), RGB(139, 69, 19), RGB(210, 105, 30), RGB(205, 133, 63), RGB(218, 165, 32), RGB(222, 184, 135), RGB(245, 222, 179), RGB(210, 180, 140), RGB(222, 184, 135), RGB(245, 222, 179), RGB(255, 222, 173), RGB(255, 228, 196), RGB(255, 235, 205), RGB(255, 248, 220), RGB(222, 184, 135) };

COLORREF wood_colors[9] = { RGB(139, 69, 19), RGB(160, 82, 45), RGB(128, 60, 10), RGB(139, 69, 19), RGB(160, 82, 45), RGB(128, 60, 10), RGB(139, 69, 19), RGB(160, 82, 45), RGB(128, 60, 10) };

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView

IMPLEMENT_DYNCREATE(CCGWorkView, CView)

BEGIN_MESSAGE_MAP(CCGWorkView, CView)
	//{{AFX_MSG_MAP(CCGWorkView)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_MESSAGE(WM_MOUSEMOVE, OnMouseMovement)
	ON_COMMAND(ID_FILE_LOAD, OnFileLoad)
	ON_COMMAND(ID_WIREFRAME_COLOUR, OnWriteframeColor)
	ON_COMMAND(ID_VIEW_ORTHOGRAPHIC, OnViewOrthographic)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ORTHOGRAPHIC, OnUpdateViewOrthographic)
	ON_COMMAND(ID_VIEW_PERSPECTIVE, OnViewPerspective)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PERSPECTIVE, OnUpdateViewPerspective)
	ON_COMMAND(ID_ACTION_ROTATE, OnActionRotate)
	ON_UPDATE_COMMAND_UI(ID_ACTION_ROTATE, OnUpdateActionRotate)
	ON_COMMAND(ID_ACTION_SCALE, OnActionScale)
	ON_UPDATE_COMMAND_UI(ID_ACTION_SCALE, OnUpdateActionScale)
	ON_COMMAND(ID_ACTION_TRANSLATE, OnActionTranslate)
	ON_UPDATE_COMMAND_UI(ID_ACTION_TRANSLATE, OnUpdateActionTranslate)
	ON_COMMAND(ID_ACTION_RESETVIEW, OnActionResetView)
	ON_COMMAND(ID_ACTION_CLEARALL, OnActionClearAll)
	ON_COMMAND(ID_AXIS_X, OnAxisX)
	ON_UPDATE_COMMAND_UI(ID_AXIS_X, OnUpdateAxisX)
	ON_COMMAND(ID_AXIS_Y, OnAxisY)
	ON_UPDATE_COMMAND_UI(ID_AXIS_Y, OnUpdateAxisY)
	ON_COMMAND(ID_AXIS_Z, OnAxisZ)
	ON_UPDATE_COMMAND_UI(ID_AXIS_Z, OnUpdateAxisZ)
	ON_COMMAND(ID_AXIS_XY, OnAxisXY)
	ON_UPDATE_COMMAND_UI(ID_AXIS_XY, OnUpdateAxisXY)
	ON_COMMAND(ID_BOUNDBOX, OnBoundBox)
	ON_UPDATE_COMMAND_UI(ID_BOUNDBOX, OnUpdateBoundBox)
	ON_COMMAND(ID_LIGHT_SHADING_FLAT, OnLightShadingFlat)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_SHADING_FLAT, OnUpdateLightShadingFlat)
	ON_COMMAND(ID_LIGHT_SHADING_GOURAUD, OnLightShadingGouraud)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_SHADING_GOURAUD, OnUpdateLightShadingGouraud)
	ON_COMMAND(ID_LIGHT_CONSTANTS, OnLightConstants)
	ON_COMMAND(ID_BUTTON_TRANS_TOGGLE, OnActionToggleView)
	ON_COMMAND(ID_OPTIONS_MOUSESENSITIVITY, OnOptionMouseSensetivity)
	ON_COMMAND(ID_OPTIONS_PERSPECTIVECONTROL, OnOptionPrespectiveControl)
	ON_COMMAND(ID_OPTIONS_OTHEROPTIONS, OnOptionOthers)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_TRANS_TOGGLE, OnUpdateActionToggleView)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_POLYGON_GIVEN, &CCGWorkView::OnPolygonGiven)
	ON_COMMAND(ID_POLYGON_CALCULATED, &CCGWorkView::OnPolygonCalculated)
	ON_UPDATE_COMMAND_UI(ID_POLYGON_CALCULATED, &CCGWorkView::OnUpdatePolygonCalculated)
	ON_UPDATE_COMMAND_UI(ID_POLYGON_GIVEN, &CCGWorkView::OnUpdatePolygonGiven)
	ON_COMMAND(ID_VERTEX_GIVEN, &CCGWorkView::OnVertexGiven)
	ON_UPDATE_COMMAND_UI(ID_VERTEX_GIVEN, &CCGWorkView::OnUpdateVertexGiven)
	ON_COMMAND(ID_VERTEX_CALCULATED, &CCGWorkView::OnVertexCalculated)
	ON_UPDATE_COMMAND_UI(ID_VERTEX_CALCULATED, &CCGWorkView::OnUpdateVertexCalculated)
	ON_COMMAND(ID_VIEW_WIREFRAME, &CCGWorkView::OnViewWireframe)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WIREFRAME, &CCGWorkView::OnUpdateViewWireframe)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SOLID, &CCGWorkView::OnUpdateViewSolid)
	ON_COMMAND(ID_VIEW_SOLID, &CCGWorkView::OnViewSolid)
	ON_COMMAND(ID_VIEW_Z, &CCGWorkView::OnViewZ)
	ON_UPDATE_COMMAND_UI(ID_VIEW_Z, &CCGWorkView::OnUpdateViewZ)
	ON_COMMAND(ID_LIGHT_SHADING_PHONg, &CCGWorkView::OnLightShadingPhong)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_SHADING_PHONg, &CCGWorkView::OnUpdateLightShadingPhong)
	ON_COMMAND(ID_RENDER_TOFILE, &CCGWorkView::OnRenderTofile)
	ON_UPDATE_COMMAND_UI(ID_RENDER_TOFILE, &CCGWorkView::OnUpdateRenderTofile)
	ON_UPDATE_COMMAND_UI(ID_RENDER_TOSCREEN, &CCGWorkView::OnUpdateRenderToscreen)
	ON_COMMAND(ID_RENDER_TOSCREEN, &CCGWorkView::OnRenderToscreen)
	ON_COMMAND(ID_BACKGROUND_SETIMAGE, &CCGWorkView::OnBackgroundSetimage)
	ON_COMMAND(ID_BACKGROUND_ACTIVE, &CCGWorkView::OnBackgroundActive)
	ON_UPDATE_COMMAND_UI(ID_BACKGROUND_ACTIVE, &CCGWorkView::OnUpdateBackgroundActive)
	ON_COMMAND(ID_BACKGROUND_REPEAT, &CCGWorkView::OnBackgroundRepeat)
	ON_UPDATE_COMMAND_UI(ID_BACKGROUND_REPEAT, &CCGWorkView::OnUpdateBackgroundRepeat)
	ON_COMMAND(ID_BACKGROUND_STRETCH, &CCGWorkView::OnBackgroundStretch)
	ON_UPDATE_COMMAND_UI(ID_BACKGROUND_STRETCH, &CCGWorkView::OnUpdateBackgroundStretch)
	ON_COMMAND(ID_OPTIONS_BACKFACECULLING, &CCGWorkView::OnOptionsBackfaceculling)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_BACKFACECULLING, &CCGWorkView::OnUpdateOptionsBackfaceculling)
	ON_COMMAND(ID_OPTIONS_NORMALINVERSE, &CCGWorkView::OnOptionsNormalinverse)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_NORMALINVERSE, &CCGWorkView::OnUpdateOptionsNormalinverse)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_OVERRIDEGIVENNORMAL, &CCGWorkView::OnUpdateOptionsOverridegivennormal)
	ON_COMMAND(ID_OPTIONS_OVERRIDEGIVENNORMAL, &CCGWorkView::OnOptionsOverridegivennormal)
	ON_COMMAND(ID_OPTIONS_ADDSILHOUETTE, &CCGWorkView::OnOptionsAddsilhouette)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_ADDSILHOUETTE, &CCGWorkView::OnUpdateOptionsAddsilhouette)
	ON_COMMAND(ID_LIGHT_LIGHT1VIEW, &CCGWorkView::OnLightLight1pov)
	ON_UPDATE_COMMAND_UI(ID_LIGHT_LIGHT1VIEW, &CCGWorkView::OnUpdateLightLight1pov)
	ON_COMMAND(ID_LIGHT1POV_Z, &CCGWorkView::OnLight1povZ)
	ON_COMMAND(ID_LIGHT1POV_Y, &CCGWorkView::OnLight1povY)
	ON_COMMAND(ID_LIGHT1POV_X, &CCGWorkView::OnLight1povX)
	ON_COMMAND(ID_LIGHT1POV_NEG_X, &CCGWorkView::OnLight1povNegX)
	ON_COMMAND(ID_LIGHT1POV_NEG_Y, &CCGWorkView::OnLight1povNegY)
	ON_COMMAND(ID_LIGHT1POV_NEG_Z, &CCGWorkView::OnLight1povNegZ)
	ON_COMMAND(ID_TEXTURE_WOOD, &CCGWorkView::OnTextureWood)
	ON_UPDATE_COMMAND_UI(ID_TEXTURE_WOOD, &CCGWorkView::OnUpdateTextureWood)
	ON_COMMAND(ID_MARBLE_PICTURE, &CCGWorkView::OnMarblePicture)
	ON_UPDATE_COMMAND_UI(ID_MARBLE_PICTURE, &CCGWorkView::OnUpdateMarblePicture)
	ON_COMMAND(ID_MARBLE_SCALE, &CCGWorkView::OnMarbleScale)
	ON_UPDATE_COMMAND_UI(ID_MARBLE_SCALE, &CCGWorkView::OnUpdateMarbleScale)
	ON_COMMAND(ID_OPTIONS_SHADOWS, &CCGWorkView::OnOptionsShadows)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SHADOWS, &CCGWorkView::OnUpdateOptionsShadows)
	ON_UPDATE_COMMAND_UI(ID_LIGHT1POV_X, &CCGWorkView::OnUpdateLight1povX)
	ON_UPDATE_COMMAND_UI(ID_LIGHT1POV_NEG_X, &CCGWorkView::OnUpdateLight1povNegX)
	ON_UPDATE_COMMAND_UI(ID_LIGHT1POV_Y, &CCGWorkView::OnUpdateLight1povY)
	ON_UPDATE_COMMAND_UI(ID_LIGHT1POV_NEG_Y, &CCGWorkView::OnUpdateLight1povNegY)
	ON_UPDATE_COMMAND_UI(ID_LIGHT1POV_Z, &CCGWorkView::OnUpdateLight1povZ)
	ON_UPDATE_COMMAND_UI(ID_LIGHT1POV_NEG_Z, &CCGWorkView::OnUpdateLight1povNegZ)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_OFF, &CCGWorkView::OnUpdateAntialiasingOff)
	ON_COMMAND(ID_ANTIALIASING_OFF, &CCGWorkView::OnAntialiasingOff)
	ON_COMMAND(ID_ANTIALIASING_SINC3X3, &CCGWorkView::OnAntialiasingSinc3x3)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_SINC3X3, &CCGWorkView::OnUpdateAntialiasingSinc3x3)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_SINC5X5, &CCGWorkView::OnUpdateAntialiasingSinc5x5)
	ON_COMMAND(ID_ANTIALIASING_SINC5X5, &CCGWorkView::OnAntialiasingSinc5x5)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_BOX3X3, &CCGWorkView::OnUpdateAntialiasingBox3x3)
	ON_COMMAND(ID_ANTIALIASING_BOX3X3, &CCGWorkView::OnAntialiasingBox3x3)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_BOX5X5, &CCGWorkView::OnUpdateAntialiasingBox5x5)
	ON_COMMAND(ID_ANTIALIASING_BOX5X5, &CCGWorkView::OnAntialiasingBox5x5)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_TRIANGLE3X3, &CCGWorkView::OnUpdateAntialiasingTriangle3x3)
	ON_COMMAND(ID_ANTIALIASING_TRIANGLE3X3, &CCGWorkView::OnAntialiasingTriangle3x3)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_TRIANGLE5X5, &CCGWorkView::OnUpdateAntialiasingTriangle5x5)
	ON_COMMAND(ID_ANTIALIASING_TRIANGLE5X5, &CCGWorkView::OnAntialiasingTriangle5x5)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_GAUSSIAN3X3, &CCGWorkView::OnUpdateAntialiasingGaussian3x3)
	ON_COMMAND(ID_ANTIALIASING_GAUSSIAN3X3, &CCGWorkView::OnAntialiasingGaussian3x3)
	ON_UPDATE_COMMAND_UI(ID_ANTIALIASING_GAUSSIAN5X5, &CCGWorkView::OnUpdateAntialiasingGaussian5x5)
	ON_COMMAND(ID_ANTIALIASING_GAUSSIAN5X5, &CCGWorkView::OnAntialiasingGaussian5x5)
	END_MESSAGE_MAP()


// A patch to fix GLaux disappearance from VS2005 to VS2008
void auxSolidCone(GLdouble radius, GLdouble height) {
	GLUquadric *quad = gluNewQuadric();
	gluQuadricDrawStyle(quad, GLU_FILL);
	gluCylinder(quad, radius, 0.0, height, 20, 20);
	gluDeleteQuadric(quad);
}

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView construction/destruction

mat4 m_cur_transform;

CCGWorkView::CCGWorkView()
{
	// Set default values
	m_nAxis = ID_AXIS_X;
	m_nAction = ID_ACTION_ROTATE;
	m_nView = ID_VIEW_ORTHOGRAPHIC;
	m_screen = NULL;
	z_buffer = NULL;
	polygon_normal = NULL;

	m_object_space_trans = false;
	m_bound_box = false;
	m_mouse_sensetivity = 1;
	CGSkelFFCState.FineNess = 2;

	m_bIsPerspective = false;

	m_nAntiAliasing = ID_ANTIALIASING_OFF;
	m_nAntiAliasingDim = 1;

	m_camera_transpose[0][0] = 1;
	m_camera_transpose[1][1] = 1;
	m_camera_transpose[2][2] = 1;
	m_camera_transpose[3][2] = 4;

	m_camera_transpose[3][3] = 1;

	m_inv_camera_transpose[0][0] = 1;
	m_inv_camera_transpose[1][1] = 1;
	m_inv_camera_transpose[2][2] = 1;
	m_inv_camera_transpose[3][2] = -4;

	m_inv_camera_transpose[3][3] = 1;

	m_tarnsform[0][0] = 1;
	m_tarnsform[1][1] = 1;
	m_tarnsform[2][2] = 1;
	m_tarnsform[3][3] = 1;

	m_presepctive_d = 1;
	m_presepctive_alpha = 0.8 * m_presepctive_d;
	m_pic_name = "Default Name.png";
	m_pngHandle.SetFileName(m_pic_name.c_str());
	m_marble.SetFileName("Marble.png");
	m_marble.ReadPng();

	m_active_background = false;
	m_valid_background = false;
	m_silhouette = false;
	m_back_face_culling = false;
	m_texture = NULL;
	m_silhouette_thickness = 0.01;
	m_speculr_n = 2;
	m_shadows = false;

	// initilize the cameras' position and direction in the view space
	// used later
	m_camera_pos = vec4(0, 0, -1, 1);
	m_camera_at = vec4(0, 0, 0, 1);
	m_camera_up = vec4(0, -1, 0, 1);

	m_color_wireframe = RGB(0, 0, 0);
	m_background_color = RGB(255, 255, 255);
	m_boundbox_color = RGB(0, 0, 0);
	m_polygon_norm_color = RGB(0, 0, 0);
	m_silhouette_color = RGB(0, 0, 255);
	m_vertex_norm_color = RGB(0, 0, 0);

	m_nLightShading = ID_LIGHT_SHADING_FLAT;
	render_type = ID_VIEW_SOLID;
	m_render_target = ID_RENDER_TOSCREEN;

	m_background_type = ID_BACKGROUND_REPEAT;
	m_light_view = false;
	m_nLightView = ID_LIGHT1POV_Z;
	m_shadow_err = 15;
	m_shadow_size = 1000;

	//init the coesffiecents of the lights
	m_ambient_k = 0.3;
	m_diffuse_k = 0.7;
	m_speculr_k = 0.7;

	m_lMaterialAmbient = 0.2;
	m_lMaterialDiffuse = 0.8;
	m_lMaterialSpecular = 1.0;
	m_nMaterialCosineFactor = 32;

	for (int l = LIGHT_ID_1; l < MAX_LIGHT; l++){
		m_lights[l].enabled = false;
		m_lights[l].z_array_xdir = NULL;
		m_lights[l].z_array_neg_xdir = NULL;
		m_lights[l].z_array_ydir = NULL;
		m_lights[l].z_array_neg_ydir = NULL;
		m_lights[l].z_array_zdir = NULL;
		m_lights[l].z_array_neg_zdir = NULL;
	}

	//init the first light to be enabled
	m_lights[LIGHT_ID_1].enabled = true;
	m_lights[LIGHT_ID_1].space = LIGHT_SPACE_LOCAL;
	m_lights[LIGHT_ID_1].type = LIGHT_TYPE_POINT;
	m_lights[LIGHT_ID_1].posX = -4;
	m_lights[LIGHT_ID_1].posY = 0;
	m_lights[LIGHT_ID_1].posZ = 2;

	m_ambientLight.colorR = 30;
	m_ambientLight.colorG = 30;
	m_ambientLight.colorB = 30;

}

CCGWorkView::~CCGWorkView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView diagnostics

#ifdef _DEBUG
void CCGWorkView::AssertValid() const
{
	CView::AssertValid();
}

void CCGWorkView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CCGWorkDoc* CCGWorkView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCGWorkDoc)));
	return (CCGWorkDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView Window Creation - Linkage of windows to CGWork

BOOL CCGWorkView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	// An CGWork window must be created with the following
	// flags and must NOT include CS_PARENTDC for the
	// class style.

	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	return CView::PreCreateWindow(cs);
}

int CCGWorkView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	InitializeCGWork();

	return 0;
}

// This method initialized the CGWork system.
BOOL CCGWorkView::InitializeCGWork()
{
	m_pDC = new CClientDC(this);

	m_hDC = CreateCompatibleDC(m_pDC->GetSafeHdc());

	if (NULL == m_hDC) { // failure to get DC
		::AfxMessageBox(CString("Couldn't get a valid DC."));
		return FALSE;
	}

	if (NULL == m_pDC) { // failure to get DC
		::AfxMessageBox(CString("Couldn't get a valid DC."));
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView message handlers

void CCGWorkView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (0 >= cx || 0 >= cy) {
		return;
	}

	// save the width and height of the current window
	m_WindowWidth = m_nAntiAliasingDim * cx;
	m_WindowHeight = m_nAntiAliasingDim * cy;

	m_OriginalWindowWidth = m_WindowWidth;
	m_OriginalWindowHeight = m_WindowHeight;

	// compute the aspect ratio
	// this will keep all dimension scales equal
	m_AspectRatio = (GLdouble)m_WindowWidth / (GLdouble)m_WindowHeight;
}

BOOL CCGWorkView::SetupViewingFrustum(void)
{
	return TRUE;
}

// This viewing projection gives us a constant aspect ration. This is done by
// increasing the corresponding size of the ortho cube.
BOOL CCGWorkView::SetupViewingOrthoConstAspect(void)
{
	return TRUE;
}

BOOL CCGWorkView::OnEraseBkgnd(CDC* pDC)
{
	// Windows will clear the window with the background color every time your window 
	// is redrawn, and then CGWork will clear the viewport with its own background color.

	// return CView::OnEraseBkgnd(pDC);
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView drawing
/////////////////////////////////////////////////////////////////////////////

void CCGWorkView::OnDraw(CDC* pDC)
{
	CCGWorkDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	delete m_screen;
	delete z_buffer;
	m_screen = (COLORREF*)calloc(m_WindowWidth * m_WindowHeight, sizeof(COLORREF));
	z_buffer = (double*)calloc(m_WindowWidth * m_WindowHeight, sizeof(double));


	for (int l = 0; l < MAX_LIGHT; l++){
		delete m_lights[l].z_array_xdir;
		m_lights[l].z_array_xdir = (double*)calloc(m_shadow_size * m_shadow_size, sizeof(double));
	}

	mat4 screen_space_scale;
	mat4 screen_space_translate;
	
	// shadow screen space transformations
	// set the shadow screen scaling transformation
	int min_axis = min(m_WindowHeight, m_WindowWidth);
	screen_space_scale[0][0] = (double)m_shadow_size*0.8;
	screen_space_scale[1][1] = (double)m_shadow_size*0.8;
	screen_space_scale[2][2] = (double)m_shadow_size*0.8;
	screen_space_scale[3][3] = 1;

	// set the shadow screen translation transformation
	screen_space_translate[0][0] = 1;
	screen_space_translate[3][0] = 0.5 * m_shadow_size;

	screen_space_translate[1][1] = 1;
	screen_space_translate[3][1] = 0.5 * m_shadow_size;

	screen_space_translate[2][2] = 1;

	screen_space_translate[3][3] = 1;

	m_shadow_screen_space = screen_space_scale * screen_space_translate;

	// copmuter screent space transformations
	// set the screen scaling transformation
	screen_space_scale[0][0] = (double)min_axis*0.6;
	screen_space_scale[1][1] = (double)min_axis*0.6;
	screen_space_scale[2][2] = (double)min_axis*0.6;
	screen_space_scale[3][3] = 1;

	m_screen_space_scale = screen_space_scale;

	// inverse screen scalint transformation
	screen_space_scale[0][0] = 1 / ((double)min_axis*0.6);
	screen_space_scale[1][1] = 1 / ((double)min_axis*0.6);
	screen_space_scale[2][2] = 1 / ((double)min_axis*0.6);
	screen_space_scale[3][3] = 1;

	m_inv_screen_space_scale = screen_space_scale;

	// set the screen translation transformation
	screen_space_translate[0][0] = 1;
	screen_space_translate[3][0] = 0.5 * m_WindowWidth;

	screen_space_translate[1][1] = 1;
	screen_space_translate[3][1] = 0.5 * m_WindowHeight;

	screen_space_translate[2][2] = 1;

	screen_space_translate[3][3] = 1;

	m_screen_space_translate = screen_space_translate;

	// inverse screen space translate
	screen_space_translate[0][0] = 1;
	screen_space_translate[3][0] = -0.5 * m_WindowWidth;

	screen_space_translate[1][1] = 1;
	screen_space_translate[3][1] = -0.5 * m_WindowHeight;

	screen_space_translate[2][2] = 1;

	screen_space_translate[3][3] = 1;

	m_inv_screen_space_translate = screen_space_translate;

	// set the screens prespective transformation
	mat4 reset;
	m_prespective_trans = reset;

	m_prespective_trans[0][0] = 1;
	m_prespective_trans[1][1] = 1;
	m_prespective_trans[2][2] = m_presepctive_d / (m_presepctive_d - m_presepctive_alpha);
	m_prespective_trans[2][3] = 1 / m_presepctive_d;
	m_prespective_trans[3][2] = -m_presepctive_alpha * m_presepctive_d / (m_presepctive_d - m_presepctive_alpha);

	// inverse prespective trans

	m_inv_prespective_trans[0][0] = 1;
	m_inv_prespective_trans[1][1] = 1;
	m_inv_prespective_trans[2][2] = 0;
	m_inv_prespective_trans[2][3] = (m_presepctive_alpha - m_presepctive_d) / (m_presepctive_alpha * m_presepctive_d);
	m_inv_prespective_trans[3][2] = m_presepctive_d;
	m_inv_prespective_trans[3][3] = m_presepctive_d / m_presepctive_alpha;

	RenderScene();

	if (!pDoc)
		return;
}

/////////////////////////////////////////////////////////////////////////////
// CCGWorkView CGWork Finishing and clearing...

void CCGWorkView::OnDestroy()
{
	CView::OnDestroy();
	if (m_screen){
		delete m_screen;
	}
	if (z_buffer){
		delete z_buffer;
	}

	if (m_hDC){
		DeleteDC(m_hDC);
	}
	// delete the DC
	if (m_pDC) {
		delete m_pDC;
	}
}

/////////////////////////////////////////////////////////////////////////////
// User Defined Functions
LPRECT rect = (LPRECT)calloc(1, sizeof(LPRECT));

bool past_pressed;
LRESULT CCGWorkView::OnMouseMovement(WPARAM wparam, LPARAM lparam){
	int xPos = GET_X_LPARAM(lparam);
	int yPos = GET_Y_LPARAM(lparam);
	
	if (wparam == MK_RBUTTON){
		std::string str(("X - " + std::to_string(xPos) + "\n" + "Y - " + std::to_string(yPos)));
		CString notice(str.c_str());
		this->MessageBox(notice);
	}

	if (wparam == MK_LBUTTON && past_pressed)
	{
		mat4 temp_transform;
		mat4 temp_transform_xy;

		mat4 temp_inv_transform;
		mat4 temp_inv_transform_xy;

		// x mouse movement params
		double diff_x = (xPos - m_mouse_xpos);

		double teta_x = m_mouse_sensetivity * 10 * asin((double)diff_x / m_WindowWidth);

		double sinx = sin(teta_x);

		double cosx = cos(teta_x); // sin(x) ~= x movements are very small

		// x mouse movement params
		double diff_y = (yPos - m_mouse_ypos);

		double teta_y = m_mouse_sensetivity * 10 * asin((double)diff_y / m_WindowHeight);  //TODO scales...
		double siny = sin(teta_y);

		double cosy = cos(teta_y); // sin(y) ~= y movements are very small

		if (m_nAction == ID_ACTION_ROTATE){
			if (m_nAxis == ID_AXIS_X){
				// trasnfrom
				temp_transform[0][0] = 1;

				temp_transform[1][1] = cosy;
				temp_transform[1][2] = -siny;

				temp_transform[2][1] = siny;
				temp_transform[2][2] = cosy;

				temp_transform[3][3] = 1;

				//inv trasnfrom
				temp_inv_transform[0][0] = 1;

				temp_inv_transform[1][1] = cosy;
				temp_inv_transform[1][2] = siny;

				temp_inv_transform[2][1] = -siny;
				temp_inv_transform[2][2] = cosy;

				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_Y){
				// transform
				temp_transform[0][0] = cosx;
				temp_transform[0][2] = -sinx;

				temp_transform[1][1] = 1;

				temp_transform[2][0] = sinx;
				temp_transform[2][2] = cosx;

				temp_transform[3][3] = 1;

				// inv transfrom
				temp_inv_transform[0][0] = cosx;
				temp_inv_transform[0][2] = sinx;

				temp_inv_transform[1][1] = 1;

				temp_inv_transform[2][0] = -sinx;
				temp_inv_transform[2][2] = cosx;

				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_Z){
				// transform
				temp_transform[0][0] = cosx;
				temp_transform[0][1] = -sinx;

				temp_transform[1][0] = sinx;
				temp_transform[1][1] = cosx;

				temp_transform[2][2] = 1;

				temp_transform[3][3] = 1;

				//inv transform
				temp_inv_transform[0][0] = cosx;
				temp_inv_transform[0][1] = sinx;

				temp_inv_transform[1][0] = -sinx;
				temp_inv_transform[1][1] = cosx;

				temp_inv_transform[2][2] = 1;

				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_XY){
				// y transform
				temp_transform[0][0] = 1;

				temp_transform[1][1] = cosy;
				temp_transform[1][2] = siny;

				temp_transform[2][1] = -siny;
				temp_transform[2][2] = cosy;

				temp_transform[3][3] = 1;

				// y inv transfrom
				temp_inv_transform[0][0] = 1;

				temp_inv_transform[1][1] = cosy;
				temp_inv_transform[1][2] = -siny;

				temp_inv_transform[2][1] = siny;
				temp_inv_transform[2][2] = cosy;

				temp_inv_transform[3][3] = 1;

				// x transform
				temp_transform_xy[0][0] = cosx;
				temp_transform_xy[0][2] = sinx;

				temp_transform_xy[1][1] = 1;

				temp_transform_xy[2][0] = -sinx;
				temp_transform_xy[2][2] = cosx;

				temp_transform_xy[3][3] = 1;

				//x invy transform
				temp_inv_transform_xy[0][0] = cosx;
				temp_inv_transform_xy[0][2] = -sinx;

				temp_inv_transform_xy[1][1] = 1;

				temp_inv_transform_xy[2][0] = sinx;
				temp_inv_transform_xy[2][2] = cosx;

				temp_inv_transform_xy[3][3] = 1;

				temp_transform = temp_transform_xy * temp_transform;
				temp_inv_transform = temp_inv_transform * temp_inv_transform_xy;
			}
		}
		else if (m_nAction == ID_ACTION_TRANSLATE){
			if (m_nAxis == ID_AXIS_X){
				// x translate
				temp_transform[0][0] = 1;
				temp_transform[3][0] = m_mouse_sensetivity * (double)diff_x / m_WindowWidth;

				temp_transform[1][1] = 1;

				temp_transform[2][2] = 1;

				temp_transform[3][3] = 1;

				// x inv translate
				temp_inv_transform[0][0] = 1;
				temp_inv_transform[3][0] = -m_mouse_sensetivity * (double)diff_x / m_WindowWidth;

				temp_inv_transform[1][1] = 1;

				temp_inv_transform[2][2] = 1;

				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_Y){
				// y translate
				temp_transform[0][0] = 1;

				temp_transform[1][1] = 1;
				temp_transform[3][1] = m_mouse_sensetivity * (double)diff_y / m_WindowHeight;

				temp_transform[2][2] = 1;

				temp_transform[3][3] = 1;

				// y inv translate
				temp_inv_transform[0][0] = 1;

				temp_inv_transform[1][1] = 1;
				temp_inv_transform[3][1] = -m_mouse_sensetivity * (double)diff_y / m_WindowHeight;

				temp_inv_transform[2][2] = 1;

				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_Z){
				// z translate
				temp_transform[0][0] = 1;

				temp_transform[1][1] = 1;

				temp_transform[2][2] = 1;
				temp_transform[3][2] = m_mouse_sensetivity * (double)diff_x / m_WindowWidth;

				temp_transform[3][3] = 1;

				// z inv translate
				temp_inv_transform[0][0] = 1;

				temp_inv_transform[1][1] = 1;

				temp_inv_transform[2][2] = 1;
				temp_inv_transform[3][2] = -m_mouse_sensetivity * (double)diff_x / m_WindowWidth;

				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_XY){
				// xy translate
				temp_transform[0][0] = 1;
				temp_transform[3][0] = m_mouse_sensetivity * (double)diff_x / m_WindowWidth;

				temp_transform[1][1] = 1;
				temp_transform[3][1] = m_mouse_sensetivity * (double)diff_y / m_WindowHeight;

				temp_transform[2][2] = 1;

				temp_transform[3][3] = 1;

				// xy inv translate
				temp_inv_transform[0][0] = 1;
				temp_inv_transform[3][0] = -m_mouse_sensetivity * (double)diff_x / m_WindowWidth;

				temp_inv_transform[1][1] = 1;
				temp_inv_transform[3][1] = -m_mouse_sensetivity * (double)diff_y / m_WindowHeight;

				temp_inv_transform[2][2] = 1;

				temp_inv_transform[3][3] = 1;
			}
		}
		else if (m_nAction == ID_ACTION_SCALE){
			if (m_nAxis == ID_AXIS_X){
				// x scale
				temp_transform[0][0] = 1 + m_mouse_sensetivity * (double)diff_x / m_WindowWidth;
				temp_transform[1][1] = 1;
				temp_transform[2][2] = 1;
				temp_transform[3][3] = 1;

				// x inv scale
				temp_inv_transform[0][0] = 1 / (1 + m_mouse_sensetivity * (double)diff_x / m_WindowWidth);
				temp_inv_transform[1][1] = 1;
				temp_inv_transform[2][2] = 1;
				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_Y){
				// y scale
				temp_transform[0][0] = 1;
				temp_transform[1][1] = 1 + m_mouse_sensetivity * (double)diff_y / m_WindowHeight;
				temp_transform[2][2] = 1;
				temp_transform[3][3] = 1;

				// y inv scale
				temp_inv_transform[0][0] = 1;
				temp_inv_transform[1][1] = 1 / (1 + m_mouse_sensetivity * (double)diff_y / m_WindowHeight);
				temp_inv_transform[2][2] = 1;
				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_Z){
				// z scale
				temp_transform[0][0] = 1;
				temp_transform[1][1] = 1;
				temp_transform[2][2] = 1 + m_mouse_sensetivity * (double)diff_x / m_WindowWidth;
				temp_transform[3][3] = 1;

				// z inv scale
				temp_inv_transform[0][0] = 1;
				temp_inv_transform[1][1] = 1;
				temp_inv_transform[2][2] = 1 / (1 + m_mouse_sensetivity * (double)diff_x / m_WindowWidth);
				temp_inv_transform[3][3] = 1;
			}
			if (m_nAxis == ID_AXIS_XY){
				// xy scale
				temp_transform[0][0] = 1 + m_mouse_sensetivity * (double)diff_x / m_WindowWidth;
				temp_transform[1][1] = 1 + m_mouse_sensetivity * (double)diff_y / m_WindowHeight;
				temp_transform[2][2] = 1;
				temp_transform[3][3] = 1;

				// xy inv scale
				temp_inv_transform[0][0] = 1 / (1 + m_mouse_sensetivity * (double)diff_x / m_WindowWidth);
				temp_inv_transform[1][1] = 1 / (1 + m_mouse_sensetivity * (double)diff_y / m_WindowHeight);
				temp_inv_transform[2][2] = 1;
				temp_inv_transform[3][3] = 1;
			}
		}
		else {
			temp_transform[0][0] = 1;
			temp_transform[1][1] = 1;
			temp_transform[2][2] = 1;
			temp_transform[3][3] = 1;

			temp_inv_transform = temp_transform;
		}

		for (unsigned int m = 0; m < models.size(); m++){
			if (models[m].active_model){
				if (m_object_space_trans) {
					models[m].obj_coord_trans = temp_transform * models[m].obj_coord_trans;
					models[m].inv_obj_coord_trans = models[m].inv_obj_coord_trans * temp_inv_transform;
				}
				else {
					models[m].camera_trans = models[m].camera_trans * temp_transform;
					models[m].inv_camera_trans = temp_inv_transform * models[m].inv_camera_trans;
				}
			}
		}

		RenderScene();

		m_mouse_xpos = xPos;
		m_mouse_ypos = yPos;
	}
	else if (wparam == MK_LBUTTON){
		past_pressed = true;
		m_mouse_xpos = xPos;
		m_mouse_ypos = yPos;
	}
	else{
		past_pressed = false;
	}

	return 0;
};

static double Noise(vec4 pos){
	return sin(sqrt(pow(pos.x, 2) + pow(pos.y, 2) + pow(pos.z, 2)));
	//return abs(sin(pos.x));
}

static double Turbalance(vec4 pos, double pixels){
	double t = 0;
	double scale = 1;
	while (scale > pixels){
		t += abs(Noise(pos / scale) * scale);
		scale /= 2;
	}
	return t;
}

COLORREF CCGWorkView::MarbleColor(vec4 pos, COLORREF c){
	int width = m_marble.GetWidth();
	int height = m_marble.GetHeight();
	double val_x = abs(sin(pos.x + Turbalance(pos, 0.0312)));
	double cos_val = abs(cos(sqrt(pow(pos.x,2)+pow(pos.y,2)+pow(pos.z,2))));
	int cur_color = m_marble.GetValue(static_cast<int>(val_x * width), static_cast<int>(cos_val * height));
	if(m_texture == ID_MARBLE_PICTURE) return RGB(static_cast<int>((GET_B(cur_color) * GetRValue(c)) / 255), static_cast<int>((GET_G(cur_color) *GetGValue(c)) / 255), static_cast<int>((GET_R(cur_color)*GetBValue(c) / 255)));
	cur_color = marble_colors[static_cast<int>(val_x*33)];
	double mesh = val_x * 33 - static_cast<int>(val_x * 33);
	COLORREF color;
	color = marble_colors[static_cast<int>(val_x * 33) + 1];
	int c2r = static_cast<int>(max(min((GetBValue(cur_color)*mesh + GetBValue(color) * (1 - mesh)), 255), 0));
	int c2g = static_cast<int>(max(min((GetGValue(cur_color)*mesh + GetGValue(color) * (1 - mesh)), 255), 0));
	int c2b = static_cast<int>(max(min((GetRValue(cur_color)*mesh + GetRValue(color) * (1 - mesh)), 255), 0));
	return RGB(static_cast<int>(c2r*GetRValue(c) / 255), static_cast<int>(c2g*GetGValue(c) / 255), static_cast<int>(c2b*GetBValue(c) / 255));
}

static COLORREF WoodColor(vec4 pos, COLORREF c){
	double val_x = abs(sin(2*(pow(pos.x,2) + pow(pos.y,2) + Turbalance(pos, 0.12))));
	COLORREF cur_color = wood_colors[static_cast<int>(val_x * 9)];
	return RGB(static_cast<int>((GetBValue(cur_color) * GetRValue(c)) / 255), static_cast<int>((GetGValue(cur_color) *GetGValue(c)) / 255), static_cast<int>((GetRValue(cur_color)*GetBValue(c) / 255)));
}

double CCGWorkView::LinePointDepth(vec4 &p1, vec4 &p2, double x, double y){
	
	double p2_x = (p2.x / p2.p);
	double p2_y = (p2.y / p2.p);
	double p2_z = (p2.z / p2.p);

	double p1_x = (p1.x / p1.p);
	double p1_y = (p1.y / p1.p);
	double p1_z = (p1.z / p1.p);

	double z_delta = p1_z - p2_z;
	double y_delta = p1_y - p2_y;
	double x_delta = p1_x - p2_x;
	double z;

	if (y_delta != 0 && (abs(y_delta) >= abs(x_delta))) // line does not change in y
		z = (z_delta / y_delta) * (y - p2_y) + p2_z;
	else if (x_delta != 0) // line does not change in x
		z = (z_delta / x_delta) * (x - p2_x) + p2_z;
	else // line does not change in y and x, meaning we only draw p1 on screen
		z = min(p1_z, p2_z);

	return z;
	
}

double CCGWorkView::LinePointRatio(vec4 &p1, vec4 &p2, double x, double y){

	double p_delta = (p1.p - p2.p);
	double y_delta = p1.y - p2.y;
	double x_delta = p1.x - p2.x;
	double p;

	if (y_delta != 0 && (abs(y_delta) >= abs(x_delta))) // line does not change in y
		//p = (p_delta / y_delta) * (y - p2.y) + p2.p;
		p = (-p2.y * p_delta / y_delta + p2.p) / (1 - y * p_delta / y_delta);
	else if (x_delta != 0) // line does not change in x
		//p = (p_delta / x_delta) * (x - p2.x) + p2.p;
		p = (-p2.x * p_delta / x_delta + p2.p) / (1 - x * p_delta / x_delta);
	else // line does not change in y and x, meaning we only draw p1 on screen
		p = min(p1.p, p2.p);

	return p;

}

static vec4 LinePointNormal(vec4 &p1, vec4 &p2, vec4 p1_normal, vec4 p2_normal, int x, int y){

	double p2_x = static_cast<int>(p2.x / p2.p);
	double p2_y = static_cast<int>(p2.y / p2.p);
	double p2_z = (p2.z / p2.p);

	double p1_x = static_cast<int>(p1.x / p1.p);
	double p1_y = static_cast<int>(p1.y / p1.p);
	double p1_z = (p1.z / p1.p);

	vec4 n_delta = p1_normal - p2_normal;
	double y_delta = p1_y - p2_y;
	double x_delta = p1_x - p2_x;
	vec4 n;

	if (y_delta != 0 && (abs(y_delta) >= abs(x_delta))) // line does not change in y
		n = (n_delta / y_delta) * (y - p2_y) + p2_normal;
	else if (x_delta != 0) // line does not change in x
		n = (n_delta / x_delta) * (x - p2_x) + p2_normal;
	else // line does not change in y and x, meaning we only draw p1 on screen
		n = (p1_z < p2_z) ? p1_normal : p2_normal;

	return n;

}

static COLORREF LinePointLight(vec4 &p1, vec4 &p2, COLORREF p1_color, COLORREF p2_color, double x, double y){

	double p2_x = static_cast<int>(p2.x / p2.p);
	double p2_y = static_cast<int>(p2.y / p2.p);
  
	double p1_x = static_cast<int>(p1.x / p1.p);
	double p1_y = static_cast<int>(p1.y / p1.p);

	double p1_z = static_cast<int>(p1.z / p1.p);
	double p2_z = static_cast<int>(p2.z / p2.p);


	int c_red_delta = GetRValue(p1_color) - GetRValue(p2_color);
	int c_grn_delta = GetGValue(p1_color) - GetGValue(p2_color);
	int c_blu_delta = GetBValue(p1_color) - GetBValue(p2_color);
	double y_delta = p1_y - p2_y;
	double x_delta = p1_x - p2_x;
	double z_delta = p1_z - p2_z;

	double cred;
	double cgrn;
	double cblu;
	COLORREF c;

	int p1r = GetRValue(p1_color);
	int p1g = GetGValue(p1_color);
	int p1b = GetBValue(p1_color);

	int p2r = GetRValue(p2_color);
	int p2g = GetGValue(p2_color);
	int p2b = GetBValue(p2_color);

	bool use_y = y_delta < x_delta;


	if (y_delta != 0 && (abs(y_delta) >= abs(x_delta))){ // line does not change in x
		cred = ((double)c_red_delta / y_delta) * (y - p2_y) + GetRValue(p2_color);
		cgrn = ((double)c_grn_delta / y_delta) * (y - p2_y) + GetGValue(p2_color);
		cblu = ((double)c_blu_delta / y_delta) * (y - p2_y) + GetBValue(p2_color);
	}
	else if (x_delta != 0){ // line does not change in y
		cred = ((double)c_red_delta / x_delta) * (x - p2_x) + GetRValue(p2_color);
		cgrn = ((double)c_grn_delta / x_delta) * (x - p2_x) + GetGValue(p2_color);
		cblu = ((double)c_blu_delta / x_delta) * (x - p2_x) + GetBValue(p2_color);
	}
	else { // line does not change in y and x, meaning we only draw p1 on screen
		cred = (p1_z < p2_z) ? GetRValue(p1_color) : GetRValue(p2_color);
		cgrn = (p1_z < p2_z) ? GetGValue(p1_color) : GetGValue(p2_color);
		cblu = (p1_z < p2_z) ? GetBValue(p1_color) : GetBValue(p2_color);
	}

	c = RGB(static_cast<int>(max(min(cred, 255), 0)),
		    static_cast<int>(max(min(cgrn, 255), 0)),
			static_cast<int>(max(min(cblu, 255), 0)));

	int c2r = static_cast<int>(max(min(cred, 255), 0));
	int c2g = static_cast<int>(max(min(cgrn, 255), 0));
	int c2b = static_cast<int>(max(min(cblu, 255), 0));

	if (cred < 0 || cgrn < 0 || cblu < 0)
		bool shit = true;

	return c;

}

static double NextPoint(vec4 &p1, vec4 p2, double dis, double dx, double dy, double dz, double d){
	double east_er = 2 * dy;
	double north_east_er = 2 * (dy - dx);
	double north_er = -2 * dx;
	double south_er = 2 * dx;
	double south_east_er = 2 * dx + 2 * dy;
	double north_west_er = -2 * (dy - dx);
	double west_er = -2 * dy;
	double south_west_er = 2 * dx - 2 * dy;

	if (dx == 0){
		p1.y += dy / dis;
		p1.z += dz / dis;
		return 0;
	}
	double incline = dy / dx;
	if (incline <= 1 && incline > 0){
		if (d == NULL) d = 2 * dy - dx;
		if (dx > 0){
			if (d < 0){
				p1.x += dx / dis;
				p1.z += dz / dis;
				return d + east_er;
			}
			p1.x += dx / dis;
			p1.y += dy / dis;
			p1.z += dz / dis;
			return d + north_east_er;
		}
		if (d > 0){
			p1.x += dx / dis;
			p1.z += dz / dis;
			return d + west_er;
		}
		p1.x += dx / dis;
		p1.y += dy / dis;
		p1.z += dz / dis;
		return d + south_west_er;
	}
	if (incline > 1){
		if (d == NULL) d = dy - 2 * dx;
		if (dx > 0){
			if (d > 0){
				p1.y += dy / dis;
				p1.z += dz / dis;
				return d + north_er;
			}
			p1.x += dx / dis;
			p1.y += dy / dis;
			p1.z += dz / dis;
			return d + north_east_er;
		}
		if (d < 0){
			p1.y += dy / dis;
			p1.z += dz / dis;
			return d + south_er;
		}
		p1.x += dx / dis;
		p1.y += dy / dis;
		p1.z += dz / dis;
		return d + south_west_er;
	}
	if (incline > -1 && incline <= 0){
		if (d == NULL) d = dx + 2 * dy;
		if (dx > 0){
			if (d < 0){
				p1.x += dx / dis;
				p1.y += dy / dis;
				p1.z += dz / dis;
				return d + south_east_er;
			}
			p1.x += dx / dis;
			p1.z += dz / dis;
			return d + east_er;
		}
		if (d > 0){
			p1.x += dx / dis;
			p1.y += dy / dis;
			p1.z += dz / dis;
			return d + north_west_er;
		}
		p1.x += dx / dis;
		p1.z += dz / dis;
		return d + west_er;
	}
	if (incline <= -1){
		if (d == NULL) d = 2 * dx + dy;
		if (dx > 0){
			if (d < 0){
				p1.y += dy / dis;
				p1.z += dz / dis;
				return d + south_er;
			}
			p1.x += dx / dis;
			p1.y += dy / dis;
			p1.z += dz / dis;
			return d + south_east_er;
		}
		if (d > 0){
			p1.y += dy / dis;
			p1.z += dz / dis;
			return d + north_er;
		}
		p1.x += dx / dis;
		p1.y += dy / dis;
		p1.z += dz / dis;
		return d + north_west_er;
	}
	return 0;
}

void CCGWorkView::DrawLine(mat4 inv_cur_transform, double* z_arr, COLORREF *arr, vec4 &p1, vec4 &p2, COLORREF p1_color, vec4* p1_normal, COLORREF p2_color, vec4* p2_normal, std::unordered_map<int, std::vector<x_z_c_n_point>>* x_y, vec4* origin_1, vec4* origin_2){

	// if the line is beyond the screen space, dont bother drawing it
	if (!(((p1.z > m_presepctive_d && p2.z > m_presepctive_d) && !(p1.x <= 0 && p2.x <= 0) && !(p1.y <= 0 && p2.y <= 0))
		&& (m_nView == ID_VIEW_PERSPECTIVE) ||
		(m_nView == ID_VIEW_ORTHOGRAPHIC)))
		return;
	int xy = (x_y != NULL);
	bool draw = (arr != NULL);

	// algorithm vars
	int x1, x2, y1, y2, dx, dy, d;
	int north_er, north_west_er, west_er, south_west_er, south_er, south_east_er, east_er, north_east_er;

	// draw location variables
	int x, y;
	vec4 origin = vec4(0,0,0,0);
	vec4 origin_direction = vec4(0, 0, 0, 0);

	//DEBUG
	origin_1 = NULL;

	// midpoint algorithm
	double p1_x = p1.x / p1.p;
	double p2_x = p2.x / p2.p;
	double p1_y = p1.y / p1.p;
	double p2_y = p2.y / p2.p;
	double p1_z = p1.z / p1.p;
	double p2_z = p2.z / p2.p;

	x1 = static_cast<int>(min(p1_x, p2_x));
	x2 = static_cast<int>(max(p1_x, p2_x));

	double true_x1 = min(p1_x, p2_x);
	double true_x2 = max(p1_x, p2_x);

	double true_y1;
	double true_y2;

	if (x1 != x2){
		y1 = static_cast<int>(p1_x < p2_x ? p1_y : p2_y);
		y2 = static_cast<int>(p1_x < p2_x ? p2_y : p1_y);
		true_y1 = p1_x < p2_x ? p1_y : p2_y;
		true_y2 = p1_x < p2_x ? p2_y : p1_y;
	}
	else{
		y1 = static_cast<int>(min(p1_y, p2_y));
		y2 = static_cast<int>(max(p1_y, p2_y));
		true_y1 = min(p1_y, p2_y);
		true_y2 = max(p1_y, p2_y);
	}


	dx = x2 - x1;
	dy = y2 - y1;

	double true_dy = true_y2 - true_y1;
	double true_dx = true_x2 - true_x1;

	double y_step = abs(true_dy) / dy;
	double x_step = abs(true_dx) / dx;

	double o_dx = origin_direction.x - origin.x;
	double o_dy = origin_direction.y - origin.y;
	double o_dz = origin_direction.z - origin.z;
	double o_err = NULL;

	x = x1;
	y = y1;

	double dis = 0;

	east_er = 2 * dy;
	north_east_er = 2 * (dy - dx);
	north_er = -2 * dx;
	north_west_er = -2 * (dy - dx);
	west_er = -2 * dy;
	south_west_er = 2 * dx - 2 * dy;
	south_er = 2 * dx;
	south_east_er = 2 * dx + 2 * dy;

	double z, p, true_x, true_y, true_z;
	x_z_c_n_point xzcn_point;

	std::vector<COLORREF> color_debug;
	bool inc = false;
	bool dec = false;
	bool stat = false;


	COLORREF c;
	vec4 n = *p1_normal;

	
	true_y = true_y1;
	true_x = true_x1;
	z = LinePointDepth(p1, p2, x, y);
	true_z = LinePointDepth(p1, p2, true_x, true_y);
	p = LinePointRatio(p1, p2, true_x, true_y);

	//DEBUG
	if (abs(x - static_cast<int>(true_x)) > 1)
		bool shit = true;
	if (abs(y - static_cast<int>(true_y)) > 1)
		bool shit = true;
	if (abs(z - static_cast<int>(true_z)) > 1)
		bool shit = true;

	if (m_nLightShading == ID_LIGHT_SHADING_PHONg)
		n = LinePointNormal(p1, p2, *p1_normal, *p2_normal, x, y);

	if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD)
		c = LinePointLight(p1, p2, p1_color, p2_color ,x, y);
	else if (draw)
		c = ApplyLight(p1_color, n, p * vec4(true_x, true_y, true_z, 1), inv_cur_transform);
	else
		c = p1_color;

	if (m_texture == ID_MARBLE_PICTURE || m_texture == ID_MARBLE_SCALE) c = MarbleColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
	if (m_texture == ID_TEXTURE_WOOD) c = WoodColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);

	if (xy) {
		xzcn_point.x = x;
		xzcn_point.z = z;
		xzcn_point.true_x = true_x;
		xzcn_point.true_y = true_y;
		xzcn_point.true_z = true_z;
		xzcn_point.p = p;
		xzcn_point.c = c;
		xzcn_point.n = n;
		xzcn_point.origin = origin;
		(*x_y)[y].push_back(xzcn_point);
	}
	else if (IN_RANGE(x, y)){
		if (true_z < z_arr[SCREEN_SPACE(x, y)]){
			if (draw)
				arr[SCREEN_SPACE(x, y)] = c;
			z_arr[SCREEN_SPACE(x, y)] = true_z;
			if (m_render_target == ID_RENDER_TOFILE){
				m_pngHandle.SetValue(x, y, c);
			}
		}
	}
	// select the correct midpoint algorithm (direction and incline)
	if (dx == 0){ // horizontal y line or line in z direction only
		//move in positive y direction only

		while (y < y2){
			y = y + 1;


			true_y = true_y + y_step;
			true_x = (true_dx / true_dy) * (true_y - true_y1) + true_x1;
			true_z = LinePointDepth(p1, p2, true_x, true_y);
			z = LinePointDepth(p1, p2, x, y);
			p = LinePointRatio(p1, p2, true_x, true_y);

			//DEBUG
			if (abs(x - static_cast<int>(true_x)) > 1)
				bool shit = true;
			if (abs(y - static_cast<int>(true_y)) > 1)
				bool shit = true;
			if (abs(z - static_cast<int>(true_z)) > 1)
				bool shit = true;

			if (m_nLightShading == ID_LIGHT_SHADING_PHONg) 
				n = LinePointNormal(p1, p2, *p1_normal, *p2_normal, x, y);
			if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD) 
				c = LinePointLight(p1, p2, p1_color, p2_color, x, y);
			else if (draw)
				c = ApplyLight(p1_color, n, p * vec4(true_x, true_y, true_z, 1), inv_cur_transform);
			else
				c = p1_color;

			if (m_texture == ID_MARBLE_PICTURE || m_texture == ID_MARBLE_SCALE) c = MarbleColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
			if (m_texture == ID_TEXTURE_WOOD) c = WoodColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
            
			if (xy) {
				xzcn_point.x = x;
				xzcn_point.z = z;
				xzcn_point.p = p;
				xzcn_point.true_x = true_x;
				xzcn_point.true_y = true_y;
				xzcn_point.true_z = true_z;
				xzcn_point.c = c;
				xzcn_point.n = n;
				xzcn_point.origin = origin;
				(*x_y)[y].push_back(xzcn_point);
			}
			else if (IN_RANGE(x, y)){
				if (true_z < z_arr[SCREEN_SPACE(x, y)]){
					if (draw)
						arr[SCREEN_SPACE(x, y)] = c;
					z_arr[SCREEN_SPACE(x, y)] = true_z;
					if (m_render_target == ID_RENDER_TOFILE){
						m_pngHandle.SetValue(x, y, c);
					}
				}
			}
		}
		return;
	}

	double incline = (double)dy / (double)dx;

	if (incline > 1){
		d = dy - 2 * dx; // try to move in positive y direction only
		while (y < y2){
			if (d > 0){
				d = d + north_er;
				y = y + 1;
			}
			else{
				d = d + north_east_er;
				x = x + 1;
				y = y + 1;
			}
			
			true_y = true_y + y_step;
			true_x = (true_dx / true_dy) * (true_y - true_y1) + true_x1;
			true_z = LinePointDepth(p1, p2, true_x, true_y);
			z = LinePointDepth(p1, p2, x, y);
			p = LinePointRatio(p1, p2, true_x, true_y);

			//DEBUG
			if (abs(x - static_cast<int>(true_x)) > 1)
				bool shit = true;
			if (abs(y - static_cast<int>(true_y)) > 1)
				bool shit = true;
			if (abs(z - static_cast<int>(true_z)) > 1)
				bool shit = true;

			if (m_nLightShading == ID_LIGHT_SHADING_PHONg) 
				n = LinePointNormal(p1, p2, *p1_normal, *p2_normal, x, y);
			if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD) 
				c = LinePointLight(p1, p2, p1_color, p2_color, x, y);
			else if (draw)
				c = ApplyLight(p1_color, n, p * vec4(true_x, true_y, true_z, 1), inv_cur_transform);
			else
				c = p1_color;

			if (m_texture == ID_MARBLE_PICTURE || m_texture == ID_MARBLE_SCALE) c = MarbleColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
			if (m_texture == ID_TEXTURE_WOOD) c = WoodColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
            
			if (xy) {
				xzcn_point.x = x;
				xzcn_point.z = z;
				xzcn_point.true_x = true_x;
				xzcn_point.true_y = true_y;
				xzcn_point.true_z = true_z;
				xzcn_point.p = p;
				xzcn_point.c = c;
				xzcn_point.n = n;
				xzcn_point.origin = origin;
				(*x_y)[y].push_back(xzcn_point);
			}
			else if (IN_RANGE(x, y)){
				if (true_z < z_arr[SCREEN_SPACE(x, y)]){
					if (draw)
						arr[SCREEN_SPACE(x, y)] = c;
					z_arr[SCREEN_SPACE(x, y)] = true_z;
					if (m_render_target == ID_RENDER_TOFILE){
						m_pngHandle.SetValue(x, y, c);
					}
				}
			}
		}
	}
	else if (0 < incline && incline <= 1)
	{
		d = 2 * dy - dx; // try to move in positive x direction only, possibly positive y
		while (x < x2){
			if (d < 0){
				d = d + east_er;
				x = x + 1;
			}
			else{
				d = d + north_east_er;
				x = x + 1;
				y = y + 1;
			}
			
			true_x = true_x + x_step;
			true_y = (true_dy / true_dx) * (true_x - true_x1) + true_y1;
			true_z = LinePointDepth(p1, p2, true_x, true_y);
			z = LinePointDepth(p1, p2, x, y);
			p = LinePointRatio(p1, p2, true_x, true_y);

			//DEBUG
			if (abs(x - static_cast<int>(true_x)) > 1)
				bool shit = true;
			if (abs(y - static_cast<int>(true_y)) > 1)
				bool shit = true;
			if (abs(z - static_cast<int>(true_z)) > 1)
				bool shit = true;

			if (m_nLightShading == ID_LIGHT_SHADING_PHONg) 
				n = LinePointNormal(p1, p2, *p1_normal, *p2_normal, x, y);
			if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD) 
				c = LinePointLight(p1, p2, p1_color, p2_color, x, y);
			else if (draw)
				c = ApplyLight(p1_color, n, p * vec4(true_x, true_y, true_z, 1), inv_cur_transform);
			else
				c = p1_color;

			if (m_texture == ID_MARBLE_PICTURE || m_texture == ID_MARBLE_SCALE) c = MarbleColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
			if (m_texture == ID_TEXTURE_WOOD) c = WoodColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);

            if (xy) {
				xzcn_point.x = x;
				xzcn_point.z = z;
				xzcn_point.true_x = true_x;
				xzcn_point.true_y = true_y;
				xzcn_point.true_z = true_z;
				xzcn_point.p = p;
				xzcn_point.c = c;
				xzcn_point.n = n;
				xzcn_point.origin = origin;
				(*x_y)[y].push_back(xzcn_point);
				color_debug.push_back(c);
			}
			else if (IN_RANGE(x, y)){
				if (true_z < z_arr[SCREEN_SPACE(x, y)]){
					if (draw)
						arr[SCREEN_SPACE(x, y)] = c;
					z_arr[SCREEN_SPACE(x, y)] = true_z;
					if (m_render_target == ID_RENDER_TOFILE){
						m_pngHandle.SetValue(x, y, c);
					}
				}
			}
		}
	}
	else if (-1 < incline && incline <= 0){
		d = dx + 2 * dy; // try to move in positive x direction only, possibly negative y
		while (x < x2){
			if (d > 0){
				d = d + east_er;
				x = x + 1;
			}
			else{
				d = d + south_east_er;
				x = x + 1;
				y = y - 1;
			}
			
			true_x = true_x + x_step;
			true_y = (true_dy / true_dx) * (true_x - true_x1) + true_y1;
			true_z = LinePointDepth(p1, p2, true_x, true_y);
			z = LinePointDepth(p1, p2, x, y);
			p = LinePointRatio(p1, p2, true_x, true_y);

			//DEBUG
			if (abs(x - static_cast<int>(true_x)) > 1)
				bool shit = true;
			if (abs(y - static_cast<int>(true_y)) > 1)
				bool shit = true;
			if (abs(z - static_cast<int>(true_z)) > 1)
				bool shit = true;

			if (m_nLightShading == ID_LIGHT_SHADING_PHONg) 
				n = LinePointNormal(p1, p2, *p1_normal, *p2_normal, x, y);
			if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD) 
				c = LinePointLight(p1, p2, p1_color, p2_color, x, y);
			else if (draw)
				c = ApplyLight(p1_color, n, p * vec4(true_x, true_y, true_z, 1), inv_cur_transform);
			else
				c = p1_color;

			if (m_texture == ID_MARBLE_PICTURE || m_texture == ID_MARBLE_SCALE) c = MarbleColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
			if (m_texture == ID_TEXTURE_WOOD) c = WoodColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);

            if (xy) {
				xzcn_point.x = x;
				xzcn_point.z = z;
				xzcn_point.true_x = true_x;
				xzcn_point.true_y = true_y;
				xzcn_point.true_z = true_z;
				xzcn_point.p = p;
				xzcn_point.c = c;
				xzcn_point.n = n;
				xzcn_point.origin = origin;
				(*x_y)[y].push_back(xzcn_point);
			}
			else if (IN_RANGE(x, y)){
				if (true_z < z_arr[SCREEN_SPACE(x, y)]){
					if (draw)
						arr[SCREEN_SPACE(x, y)] = c;
					z_arr[SCREEN_SPACE(x, y)] = true_z;
					if (m_render_target == ID_RENDER_TOFILE){
						m_pngHandle.SetValue(x, y, c);
					}
				}
			}
		}
	}
	else if (incline <= -1){ // condition unneccessary, exists to make conditions clear
		d = 2 * dx + dy; // try to move in negative y direction only
		while (y > y2){
			if (d < 0){
				d = d + south_er;
				y = y - 1;
			}
			else{
				d = d + south_east_er;
				x = x + 1;
				y = y - 1;
			}
			
			true_y = true_y + y_step;
			true_x = (true_dx / true_dy) * (true_y - true_y1) + true_x1;
			true_z = LinePointDepth(p1, p2, true_x, true_y);
			z = LinePointDepth(p1, p2, x, y);
			p = LinePointRatio(p1, p2, true_x, true_y);

			//DEBUG
			if (abs(x - static_cast<int>(true_x)) > 1)
				bool shit = true;
			if (abs(y - static_cast<int>(true_y)) > 1)
				bool shit = true;
			if (abs(z - static_cast<int>(true_z)) > 1)
				bool shit = true;

			if (m_nLightShading == ID_LIGHT_SHADING_PHONg) 
				n = LinePointNormal(p1, p2, *p1_normal, *p2_normal, x, y);
			if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD) 
				c = LinePointLight(p1, p2, p1_color, p2_color, x, y);
			else if (draw)
				c = ApplyLight(p1_color, n, p * vec4(true_x, true_y, true_z, 1), inv_cur_transform);
			else
				c = p1_color;

			if (m_texture == ID_MARBLE_PICTURE || m_texture == ID_MARBLE_SCALE) c = MarbleColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
			if (m_texture == ID_TEXTURE_WOOD) c = WoodColor(p * vec4(x, y, z, 1) * inv_cur_transform_object_space, c);
            
            if (xy) {
				xzcn_point.x = x;
				xzcn_point.z = z;
				xzcn_point.true_x = true_x;
				xzcn_point.true_y = true_y;
				xzcn_point.true_z = true_z;
				xzcn_point.p = p;
				xzcn_point.c = c;
				xzcn_point.n = n;
				xzcn_point.origin = origin;
				(*x_y)[y].push_back(xzcn_point);
			}
			else if (IN_RANGE(x, y)){
				if (true_z < z_arr[SCREEN_SPACE(x, y)]){
					if (draw)
						arr[SCREEN_SPACE(x, y)] = c;
					z_arr[SCREEN_SPACE(x, y)] = true_z;
					if (m_render_target == ID_RENDER_TOFILE){
						m_pngHandle.SetValue(x, y, c);
					}
				}
			}
		}
	}

	return;
}

void CCGWorkView::ScanConversion(double *z_arr, COLORREF *arr, polygon &p, mat4 cur_transform, mat4 inv_cur_transfrom, COLORREF color){
	vec4 p1, p2;
	std::unordered_map<int, std::vector<x_z_c_n_point>> x_y;
	COLORREF c1, c2;

	////////////////////////////////////////////
	// create the normal of this polygon
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	line p1_normal_line;
	line p2_normal_line;
	vec4 p1_normal = vec4(0, 0, 0, 1);
	vec4 p2_normal = vec4(0, 0, 0, 1);
	if (m_nLightShading == ID_LIGHT_SHADING_FLAT){
		p2_normal_line = p1_normal_line = p.Normal(!m_override_normals);
		if (p1_normal_line.p_a == p1_normal_line.p_b)
			p2_normal_line = p1_normal_line = p.Normal(m_override_normals);

		vec4 normal_pa = p1_normal_line.p_a * cur_transform;
		vec4 normal_pb = p1_normal_line.p_b * cur_transform;
		p1_normal = (normal_pb / normal_pb.p) - (normal_pa / normal_pa.p);
	}

	////////////////////////////////////////////
	// "draw" the lines on screen and save where the x's of each y row are
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	c1 = c2 = color;
	vec4 p1_normal_pa, p1_normal_pb, p2_normal_pa, p2_normal_pb, x1, x2;
	for (unsigned int pnt = 0; pnt < p.points.size(); pnt++){
		x1 = p1 = p.points[pnt];
		x2 = p2 = p.points[(pnt + 1) % p.points.size()];
		if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD || m_nLightShading == ID_LIGHT_SHADING_PHONg){

			p1_normal_line = p.VertexNormal(!m_override_normals)[p1];
			if (p1_normal_line.p_a == p1_normal_line.p_b)
				p1_normal_line = p.VertexNormal(m_override_normals)[p1];

			p2_normal_line = p.VertexNormal(!m_override_normals)[p2];
			if (p2_normal_line.p_a == p2_normal_line.p_b)
				p2_normal_line = p.VertexNormal(m_override_normals)[p2];

			p1_normal_pa = p1_normal_line.p_a * cur_transform;
			p1_normal_pb = (p1_normal_line.p_b * cur_transform);
			p2_normal_pa = p2_normal_line.p_a * cur_transform;
			p2_normal_pb = (p2_normal_line.p_b * cur_transform);
			p1_normal = (p1_normal_pb / p1_normal_pb.p) - (p1_normal_pa / p1_normal_pa.p);
			p1_normal = p1_normal * m_inverse;

			p2_normal = (p2_normal_pb / p2_normal_pb.p) - (p2_normal_pa / p2_normal_pa.p);
			p2_normal = p2_normal * m_inverse;
		}

		p1 = p1 * cur_transform;
		p2 = p2 * cur_transform;

		if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD){
			c1 = ApplyLight(color, p1_normal, p1, inv_cur_transfrom);
			c2 = ApplyLight(color, p2_normal, p2, inv_cur_transfrom);
		}


		// skip polygons behind the camera
		if (!(((p1.z > m_presepctive_d && p2.z > m_presepctive_d) && !(p1.x <= 0 && p2.x <= 0) && !(p1.y <= 0 && p2.y <= 0))
			&& (m_nView == ID_VIEW_PERSPECTIVE) ||
			(m_nView == ID_VIEW_ORTHOGRAPHIC)))
			return;
		DrawLine(inv_cur_transfrom, z_arr, arr, p1, p2, c1, &p1_normal, c2, &p2_normal, &x_y, &x1, &x2);


		////////////////////////////////////////////
		// z_buffer drawing
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		vec4 scan_p1, scan_p2, n1, n2;
		COLORREF c;
		vec4 pol_normal = p.Normal_Val(!m_override_normals);
		double dis_x;
		double dx;
		double dy;
		double dz;
		vec4 origin;
		double err;
		double z_step, x_step, y_step, z, ratio;
		double true_y, true_x;
		for (auto iter = x_y.begin(); iter != x_y.end(); ++iter){
			std::sort(iter->second.begin(), iter->second.end());
			int y = iter->first;
			if (iter->second.size() > 1){
				//scan_p1 = vec4(iter->second[0].x, y, iter->second[0].z, 1);
				//scan_p2 = vec4(iter->second[iter->second.size() - 1].x, y, iter->second[iter->second.size() - 1].z, 1);
				// z buffer / shadow map calculations
				true_y = iter->second[0].true_y;
				true_x = iter->second[0].true_x;

				scan_p1 = vec4(iter->second[0].true_x, iter->second[0].true_y, iter->second[0].true_z, 1);
				scan_p2 = vec4(iter->second[iter->second.size() - 1].true_x, iter->second[iter->second.size() - 1].true_y, iter->second[iter->second.size() - 1].true_z, 1);
				x_step = (iter->second[iter->second.size() - 1].true_x - iter->second[0].true_x) / (iter->second[iter->second.size() - 1].x - static_cast<int>(iter->second[0].x));
				y_step = (iter->second[iter->second.size() - 1].true_y - iter->second[0].true_y) / (iter->second[iter->second.size() - 1].x - static_cast<int>(iter->second[0].x));
				z_step = (iter->second[iter->second.size() - 1].true_z - iter->second[0].true_z) / (iter->second[iter->second.size() - 1].x - static_cast<int>(iter->second[0].x));

				//scan_p1 = vec4(iter->second[0].x, y, iter->second[0].z, 1);
				//scan_p2 = vec4(iter->second[iter->second.size() - 1].x, y, iter->second[iter->second.size() - 1].z, 1);
				//x_step = (iter->second[iter->second.size() - 1].true_x - iter->second[0].true_x) / (static_cast<int>(iter->second[0].x) - iter->second[iter->second.size() - 1].x);
				//y_step = (iter->second[iter->second.size() - 1].true_y - iter->second[0].true_y) / (static_cast<int>(iter->second[0].x) - iter->second[iter->second.size() - 1].x);

				c1 = iter->second[0].c;
				c2 = iter->second[iter->second.size() - 1].c;
				n1 = iter->second[0].n;
				n2 = iter->second[iter->second.size() - 1].n;

				// texture calculations
				origin = iter->second[0].origin;
				dx = iter->second[iter->second.size() - 1].origin.x - origin.x;
				dy = iter->second[iter->second.size() - 1].origin.y - origin.y;
				dz = iter->second[iter->second.size() - 1].origin.z - origin.z;
				dis_x = iter->second[iter->second.size() - 1].x - iter->second[0].x;
				err = NULL;
				for (int x = static_cast<int>(iter->second[0].x); x <= iter->second[iter->second.size() - 1].x; x++){
					if (abs(x - static_cast<int>(true_x)) > 1)
						bool shit = true;
					if (abs(y - static_cast<int>(true_y)) > 1)
						bool shit = true;
					//if (abs(z - static_cast<int>(true_z)) > 1)
					//	bool shit = true;

					if (IN_RANGE(x, y)){
						z = LinePointDepth(scan_p1, scan_p2, true_x, true_y);
						ratio = LinePointRatio(iter->second[0].p * scan_p1, iter->second[iter->second.size() - 1].p * scan_p2, true_x, true_y);
						if (z < z_arr[SCREEN_SPACE(x, y)]){
							vec4 n = p1_normal;

							if (arr != NULL) {
								if (m_nLightShading == ID_LIGHT_SHADING_PHONg)
									n = LinePointNormal(scan_p1, scan_p2, n1, n2, x, y);
								if (m_nLightShading == ID_LIGHT_SHADING_GOURAUD)
									c = LinePointLight(scan_p1, scan_p2, c1, c2, x, y);
								else 
									c = ApplyLight(color, n, ratio * vec4(true_x, true_y, z, 1), inv_cur_transfrom);

								if (m_texture == ID_MARBLE_PICTURE || m_texture == ID_MARBLE_SCALE) c = MarbleColor(ratio * vec4(true_x, true_y, z, 1) * inv_cur_transform_object_space, c);
								if (m_texture == ID_TEXTURE_WOOD) c = WoodColor(ratio * vec4(true_x, true_y, z, 1) * inv_cur_transform_object_space, c);

								arr[SCREEN_SPACE(x, y)] = c;
							}

							z_arr[SCREEN_SPACE(x, y)] = z;
							if (m_render_target == ID_RENDER_TOFILE){
								m_pngHandle.SetValue(x, y, arr[SCREEN_SPACE(x, y)]);
							}
						}
					}
					if (m_texture != NULL) err = NextPoint(origin, iter->second[iter->second.size() - 1].origin, dis_x, dx, dy, dz, err);
					true_x += x_step;
					true_y += y_step;
				}
			}
		}
	}
}

mat4 CCGWorkView::ScaleToScreen(mat4 cur_transform, vec4 max_vec, vec4 min_vec){
	double minx = min_vec.x;
	double miny = min_vec.y;
	double minz = min_vec.z;

	double maxx = max_vec.x;
	double maxy = max_vec.y;
	double maxz = max_vec.z;

	vec4 edge_pnts[8];
	mat4 bound_box_scale;

	edge_pnts[0] = vec4(minx, miny, minz, 1.0);
	edge_pnts[1] = vec4(minx, miny, maxz, 1.0);

	edge_pnts[2] = vec4(minx, maxy, minz, 1.0);
	edge_pnts[3] = vec4(minx, maxy, maxz, 1.0);

	edge_pnts[4] = vec4(maxx, miny, minz, 1.0);
	edge_pnts[5] = vec4(maxx, miny, maxz, 1.0);

	edge_pnts[6] = vec4(maxx, maxy, minz, 1.0);
	edge_pnts[7] = vec4(maxx, maxy, maxz, 1.0);

	vec4 edge_pnt_on_screen = edge_pnts[0] * cur_transform;
	edge_pnt_on_screen = edge_pnt_on_screen / edge_pnt_on_screen.p;
	maxx = edge_pnt_on_screen.x;
	maxy = edge_pnt_on_screen.y;
	maxz = edge_pnt_on_screen.z;

	minx = edge_pnt_on_screen.x;
	miny = edge_pnt_on_screen.x;
	minz = edge_pnt_on_screen.x;

	for (int i = 0; i < 8; i++){
		edge_pnt_on_screen = edge_pnts[i] * cur_transform;
		edge_pnt_on_screen = edge_pnt_on_screen / edge_pnt_on_screen.p;
		maxx = max(edge_pnt_on_screen.x, maxx);
		maxy = max(edge_pnt_on_screen.y, maxy);
		maxz = max(edge_pnt_on_screen.z, maxz);

		minx = max(edge_pnt_on_screen.x, minx);
		miny = max(edge_pnt_on_screen.x, miny);
		minz = max(edge_pnt_on_screen.x, minz);
	}


	double scale = m_WindowWidth < m_WindowHeight ? m_WindowWidth / (maxx - minx) : m_WindowHeight / (maxy - miny);
	bound_box_scale[0][0] = scale;
	bound_box_scale[1][1] = scale;
	bound_box_scale[2][2] = scale;
	bound_box_scale[3][3] = 1;
	return bound_box_scale;
};


void CCGWorkView::DrawBoundBox(double *z_arr, COLORREF *arr, model &model, mat4 cur_transform, mat4 inv_cur_transform, COLORREF color){

	double minx = model.min_vec.x;
	double miny = model.min_vec.y;
	double minz = model.min_vec.z;

	double maxx = model.max_vec.x;
	double maxy = model.max_vec.y;
	double maxz = model.max_vec.z;

	vec4 xmin_ymin_zmin(minx, miny, minz, 1.0);
	vec4 xmin_ymin_zmax(minx, miny, maxz, 1.0);

	vec4 xmin_ymax_zmin(minx, maxy, minz, 1.0);
	vec4 xmin_ymax_zmax(minx, maxy, maxz, 1.0);

	vec4 xmax_ymin_zmin(maxx, miny, minz, 1.0);
	vec4 xmax_ymin_zmax(maxx, miny, maxz, 1.0);

	vec4 xmax_ymax_zmin(maxx, maxy, minz, 1.0);
	vec4 xmax_ymax_zmax(maxx, maxy, maxz, 1.0);

	vec4 psudo_normal = vec4(0, 0, 0, 1);

	// zmin rectangle first
	DrawLine(inv_cur_transform,z_arr, arr, xmin_ymin_zmin * cur_transform, xmin_ymax_zmin * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform, z_arr, arr, xmin_ymax_zmin * cur_transform, xmax_ymax_zmin * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform, z_arr, arr, xmax_ymax_zmin * cur_transform, xmax_ymin_zmin * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform, z_arr, arr, xmax_ymin_zmin * cur_transform, xmin_ymin_zmin * cur_transform, color, &psudo_normal, color, &psudo_normal);

	// zmax rectangle second
	DrawLine(inv_cur_transform,z_arr, arr, xmin_ymin_zmax * cur_transform, xmin_ymax_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform, z_arr, arr, xmin_ymax_zmax * cur_transform, xmax_ymax_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform, z_arr, arr, xmax_ymax_zmax * cur_transform, xmax_ymin_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform, z_arr, arr, xmax_ymin_zmax * cur_transform, xmin_ymin_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);

	// connect the two rectangles next
	DrawLine(inv_cur_transform, z_arr, arr, xmin_ymin_zmin * cur_transform, xmin_ymin_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform, z_arr, arr, xmin_ymax_zmin * cur_transform, xmin_ymax_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform,z_arr, arr, xmax_ymin_zmin * cur_transform, xmax_ymin_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);
	DrawLine(inv_cur_transform,z_arr, arr, xmax_ymax_zmin * cur_transform, xmax_ymax_zmax * cur_transform, color, &psudo_normal, color, &psudo_normal);
}

bool CCGWorkView::VisibleToLight(LightParams light, mat4 cur_inv_transform, vec4 point){

	if (!m_shadows) return true;

	mat4 light_transform;

	vec4 ligt_coord_pos = point *
		cur_inv_transform *
		light.light_transform;

	ligt_coord_pos = ligt_coord_pos / ligt_coord_pos.p;

	// check if light sees this point via z buffer;
	int x = static_cast<int>(ligt_coord_pos.x);
	int y = static_cast<int>(ligt_coord_pos.y);
	
	if (IN_SHADOWMAP_RANGE(x, y)){
		double z = light.z_array_xdir[SCREEN_SPACE_SHADOWMAP(x, y)];
		if (abs(ligt_coord_pos.z - z) < m_shadow_err)
			return true;
	}
	
	return false;
}

COLORREF CCGWorkView::ApplyLight(COLORREF in_color, vec4 normal, vec4 pos, mat4 cur_inv_transform){
	if (m_nLightShading == NULL) return in_color;
	// initilize as "no light"
	int inentisity = 0;

	// apply amient light
	double red_inentsity = m_ambient_k * ((double)m_ambientLight.colorR / 255) * GetRValue(in_color);
	double grn_inentsity = m_ambient_k * ((double)m_ambientLight.colorG / 255) * GetGValue(in_color);
	double blu_inentsity = m_ambient_k * ((double)m_ambientLight.colorB / 255) * GetBValue(in_color);

	vec4 light_pos = pos;

	double cos_teta = 1;
	double sin_teta = 1;
	double cos_alpha = 1;
	double len_light_dist = 1;
	double len_normal = sqrt(pow(normal.x, 2.0) +
		pow(normal.y, 2.0) +
		pow(normal.z, 2.0));
	double len_to_camera;

	pos = pos / pos.p;

	// draw z buffer depth using the blue channel
	if (render_type == ID_VIEW_Z)
		return RGB(static_cast<int>(255 * (pos.z / 4000)), 0, 0);

	// when you don't want light effect, tner a o length normal
	if (len_normal == 0)
		return 0;

	// apply diffuse reflection
	for (int l = LIGHT_ID_1; l < MAX_LIGHT; l++){
		if (m_lights[l].enabled){
			if (VisibleToLight(m_lights[l], cur_inv_transform, light_pos)){
				if (m_lights[l].type == LIGHT_TYPE_POINT){

					len_to_camera = sqrt(pow(pos.x, 2.0) +
						pow(pos.y, 2.0) +
						pow(pos.z, 2.0));

					len_light_dist = sqrt(pow(pos.x - m_lights[l].rel_pos.x, 2.0) +
						pow(pos.y - m_lights[l].rel_pos.y, 2.0) +
						pow(pos.z - m_lights[l].rel_pos.z, 2.0));

					// angle between point normal and light source direction
					cos_teta = ((pos.x - m_lights[l].rel_pos.x) * (normal.x) +
						(pos.y - m_lights[l].rel_pos.y) * (normal.y) +
						(pos.z - m_lights[l].rel_pos.z) * (normal.z)) /
						(len_normal * len_light_dist);

					//diffuse light from source
					// when the normal is facing the light source, add the calculation
					// when the normal is not facing the light source, ignore it
					red_inentsity += m_diffuse_k * ((double)m_lights[l].colorR / 255) * GetRValue(in_color) * ((cos_teta > 0) ? cos_teta : 0);
					grn_inentsity += m_diffuse_k * ((double)m_lights[l].colorG / 255) * GetGValue(in_color) * ((cos_teta > 0) ? cos_teta : 0);
					blu_inentsity += m_diffuse_k * ((double)m_lights[l].colorB / 255) * GetBValue(in_color) * ((cos_teta > 0) ? cos_teta : 0);

					//specular light from source
					double cos_big_teta = ((pos.x - m_lights[l].rel_pos.x) * (pos.x) +
						(pos.y - m_lights[l].rel_pos.y) * (pos.y) +
						(pos.z - m_lights[l].rel_pos.z) * (pos.z)) /
						(len_to_camera * len_light_dist);
					double cos_alpha = cos(acos(cos_big_teta) - 2 * acos(cos_teta));

					red_inentsity += m_speculr_k * ((double)m_lights[l].colorR / 255) * GetRValue(in_color) * ((cos_alpha > 0) ? pow(cos_alpha, m_speculr_n) : 0);
					grn_inentsity += m_speculr_k * ((double)m_lights[l].colorG / 255) * GetGValue(in_color) * ((cos_alpha > 0) ? pow(cos_alpha, m_speculr_n) : 0);
					blu_inentsity += m_speculr_k * ((double)m_lights[l].colorB / 255) * GetBValue(in_color) * ((cos_alpha > 0) ? pow(cos_alpha, m_speculr_n) : 0);
				}
				else if (m_lights[l].type == LIGHT_TYPE_DIRECTIONAL){
					len_to_camera = sqrt(pow(pos.x, 2.0) +
						pow(pos.y, 2.0) +
						pow(pos.z, 2.0));

					// the "light distance" here is the size of the direction
					len_light_dist = sqrt(pow(m_lights[l].rel_dir.x, 2.0) +
						pow(m_lights[l].rel_dir.y, 2.0) +
						pow(m_lights[l].rel_dir.z, 2.0));

					cos_teta = ((m_lights[l].rel_dir.x) * (normal.x) +
						(m_lights[l].rel_dir.y) * (normal.y) +
						(m_lights[l].rel_dir.z) * (normal.z)) /
						(len_normal * len_light_dist);

					//diffuse light from source
					// when the normal is facing the light source, add the calculation
					// when the normal is not facing the light source, ignore it
					red_inentsity += m_diffuse_k * ((double)m_lights[l].colorR / 255) * GetRValue(in_color) * ((cos_teta > 0) ? cos_teta : 0);
					grn_inentsity += m_diffuse_k * ((double)m_lights[l].colorG / 255) * GetGValue(in_color) * ((cos_teta > 0) ? cos_teta : 0);
					blu_inentsity += m_diffuse_k * ((double)m_lights[l].colorB / 255) * GetBValue(in_color) * ((cos_teta > 0) ? cos_teta : 0);

					//specular light from source
					double cos_big_teta = ((m_lights[l].rel_dir.x) * (pos.x) +
						(m_lights[l].rel_dir.y) * (pos.y) +
						(m_lights[l].rel_dir.z) * (pos.z)) /
						(len_to_camera * len_light_dist);
					double cos_alpha = cos(acos(cos_big_teta) - 2 * acos(cos_teta));

					red_inentsity += m_speculr_k * ((double)m_lights[l].colorR / 255) * GetRValue(in_color) * ((cos_alpha > 0) ? pow(cos_alpha, m_speculr_n) : 0);
					grn_inentsity += m_speculr_k * ((double)m_lights[l].colorG / 255) * GetGValue(in_color) * ((cos_alpha > 0) ? pow(cos_alpha, m_speculr_n) : 0);
					blu_inentsity += m_speculr_k * ((double)m_lights[l].colorB / 255) * GetBValue(in_color) * ((cos_alpha > 0) ? pow(cos_alpha, m_speculr_n) : 0);
				}
				else if (m_lights[l].type == LIGHT_TYPE_SPOT){

				}
			}
		}
	}

	COLORREF out_color = RGB(static_cast<int>(max(min(red_inentsity, 255), 0)),
						 	 static_cast<int>(max(min(grn_inentsity, 255), 0)),
							 static_cast<int>(max(min(blu_inentsity, 255), 0)));
	return out_color;
}

void CCGWorkView::SetBackgound(){
	if (m_active_background && m_valid_background){
		int bpp = m_pngBackground.GetNumChannels();
		int cur_color;
		int pic_width = m_pngBackground.GetWidth();
		int pic_height = m_pngBackground.GetHeight();
		if (bpp == 3 || bpp == 4){
			for (int x = 0; x < m_WindowWidth; x++)
				for (int y = 0; y < m_WindowHeight; y++){
					if (m_background_type == ID_BACKGROUND_REPEAT)
						cur_color = m_pngBackground.GetValue(
						x % pic_width,
						y % pic_height
						);
					else if (m_background_type == ID_BACKGROUND_STRETCH)
						cur_color = m_pngBackground.GetValue(
						static_cast<int>(((double)pic_width / m_WindowWidth) * x),
						static_cast<int>(((double)pic_height / m_WindowHeight) * y)
						);

					m_screen[SCREEN_SPACE(x, y)] = RGB(GET_B(cur_color), GET_G(cur_color), GET_R(cur_color));
					if (m_render_target == ID_RENDER_TOFILE){
						m_pngHandle.SetValue(x, y, m_screen[SCREEN_SPACE(x, y)]);
					}
				}
		}
		else
			for (int x = 0; x < m_WindowWidth; x++)
				for (int y = 0; y < m_WindowHeight; y++){
					if (m_background_type == ID_BACKGROUND_REPEAT)
						cur_color = m_pngBackground.GetValue(
						x % pic_width,
						y % pic_height
						);
					else if (m_background_type == ID_BACKGROUND_STRETCH)
						cur_color = m_pngBackground.GetValue(
						static_cast<int>(((double)pic_width / m_WindowWidth) * x),
						static_cast<int>(((double)pic_height / m_WindowHeight) * y)
						);

					m_screen[SCREEN_SPACE(x, y)] = RGB(GET_B(cur_color), GET_G(cur_color), GET_R(cur_color));;
					if (m_render_target == ID_RENDER_TOFILE){
						m_pngHandle.SetValue(x, y, m_screen[SCREEN_SPACE(x, y)]);
					}
				}
	}
	else
		std::fill_n(m_screen, m_WindowWidth * m_WindowHeight, m_background_color);

}

void CCGWorkView::set_light_pos(mat4 view_space_trans){
	
	vec4 pos;
	mat4 transpose;
	mat4 eye;
	eye[0][0] = 1;
	eye[1][1] = 1;
	eye[2][2] = 1;
	eye[3][3] = 1;

	for (int l = LIGHT_ID_1; l < MAX_LIGHT; l++){
		transpose = eye;
		if (m_lights[l].enabled){
			pos.x = m_lights[l].posX;
			pos.y = m_lights[l].posY;
			pos.z = m_lights[l].posZ;
			pos.p = 1;

			if (m_lights[l].space == LIGHT_SPACE_VIEW)
				pos = pos *  m_screen_space_scale * m_screen_space_translate;
			else
				pos = pos * view_space_trans * m_screen_space_scale * m_screen_space_translate;

			m_lights[l].rel_pos = pos / pos.p;

			pos = pos * view_space_trans;


			m_lights[l].transpose[3][0] = -m_lights[l].posX;
			m_lights[l].transpose[3][1] = -m_lights[l].posY;
			m_lights[l].transpose[3][2] = -m_lights[l].posZ;

			pos.x = m_lights[l].dirX;
			pos.y = m_lights[l].dirY;
			pos.z = m_lights[l].dirZ;
			pos.p = 1;



			if (m_lights[l].space == LIGHT_SPACE_VIEW)
				pos = pos * view_space_trans * m_screen_space_scale * m_screen_space_translate;
			else
				pos = pos * m_screen_space_scale * m_screen_space_translate;

			m_lights[l].rel_dir = pos / pos.p;

		}
	}
}

void CCGWorkView::RenderLightScene(LightParams &light, vec4 model_center, vec4 global_center){
	if (!m_shadows) return;

	int prev_m_nView = m_nView;
	int prev_m_nLightShading = m_nLightShading;
	int prev_m_WindowHeight = m_WindowHeight;
	int prev_m_WindowWidth = m_WindowWidth;
	
	// point the light to the center of the current model
	mat4 light_coord_system;
	mat4 traspose_to_center;
	vec4 light_dir_x, light_dir_y, light_dir_z;
	vec4 light_pos;
	
	if (light.type == LIGHT_TYPE_POINT){
		m_nView = ID_VIEW_PERSPECTIVE;

		light_pos.x = m_lights[0].posX;
		light_pos.y = m_lights[0].posY;
		light_pos.z = m_lights[0].posZ;
		light_dir_z = model_center - light_pos;

		light_dir_z = light_dir_z / sqrt(pow(light_dir_z.x, 2) + pow(light_dir_z.y, 2) + pow(light_dir_z.z, 2));
		light_dir_z.p = 1;

		if (light_dir_z.x != 0){
			light_dir_x.x = -(light_dir_z.y + light_dir_z.z) / light_dir_z.x;
			light_dir_x.y = 1;
			light_dir_x.z = 1;
		}
		else if (light_dir_z.y != 0) {
			light_dir_x.x = 1;
			light_dir_x.y = -(light_dir_z.x + light_dir_z.z) / light_dir_z.y;
			light_dir_x.z = 1;
		}
		else {
			light_dir_x.x = 1;
			light_dir_x.y = 1;
			light_dir_x.z = -(light_dir_z.y + light_dir_z.x) / light_dir_z.z;
		}

		light_dir_x = light_dir_x / sqrt(pow(light_dir_x.x, 2) + pow(light_dir_x.y, 2) + pow(light_dir_x.z, 2));
		light_dir_x.p = 1;

		light_dir_y = cross(light_dir_z, light_dir_x);

		light_dir_x.p = light_dir_y.p = light_dir_z.p = 0;

		light_coord_system[0] = light_dir_x;
		light_coord_system[1] = light_dir_y;
		light_coord_system[2] = light_dir_z;
		light_coord_system[3][3] = 1;
		light_coord_system = transpose(light_coord_system);

		light.light_transform = light.transpose	* light_coord_system * m_prespective_trans * m_shadow_screen_space;
	}
	else if (light.type == LIGHT_TYPE_DIRECTIONAL){
		m_nView = ID_VIEW_ORTHOGRAPHIC;

		light_dir_z.x = m_lights[0].dirX;
		light_dir_z.y = m_lights[0].dirY;
		light_dir_z.z = m_lights[0].dirZ;

		light_dir_z = light_dir_z / sqrt(pow(light_dir_z.x, 2) + pow(light_dir_z.y, 2) + pow(light_dir_z.z, 2));
		light_dir_z.p = 1;

		if (light_dir_z.x != 0){
			light_dir_x.x = -(light_dir_z.y + light_dir_z.z) / light_dir_z.x;
			light_dir_x.y = 1;
			light_dir_x.z = 1;
		}
		else if (light_dir_z.y != 0) {
			light_dir_x.x = 1;
			light_dir_x.y = -(light_dir_z.x + light_dir_z.z) / light_dir_z.y;
			light_dir_x.z = 1;
		}
		else {
			light_dir_x.x = 1;
			light_dir_x.y = 1;
			light_dir_x.z = -(light_dir_z.y + light_dir_z.x) / light_dir_z.z;
		}

		light_dir_x = light_dir_x / sqrt(pow(light_dir_x.x, 2) + pow(light_dir_x.y, 2) + pow(light_dir_x.z, 2));
		light_dir_x.p = 1;

		light_dir_y = cross(light_dir_z, light_dir_x);

		light_dir_x.p = light_dir_y.p = light_dir_z.p = 0;

		light_coord_system[0] = light_dir_x;
		light_coord_system[1] = light_dir_y;
		light_coord_system[2] = light_dir_z;
		light_coord_system[3][3] = 1;
		light_coord_system = transpose(light_coord_system);


		global_center = global_center * light_coord_system;

		traspose_to_center[0][0] = 1;
		traspose_to_center[1][1] = 1;
		traspose_to_center[2][2] = 1;

		traspose_to_center[3] = -1 * model_center;

		traspose_to_center[3][3] = 1;

		light.light_transform = light_coord_system * traspose_to_center * m_shadow_screen_space;
	}

	mat4 light_transform;
	mat4 null_transfrom;

	m_WindowHeight = m_shadow_size;
	m_WindowWidth = m_shadow_size;

	std::fill_n(light.z_array_xdir, m_shadow_size * m_shadow_size, std::numeric_limits<double>::infinity());
	for (unsigned int m = 0; m < models.size(); m++){

		light_transform = models[m].view_space_trans *
						models[m].obj_coord_trans	 *
						models[m].camera_trans	 *
						light.light_transform;

		for (unsigned int pol = 0; pol < models[m].polygons.size(); pol++){
			ScanConversion(light.z_array_xdir, NULL, models[m].polygons[pol], light_transform, null_transfrom, models[m].color);
		}
	}

	m_WindowHeight = prev_m_WindowHeight;
	m_WindowWidth = prev_m_WindowWidth;
	m_nView = prev_m_nView;
	m_nLightShading = prev_m_nLightShading;
}

void CCGWorkView::AntiAliasing(COLORREF *out_arr, COLORREF *in_arr){
	if (m_nAntiAliasing == ID_ANTIALIASING_OFF){
		std::memcpy(out_arr, in_arr, m_WindowWidth * m_WindowHeight * sizeof(COLORREF));
		return;
	}

	double r_min, g_min, b_min;
	double r_max, g_max, b_max;
	double f_r, f_g, f_b;

	r_min = g_min = b_min = 0;
	r_max = g_max = b_max = 255;
	
	for (int y = 0; y < m_WindowHeight / m_nAntiAliasingDim; y++){
		for (int x = 0; x < m_WindowWidth / m_nAntiAliasingDim; x++){
			f_r = f_g = f_b = 0;
			// apply the filter
			for (int m_j = 0; m_j < m_nAntiAliasingDim; m_j++){
				for (int m_i = 0; m_i < m_nAntiAliasingDim; m_i++){
					if (IN_RANGE(m_nAntiAliasingDim * x + m_i, m_nAntiAliasingDim * y + m_j)){
						f_r += m_AntiAliasingMask[m_i][m_j] * GetRValue(in_arr[SCREEN_SPACE(m_nAntiAliasingDim * x + m_i, m_nAntiAliasingDim * y + m_j)]);
						f_g += m_AntiAliasingMask[m_i][m_j] * GetGValue(in_arr[SCREEN_SPACE(m_nAntiAliasingDim * x + m_i, m_nAntiAliasingDim * y + m_j)]);
						f_b += m_AntiAliasingMask[m_i][m_j] * GetBValue(in_arr[SCREEN_SPACE(m_nAntiAliasingDim * x + m_i, m_nAntiAliasingDim * y + m_j)]);
					}
					else
						bool shit = true;
				}
			}

			if (f_r < -0.5 || f_r > 255.5 || f_g < -0.5 || f_g > 255.5 || f_b < -0.5 || f_b > 255.5)
				bool shit = true;

			out_arr[SCREEN_SPACE_ALIASING(x, y)] = RGB(static_cast<int>(max(0, min(f_r, 255))), static_cast<int>(max(0, min(f_g, 255))), static_cast<int>(max(0, min(f_b, 255))));
		}
	}

};

void CCGWorkView::RenderScene() {
	vec4 psudo_normal = vec4(0, 0, 0, 1);

	///////////////////////////////
	// PNG render handle
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_render_target == ID_RENDER_TOFILE){
		m_pngHandle.SetWidth(m_WindowWidth);
		m_pngHandle.SetHeight(m_WindowHeight);
		if (!m_pngHandle.InitWritePng()){
			//TODO send error
			return;
		}

	}

	///////////////////////////////
	// Background drawing
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SetBackgound();	

	///////////////////////////////
	// Camera POV rendering
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::fill_n(z_buffer, m_WindowWidth * m_WindowHeight, std::numeric_limits<double>::infinity());
	vec4 p1, p2;
	polygon cur_polygon;
	mat4 cur_transform;
	mat4 inv_cur_transfrom;
	COLORREF c;
	for (unsigned int m = 0; m < models.size(); m++){
		if (m_nView == ID_VIEW_ORTHOGRAPHIC){
			cur_transform = models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans * m_camera_transpose * m_screen_space_scale * m_screen_space_translate;
			inv_cur_transfrom = m_inv_screen_space_translate * m_inv_screen_space_scale * m_inv_camera_transpose;
			set_light_pos(models[m].camera_trans * m_camera_transpose);
		}
		else if (m_nView == ID_VIEW_PERSPECTIVE){
			cur_transform = models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans * m_camera_transpose * m_prespective_trans * m_screen_space_scale * m_screen_space_translate;
			inv_cur_transfrom = m_inv_screen_space_translate * m_inv_screen_space_scale * m_inv_prespective_trans * m_inv_camera_transpose;
			set_light_pos(models[m].camera_trans * m_camera_transpose);
		}
		inv_cur_transform_object_space = inv_cur_transfrom * models[m].inv_camera_trans * models[m].inv_obj_coord_trans;
		m_cur_transform = cur_transform;
		if (m_light_view){
			// point the light to the center of the current model
			mat4 light_coord_system, traspose_to_center;
			vec4 light_dir_x, light_dir_y, light_dir_z;
			vec4 light_pos;
			
			vec4 model_center;
			if (m_lights[0].type == LIGHT_TYPE_POINT){
				m_nView = ID_VIEW_PERSPECTIVE;
				model_center = (0.5 * (models[m].max_vec + models[m].min_vec)) * models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans;

				light_pos.x = m_lights[0].posX;
				light_pos.y = m_lights[0].posY;
				light_pos.z = m_lights[0].posZ;
				light_dir_z = model_center - light_pos;

				light_dir_z = light_dir_z / sqrt(pow(light_dir_z.x, 2) + pow(light_dir_z.y, 2) + pow(light_dir_z.z, 2));
				light_dir_z.p = 1;

				if (light_dir_z.x != 0){
					light_dir_x.x = -(light_dir_z.y + light_dir_z.z) / light_dir_z.x;
					light_dir_x.y = 1;
					light_dir_x.z = 1;
				}
				else if (light_dir_z.y != 0) {
					light_dir_x.x = 1;
					light_dir_x.y = -(light_dir_z.x + light_dir_z.z) / light_dir_z.y;
					light_dir_x.z = 1;
				}
				else {
					light_dir_x.x = 1;
					light_dir_x.y = 1;
					light_dir_x.z = -(light_dir_z.y + light_dir_z.x) / light_dir_z.z;
				}

				light_dir_x = light_dir_x / sqrt(pow(light_dir_x.x, 2) + pow(light_dir_x.y, 2) + pow(light_dir_x.z, 2));
				light_dir_x.p = 1;

				light_dir_y = cross(light_dir_z, light_dir_x);

				light_dir_x.p = light_dir_y.p = light_dir_z.p = 0;

				light_coord_system[0] = light_dir_x;
				light_coord_system[1] = light_dir_y;
				light_coord_system[2] = light_dir_z;
				light_coord_system[3][3] = 1;
				light_coord_system = transpose(light_coord_system);

				cur_transform = models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans * m_lights[0].transpose * light_coord_system * m_prespective_trans * m_screen_space_scale * m_screen_space_translate;

			}
			else if (m_lights[0].type == LIGHT_TYPE_DIRECTIONAL){
				m_nView = ID_VIEW_ORTHOGRAPHIC;
				model_center = vec4(0, 0, 0, 1) * models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans;

				light_dir_z.x = m_lights[0].dirX;
				light_dir_z.y = m_lights[0].dirY;
				light_dir_z.z = m_lights[0].dirZ;

				light_dir_z = light_dir_z / sqrt(pow(light_dir_z.x, 2) + pow(light_dir_z.y, 2) + pow(light_dir_z.z, 2));
				light_dir_z.p = 1;

				if (light_dir_z.x != 0){
					light_dir_x.x = -(light_dir_z.y + light_dir_z.z) / light_dir_z.x;
					light_dir_x.y = 1;
					light_dir_x.z = 1;
				}
				else if (light_dir_z.y != 0) {
					light_dir_x.x = 1;
					light_dir_x.y = -(light_dir_z.x + light_dir_z.z) / light_dir_z.y;
					light_dir_x.z = 1;
				}
				else {
					light_dir_x.x = 1;
					light_dir_x.y = 1;
					light_dir_x.z = -(light_dir_z.y + light_dir_z.x) / light_dir_z.z;
				}

				light_dir_x = light_dir_x / sqrt(pow(light_dir_x.x, 2) + pow(light_dir_x.y, 2) + pow(light_dir_x.z, 2));
				light_dir_x.p = 1;

				light_dir_y = cross(light_dir_z, light_dir_x);

				light_dir_x.p = light_dir_y.p = light_dir_z.p = 0;

				light_coord_system[0] = light_dir_x;
				light_coord_system[1] = light_dir_y;
				light_coord_system[2] = light_dir_z;
				light_coord_system[3][3] = 1;
				light_coord_system = transpose(light_coord_system);


				model_center = model_center * light_coord_system;

				traspose_to_center[0][0] = 1;
				traspose_to_center[1][1] = 1;
				traspose_to_center[2][2] = 1;

				traspose_to_center[3] = -1 * model_center;

				traspose_to_center[3][3] = 1;

				cur_transform = models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans * light_coord_system * traspose_to_center * m_screen_space_scale * m_screen_space_translate;
			}
		}

		if (render_type == ID_VIEW_SOLID || render_type == ID_VIEW_Z){

			// render scence from light POV for this model, assume if light is inside boundbox, model has no shadows
			vec4 model_center = (0.5 * (models[m].max_vec + models[m].min_vec)) * models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans;
			vec4 global_center = vec4(0, 0, 0, 1) * models[m].view_space_trans * models[m].obj_coord_trans * models[m].camera_trans;
			for (int i = 0; i < MAX_LIGHT; i++){
				if (m_lights[i].enabled)
					RenderLightScene(m_lights[i], model_center, global_center);
			}

			if (!m_back_face_culling){
				for (unsigned int pol = 0; pol < models[m].polygons.size(); pol++){
					if (m_texture != NULL) c = RGB(255, 255, 255);
					else  c = models[m].color;
					ScanConversion(z_buffer, m_screen, models[m].polygons[pol], cur_transform, inv_cur_transfrom, c);
				}
			}
			else{
				for (unsigned int pol = 0; pol < models[m].polygons.size(); pol++){
					p1 = models[m].polygons[pol].Normal(!m_override_normals).p_a * cur_transform;
					p2 = models[m].polygons[pol].Normal(!m_override_normals).p_b * cur_transform;
					if (m_texture != NULL) c = RGB(255, 255, 255);
					else  c = models[m].color;
					if (p2.z / p2.p < p1.z / p1.p)
						ScanConversion(z_buffer, m_screen, models[m].polygons[pol], cur_transform, inv_cur_transfrom, c);
				}
			}
		}
		else if (render_type == ID_VIEW_WIREFRAME){
			if (!m_back_face_culling){
				for (unsigned int pnt = 0; pnt < models[m].points_list.size(); pnt++){
					p1 = (models[m].points_list[pnt].p_a)* cur_transform;
					p2 = (models[m].points_list[pnt].p_b)* cur_transform;
					int prev_shading = m_nLightShading;
					m_nLightShading = ID_LIGHT_SHADING_FLAT;
					DrawLine(inv_cur_transfrom, z_buffer, m_screen, p1, p2, models[m].color, &psudo_normal, models[m].color, &psudo_normal);
					m_nLightShading = prev_shading;
				}
			}
			else {
				for (unsigned int pol = 0; pol < models[m].polygons.size(); pol++){
					p1 = models[m].polygons[pol].Normal(!m_override_normals).p_a * cur_transform;
					p2 = models[m].polygons[pol].Normal(!m_override_normals).p_b * cur_transform;
					if (p2.z / p2.p < p1.z / p1.p){
						for (unsigned int p = 0; p < models[m].polygons[pol].points.size(); p++){
							int prev_shading = m_nLightShading;
							m_nLightShading = ID_LIGHT_SHADING_FLAT;
							DrawLine(inv_cur_transfrom, z_buffer, m_screen, models[m].polygons[pol].points[p] * cur_transform, models[m].polygons[pol].points[(p + 1) % models[m].polygons[pol].points.size()] * cur_transform, models[m].color, &psudo_normal);
							m_nLightShading = prev_shading;
						}
					}
				}
			}
		}

		if (polygon_normal == ID_POLYGON_GIVEN || polygon_normal == ID_POLYGON_CALCULATED){
			for (unsigned int count = 0; count < models[m].polygons.size(); count++){
				cur_polygon = models[m].polygons[count];
				p1 = cur_polygon.Normal(given_polygon_normal).p_a * cur_transform;
				p2 = cur_polygon.Normal(given_polygon_normal).p_b * cur_transform;
				int prev_shading = m_nLightShading;
				m_nLightShading = ID_LIGHT_SHADING_FLAT;
				DrawLine(inv_cur_transfrom, z_buffer, m_screen, p1, p2, m_polygon_norm_color, &psudo_normal);
				m_nLightShading = prev_shading;
			}
		}

		if (vertex_normal == ID_VERTEX_GIVEN || vertex_normal == ID_VERTEX_CALCULATED){
			std::vector<line> vertex_normal = models[m].Normal(given_vertex_normal);
			for (unsigned int count = 0; count < vertex_normal.size(); count++){
				p1 = vertex_normal[count].p_a * cur_transform;

				if (m_inverse == -1){
					p2 = (2 * vertex_normal[count].p_a) - vertex_normal[count].p_b;
					p2.p = 1;
				}
				else
					p2 = vertex_normal[count].p_b;

				p2 = p2 * cur_transform;
				
				int prev_shading = m_nLightShading;
				m_nLightShading = ID_LIGHT_SHADING_FLAT;
				DrawLine(inv_cur_transfrom, z_buffer, m_screen, p1, p2, m_vertex_norm_color, &psudo_normal);
				m_nLightShading = prev_shading;
			}
		}
		if (m_bound_box){
			DrawBoundBox(z_buffer, m_screen, models[m], cur_transform, inv_cur_transfrom, m_boundbox_color);
		}
	}


	if (m_silhouette){
		std::fill_n(z_buffer, m_WindowWidth * m_WindowHeight, std::numeric_limits<double>::infinity());
		for (unsigned int m = 0; m < models.size(); m++){

			std::unordered_map<line, int> line_appears_once;
			for (unsigned int count = 0; count < models[m].polygons.size(); count++){
				for (unsigned int p = 0; p < models[m].polygons[count].points.size(); p++){
					vec4 cur_vertex = models[m].polygons[count].points[p];
					vec4 next_vertex = models[m].polygons[count].points[(p + 1) % models[m].polygons[count].points.size()];
					line cur_line = line(cur_vertex, next_vertex);
					line_appears_once[cur_line]++;
				}
			}

			std::unordered_map<line, int> fitting_normals;
			for (unsigned int count = 0; count < models[m].polygons.size(); count++){
				std::unordered_map<vec4, line> cur_normals = models[m].polygons[count].VertexNormal(!m_override_normals);
				line pol_normal = models[m].polygons[count].Normal(!m_override_normals);
				p1 = pol_normal.p_a * cur_transform;
				p2 = pol_normal.p_b * cur_transform;
				int polygon_normal_direction = (p1.z / p1.p >= p2.z / p2.p) ? 2 : 1;
				for (unsigned int p = 0; p < models[m].polygons[count].points.size(); p++){
					vec4 cur_vertex = models[m].polygons[count].points[p];
					vec4 next_vertex = models[m].polygons[count].points[(p + 1) % models[m].polygons[count].points.size()];
					// 0 - none, 1- one positive, 2- one negative, 3 - both
					line cur_line = line(cur_vertex, next_vertex);
					if (fitting_normals[cur_line] == 0){
						fitting_normals[cur_line] += polygon_normal_direction;
					}
					else if (fitting_normals[cur_line] == 1 && polygon_normal_direction == 2){
						fitting_normals[cur_line] += polygon_normal_direction;
					}
					else if (fitting_normals[cur_line] == 2 && polygon_normal_direction == 1){
						fitting_normals[cur_line] += polygon_normal_direction;
					}
					if (fitting_normals[cur_line] == 3 || line_appears_once[cur_line] == 1){

						p1 = cur_normals[cur_vertex].p_a;
						p2 = cur_normals[cur_vertex].p_b;
						vec4 p3 = cur_normals[next_vertex].p_b;
						vec4 p4 = cur_normals[next_vertex].p_a;
						p2 = p1 + (p2 - p1) * m_silhouette_thickness / m_WindowHeight;
						p3 = p4 + (p3 - p4) * m_silhouette_thickness / m_WindowHeight;

						polygon pl;
						pl.points.push_back(p1);
						pl.points.push_back(p2);
						pl.points.push_back(p3);
						pl.points.push_back(p4);
						int prev = m_nLightShading;
						m_nLightShading = NULL;
						ScanConversion(z_buffer, m_screen, pl, cur_transform, inv_cur_transfrom, m_silhouette_color);
						m_nLightShading = prev;
					}
				}
			}
		}
	}

	///////////////////////////////
	// Anti Aliasing
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	COLORREF* m_screen_out;
	int real_window_width = (m_WindowWidth / m_nAntiAliasingDim);
	int real_window_height = (m_WindowHeight / m_nAntiAliasingDim);

	GetWindowRect(rect);

	if (m_nAntiAliasing == ID_ANTIALIASING_OFF)
		m_screen_out = m_screen;
	else {
		m_screen_out = (COLORREF*)calloc(real_window_width * real_window_height, sizeof(COLORREF));
		AntiAliasing(m_screen_out, m_screen);
	}

	///////////////////////////////
	// Bruffer to output
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (m_render_target == ID_RENDER_TOFILE){
		m_pngHandle.WritePng();
	}
	else {

		m_map = CreateBitmap(real_window_width,	// width
			real_window_height,					// height
			1,								// Color Planes, unfortanutelly don't know what is it actually. Let it be 1
			8 * 4,							// Size of memory for one pixel in bits (in win32 4 bytes = 4*8 bits)
			(void*)m_screen_out);			// pointer to array

		SelectObject(m_hDC, m_map);			// Inserting picture into our temp HDC

		// Copy image from temp HDC to window
		BitBlt(m_pDC->GetSafeHdc(),			// Destination
			0,								// x and
			0,								// y - upper-left corner of place, where we'd like to copy
			real_window_width,					// width of the region
			real_window_height,					// height
			m_hDC,							// source
			0,								// x and
			0,								// y of upper left corner  of part of the source, from where we'd like to copy
			SRCCOPY);						// Defined DWORD to juct copy pixels. Watch more on msdn;

	}

	DeleteObject(m_map);
	if (m_nAntiAliasing != ID_ANTIALIASING_OFF)
		delete m_screen_out;
	return;
}

void CCGWorkView::OnFileLoad()
{
	TCHAR szFilters[] = _T("IRIT Data Files (*.itd)|*.itd|All Files (*.*)|*.*||");

	CFileDialog dlg(TRUE, _T("itd"), _T("*.itd"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

	if (dlg.DoModal() == IDOK) {
		m_strItdFileName = dlg.GetPathName();		// Full path and filename
		PngWrapper p;
		CGSkelProcessIritDataFiles(m_strItdFileName, 1);

		Invalidate();	// force a WM_PAINT for drawing.
	}

}

// VIEW HANDLERS ///////////////////////////////////////////
// Note: that all the following Message Handlers act in a similar way.
// Each control or command has two functions associated with it.

void CCGWorkView::OnViewOrthographic()
{
	m_nView = ID_VIEW_ORTHOGRAPHIC;
	m_bIsPerspective = false;
	Invalidate();		// redraw using the new view.
}

void CCGWorkView::OnUpdateViewOrthographic(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nView == ID_VIEW_ORTHOGRAPHIC);
}

void CCGWorkView::OnViewPerspective()
{
	m_nView = ID_VIEW_PERSPECTIVE;
	m_bIsPerspective = true;
	Invalidate();
}

void CCGWorkView::OnUpdateViewPerspective(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nView == ID_VIEW_PERSPECTIVE);
}

// ACTION HANDLERS ///////////////////////////////////////////

void CCGWorkView::OnActionRotate()
{
	m_nAction = ID_ACTION_ROTATE;
}

void CCGWorkView::OnUpdateActionRotate(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nAction == ID_ACTION_ROTATE);
}

void CCGWorkView::OnActionTranslate()
{
	m_nAction = ID_ACTION_TRANSLATE;
}

void CCGWorkView::OnUpdateActionTranslate(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nAction == ID_ACTION_TRANSLATE);
}

void CCGWorkView::OnActionScale()
{
	m_nAction = ID_ACTION_SCALE;
}

void CCGWorkView::OnUpdateActionScale(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nAction == ID_ACTION_SCALE);
}

void CCGWorkView::OnActionClearAll()
{
	models.clear();
	RenderScene();
}

void CCGWorkView::OnActionResetView()
{
	mat4 reset_transform;
	reset_transform[0][0] = 1;
	reset_transform[1][1] = 1;
	reset_transform[2][2] = 1;
	reset_transform[3][3] = 1;

	m_tarnsform = reset_transform;
	for (unsigned int m = 0; m < models.size(); m++){
		models[m].obj_coord_trans = reset_transform;
		models[m].camera_trans = reset_transform;
	}

	RenderScene();
}

// OBJ/CAMERA VIEW TOGGLE //////////////////////////////////

void CCGWorkView::OnActionToggleView()
{
	m_object_space_trans = !m_object_space_trans;
}

void CCGWorkView::OnUpdateActionToggleView(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_object_space_trans);
}

// AXIS HANDLERS ///////////////////////////////////////////

// Gets calles when the X button is pressed or when the Axis->X menu is selected.
// The only thing we do here is set the ChildView member variable m_nAxis to the 
// selected axis.
void CCGWorkView::OnAxisX()
{
	m_nAxis = ID_AXIS_X;
}

// Gets called when windows has to repaint either the X button or the Axis pop up menu.
// The control is responsible for its redrawing.
// It sets itself disabled when the action is a Scale action.
// It sets itself Checked if the current axis is the X axis.
void CCGWorkView::OnUpdateAxisX(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nAxis == ID_AXIS_X);
}

void CCGWorkView::OnAxisY()
{
	m_nAxis = ID_AXIS_Y;
}

void CCGWorkView::OnUpdateAxisY(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nAxis == ID_AXIS_Y);
}

void CCGWorkView::OnAxisZ()
{
	m_nAxis = ID_AXIS_Z;
}

void CCGWorkView::OnUpdateAxisZ(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nAxis == ID_AXIS_Z);
}

void CCGWorkView::OnAxisXY()
{
	m_nAxis = ID_AXIS_XY;
}

void CCGWorkView::OnUpdateAxisXY(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nAxis == ID_AXIS_XY);
}

// OPTIONS HANDLERS ///////////////////////////////////////////

void CCGWorkView::OnWriteframeColor()
{
	ColorSelectionDialog dlg(m_color_wireframe, m_boundbox_color, m_background_color, m_vertex_norm_color, m_polygon_norm_color, m_silhouette_color, m_silhouette_thickness);
	if (dlg.DoModal() == IDOK){

		COLORREF new_m_color_wireframe = RGB(GetBValue(dlg.wireframe_color), GetGValue(dlg.wireframe_color), GetRValue(dlg.wireframe_color));
		m_boundbox_color = RGB(GetBValue(dlg.boundbox_color), GetGValue(dlg.boundbox_color), GetRValue(dlg.boundbox_color));
		m_background_color = RGB(GetBValue(dlg.background_color), GetGValue(dlg.background_color), GetRValue(dlg.background_color));
		m_vertex_norm_color = RGB(GetBValue(dlg.vertex_norm_color), GetGValue(dlg.vertex_norm_color), GetRValue(dlg.vertex_norm_color));
		m_polygon_norm_color = RGB(GetBValue(dlg.polygon_norm_color), GetGValue(dlg.polygon_norm_color), GetRValue(dlg.polygon_norm_color));
		m_silhouette_color = RGB(GetBValue(dlg.silhouette_color), GetGValue(dlg.silhouette_color), GetRValue(dlg.silhouette_color));
		m_silhouette_thickness = dlg.silhouette_thickness;
		if (new_m_color_wireframe != m_color_wireframe){
			m_color_wireframe = new_m_color_wireframe;
			for (unsigned int m = 0; m < models.size(); m++){
				models[m].color = m_color_wireframe;
			}
		}
		Invalidate();
	}
}

void CCGWorkView::OnBoundBox()
{
	m_bound_box = !m_bound_box;
	Invalidate();
}

void CCGWorkView::OnUpdateBoundBox(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bound_box);
}

void CCGWorkView::OnOptionMouseSensetivity(){
	MouseSensetiveDialog dlg(m_mouse_sensetivity);
	if (dlg.DoModal() == IDOK){
		m_mouse_sensetivity = dlg.m_mouse_sensetivity;
	}
}

void CCGWorkView::OnOptionPrespectiveControl(){
	PrespectiveControlDialog dlg(m_presepctive_d, m_presepctive_alpha);
	if (dlg.DoModal() == IDOK){
		m_presepctive_d = dlg.d;
		m_presepctive_alpha = dlg.m_prespective_alpha;

		m_prespective_trans[0][0] = 1;
		m_prespective_trans[1][1] = 1;
		m_prespective_trans[2][2] = m_presepctive_d / (m_presepctive_d - m_presepctive_alpha);
		m_prespective_trans[2][3] = 1 / m_presepctive_d;
		m_prespective_trans[3][2] = -m_presepctive_alpha * m_presepctive_d / (m_presepctive_d - m_presepctive_alpha);

		Invalidate();
	}
}

void CCGWorkView::OnOptionOthers(){

	CString models_list;
	std::vector<bool> active_models_list;
	for (unsigned int m = 0; m < models.size(); m++){
		models_list += models[m].model_name + "\n";
		active_models_list.push_back(models[m].active_model);
	}
	OtherOptionsDialog dlg(m_shadow_size, m_shadow_err, CGSkelFFCState.FineNess, models_list, active_models_list);
	if (dlg.DoModal() == IDOK){
		CGSkelFFCState.FineNess = dlg.finess;
		m_shadow_err = dlg.shadow_err;
		m_shadow_size = dlg.shadow_size;
		for (unsigned int m = 0; m < models.size(); m++){
			models[m].active_model = dlg.active_modules[m];
		}

		Invalidate();
	}
}
// LIGHT SHADING HANDLERS ///////////////////////////////////////////

void CCGWorkView::OnLightShadingFlat()
{
	m_nLightShading = ID_LIGHT_SHADING_FLAT;
	Invalidate();
}

void CCGWorkView::OnUpdateLightShadingFlat(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nLightShading == ID_LIGHT_SHADING_FLAT);
}

void CCGWorkView::OnLightShadingGouraud()
{
	m_nLightShading = ID_LIGHT_SHADING_GOURAUD;
	Invalidate();
}

void CCGWorkView::OnUpdateLightShadingGouraud(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_nLightShading == ID_LIGHT_SHADING_GOURAUD);
}

// LIGHT SETUP HANDLER ///////////////////////////////////////////

void CCGWorkView::OnLightConstants()
{
	CLightDialog dlg;

	for (int id = LIGHT_ID_1; id<MAX_LIGHT; id++)
	{
		dlg.SetDialogData((LightID)id, m_lights[id]);
	}
	dlg.SetDialogData(LIGHT_ID_AMBIENT, m_ambientLight);
	dlg.SetLightConstants(m_ambient_k, m_diffuse_k, m_speculr_k, m_speculr_n);

	if (dlg.DoModal() == IDOK)
	{
		for (int id = LIGHT_ID_1; id<MAX_LIGHT; id++)
		{
			m_lights[id] = dlg.GetDialogData((LightID)id);
		}
		m_ambientLight = dlg.GetDialogData(LIGHT_ID_AMBIENT);

		m_ambient_k = dlg.m_ambient_mod;
		m_diffuse_k = dlg.m_diffuse_mod;
		m_speculr_k = dlg.m_specular_mod;
		m_speculr_n = dlg.m_specular_n;

		Invalidate();
	}
	
}

void CCGWorkView::OnPolygonGiven()
{
	if (polygon_normal == ID_POLYGON_GIVEN)
		polygon_normal = NULL;
	else
		polygon_normal = ID_POLYGON_GIVEN;
	given_polygon_normal = true;
	Invalidate();
}

void CCGWorkView::OnPolygonCalculated()
{
	if (polygon_normal == ID_POLYGON_CALCULATED)
		polygon_normal = NULL;
	else
		polygon_normal = ID_POLYGON_CALCULATED;
	given_polygon_normal = false;
	Invalidate();
}

void CCGWorkView::OnUpdatePolygonCalculated(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(polygon_normal == ID_POLYGON_CALCULATED);
}

void CCGWorkView::OnUpdatePolygonGiven(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(polygon_normal == ID_POLYGON_GIVEN);
}

void CCGWorkView::OnVertexGiven()
{
	if (vertex_normal == ID_VERTEX_GIVEN)
		vertex_normal = NULL;
	else
		vertex_normal = ID_VERTEX_GIVEN;
	given_vertex_normal = true;
	Invalidate();
}

void CCGWorkView::OnUpdateVertexGiven(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(vertex_normal == ID_VERTEX_GIVEN);
}

void CCGWorkView::OnVertexCalculated()
{
	if (vertex_normal == ID_VERTEX_CALCULATED)
		vertex_normal = NULL;
	else
		vertex_normal = ID_VERTEX_CALCULATED;
	given_vertex_normal = false;
	Invalidate();
}

void CCGWorkView::OnUpdateVertexCalculated(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(vertex_normal == ID_VERTEX_CALCULATED);
}

void CCGWorkView::OnViewWireframe()
{
	render_type = ID_VIEW_WIREFRAME;
	Invalidate();
}

void CCGWorkView::OnUpdateViewWireframe(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(render_type == ID_VIEW_WIREFRAME);
}

void CCGWorkView::OnViewSolid()
{
	render_type = ID_VIEW_SOLID;
	Invalidate();
}

void CCGWorkView::OnUpdateViewSolid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(render_type == ID_VIEW_SOLID);
}

void CCGWorkView::OnViewZ()
{
	render_type = ID_VIEW_Z;
	Invalidate();
}

void CCGWorkView::OnUpdateViewZ(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(render_type == ID_VIEW_Z);
}

void CCGWorkView::OnLightShadingPhong()
{
	m_nLightShading = ID_LIGHT_SHADING_PHONg;
	Invalidate();
}

void CCGWorkView::OnUpdateLightShadingPhong(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nLightShading == ID_LIGHT_SHADING_PHONg);
}

void CCGWorkView::OnRenderTofile()
{
	CString window_pic_name(m_pic_name.c_str());
	FileRenderDlg dlg(m_WindowWidth, m_WindowHeight, window_pic_name);
	m_OriginalWindowHeight = m_WindowHeight;
	m_OriginalWindowWidth = m_WindowWidth;

	if (dlg.DoModal() == IDOK)
	{
		m_WindowHeight = m_nAntiAliasingDim * dlg.m_pic_height;
		m_WindowWidth = m_nAntiAliasingDim * dlg.m_pic_width;

		//// Convert a TCHAR string to a LPCSTR
		// required for data type conversions
		CT2CA pszConvertedAnsiString(dlg.m_pic_name);
		m_pic_name = std::string(pszConvertedAnsiString);

		m_pngHandle.SetFileName(m_pic_name.c_str());

		m_render_target = ID_RENDER_TOFILE;
		
	}

	Invalidate();
}

void CCGWorkView::OnUpdateRenderTofile(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_render_target == ID_RENDER_TOFILE);
}

void CCGWorkView::OnRenderToscreen()
{
	m_render_target = ID_RENDER_TOSCREEN;
	m_WindowHeight = m_nAntiAliasingDim * m_OriginalWindowHeight;
	m_WindowWidth = m_nAntiAliasingDim * m_OriginalWindowWidth;
	Invalidate();
}

void CCGWorkView::OnUpdateRenderToscreen(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_render_target == ID_RENDER_TOSCREEN);
}

void CCGWorkView::OnBackgroundSetimage()
{
	TCHAR szFilters[] = _T("PNG Data Files (*.png)|*.png|All Files (*.*)|*.*||");

	CFileDialog dlg(TRUE, _T("png"), _T("*.png"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters);

	if (dlg.DoModal() == IDOK) {	
		CT2CA pszConvertedAnsiString(dlg.GetPathName()); // Full path and filename

		std::string string_filename = std::string(pszConvertedAnsiString);

		m_pngBackground.SetFileName(string_filename.c_str());
		
		
		if (m_pngBackground.ReadPng()){
			m_valid_background = true;
			m_pngBackground.ReadPng();
		}
		else {
			m_valid_background = false;
			// TODO eror message
		}

		Invalidate();	// force a WM_PAINT for drawing.
	}

}

void CCGWorkView::OnBackgroundActive()
{
	m_active_background = !m_active_background;
	Invalidate();
}

void CCGWorkView::OnUpdateBackgroundActive(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_active_background);
}

void CCGWorkView::OnBackgroundRepeat()
{
	m_background_type = ID_BACKGROUND_REPEAT;
	Invalidate();
}

void CCGWorkView::OnUpdateBackgroundRepeat(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_background_type == ID_BACKGROUND_REPEAT);
}

void CCGWorkView::OnBackgroundStretch()
{
	m_background_type = ID_BACKGROUND_STRETCH;
	Invalidate();
}

void CCGWorkView::OnUpdateBackgroundStretch(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_background_type == ID_BACKGROUND_STRETCH);
}

void CCGWorkView::OnOptionsBackfaceculling()
{
	// TODO: Add your command handler code here
	
	m_back_face_culling = !m_back_face_culling;
	Invalidate();
}

void CCGWorkView::OnUpdateOptionsBackfaceculling(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_back_face_culling);
}

void CCGWorkView::OnOptionsNormalinverse()
{
	for (unsigned int m = 0; m < models.size(); m++){
		for (unsigned int pol = 0; pol < models[m].polygons.size(); pol++){
			models[m].polygons[pol].inverse();
		}
	}
	m_inverse = -1 * m_inverse;
	Invalidate();
}

void CCGWorkView::OnUpdateOptionsNormalinverse(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_inverse == -1);
}

void CCGWorkView::OnUpdateOptionsOverridegivennormal(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_override_normals);
}

void CCGWorkView::OnOptionsOverridegivennormal()
{
	m_override_normals = !m_override_normals;
	Invalidate();
}

void CCGWorkView::OnOptionsAddsilhouette()
{
	// TODO: Add your command handler code here
	m_silhouette = !m_silhouette;
	Invalidate();
}

void CCGWorkView::OnUpdateOptionsAddsilhouette(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_silhouette);
}

void CCGWorkView::OnLightLight1pov()
{
	// TODO: Add your command handler code here
	m_light_view = !m_light_view;
	Invalidate();
}

void CCGWorkView::OnUpdateLightLight1pov(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_light_view);
}

void CCGWorkView::OnLight1povZ()
{
	m_nLightView = ID_LIGHT1POV_Z;
	Invalidate();
}

void CCGWorkView::OnLight1povNegZ()
{
	m_nLightView = ID_LIGHT1POV_NEG_Z;
	Invalidate();
}

void CCGWorkView::OnLight1povY()
{
	m_nLightView = ID_LIGHT1POV_Y;
	Invalidate();
}

void CCGWorkView::OnLight1povNegY()
{
	m_nLightView = ID_LIGHT1POV_NEG_Y;
	Invalidate();
}

void CCGWorkView::OnLight1povX()
{
	m_nLightView = ID_LIGHT1POV_X;
	Invalidate();
}

void CCGWorkView::OnLight1povNegX()
{
	m_nLightView = ID_LIGHT1POV_NEG_X;
	Invalidate();
}

void CCGWorkView::OnTextureWood()
{
	// TODO: Add your command handler code here
	if(m_texture==ID_TEXTURE_WOOD) m_texture = NULL;
	else m_texture = ID_TEXTURE_WOOD;
	Invalidate();
}

void CCGWorkView::OnUpdateTextureWood(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_texture==ID_TEXTURE_WOOD);
}

void CCGWorkView::OnMarblePicture()
{
	// TODO: Add your command handler code here
	if (m_texture == ID_MARBLE_PICTURE) m_texture = NULL;
	else m_texture = ID_MARBLE_PICTURE;
	Invalidate();
}


void CCGWorkView::OnUpdateMarblePicture(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_texture == ID_MARBLE_PICTURE);
}


void CCGWorkView::OnMarbleScale()
{
	// TODO: Add your command handler code here
	if (m_texture == ID_MARBLE_SCALE) m_texture = NULL;
	else m_texture = ID_MARBLE_SCALE;
	Invalidate();
}


void CCGWorkView::OnUpdateMarbleScale(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(m_texture == ID_MARBLE_SCALE);
}

void CCGWorkView::OnOptionsShadows()
{
	m_shadows = !m_shadows;
	Invalidate();
}


void CCGWorkView::OnUpdateOptionsShadows(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_shadows);
}


void CCGWorkView::OnUpdateLight1povX(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nLightView == ID_LIGHT1POV_X);
}


void CCGWorkView::OnUpdateLight1povNegX(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nLightView == ID_LIGHT1POV_NEG_X);
}


void CCGWorkView::OnUpdateLight1povY(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nLightView == ID_LIGHT1POV_Y);
}


void CCGWorkView::OnUpdateLight1povNegY(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nLightView == ID_LIGHT1POV_NEG_Y);
}


void CCGWorkView::OnUpdateLight1povZ(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nLightView == ID_LIGHT1POV_Z);
}


void CCGWorkView::OnUpdateLight1povNegZ(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nLightView == ID_LIGHT1POV_NEG_Z);
}


void CCGWorkView::OnUpdateAntialiasingOff(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_OFF);
}

void CCGWorkView::OnAntialiasingOff()
{
	m_nAntiAliasing = ID_ANTIALIASING_OFF;
	m_nAntiAliasingDim = 1;

	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}


void CCGWorkView::OnUpdateAntialiasingSinc3x3(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_SINC3X3);
}

void CCGWorkView::OnAntialiasingSinc3x3()
{
	m_nAntiAliasing = ID_ANTIALIASING_SINC3X3;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_3X3;
	double normalize = 1.0 / 24;
	m_AntiAliasingMask[0][0] = normalize * 2; m_AntiAliasingMask[0][1] = normalize * 3; m_AntiAliasingMask[0][2] = normalize * 2;
	m_AntiAliasingMask[1][0] = normalize * 3; m_AntiAliasingMask[1][1] = normalize * 4; m_AntiAliasingMask[1][2] = normalize * 3;
	m_AntiAliasingMask[2][0] = normalize * 2; m_AntiAliasingMask[2][1] = normalize * 3; m_AntiAliasingMask[2][2] = normalize * 2;

	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}

void CCGWorkView::OnUpdateAntialiasingSinc5x5(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_SINC5X5);
}


void CCGWorkView::OnAntialiasingSinc5x5()
{
	m_nAntiAliasing = ID_ANTIALIASING_SINC5X5;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_5X5;
	double normalize = 1.0 / 33;
	m_AntiAliasingMask[0][0] = normalize * -2; m_AntiAliasingMask[0][1] = normalize * -1; m_AntiAliasingMask[0][2] = normalize * 0; m_AntiAliasingMask[0][3] = normalize * -1; m_AntiAliasingMask[0][4] = normalize * -2;
	m_AntiAliasingMask[1][0] = normalize * -1; m_AntiAliasingMask[1][1] = normalize * 4;  m_AntiAliasingMask[1][2] = normalize * 6; m_AntiAliasingMask[1][3] = normalize * 4;  m_AntiAliasingMask[1][4] = normalize * -1;
	m_AntiAliasingMask[2][0] = normalize * 0;  m_AntiAliasingMask[2][1] = normalize * 6;  m_AntiAliasingMask[2][2] = normalize * 9; m_AntiAliasingMask[2][3] = normalize * 6;  m_AntiAliasingMask[2][4] = normalize * 0;
	m_AntiAliasingMask[3][0] = normalize * -1; m_AntiAliasingMask[3][1] = normalize * 4;  m_AntiAliasingMask[3][2] = normalize * 6; m_AntiAliasingMask[3][3] = normalize * 4;  m_AntiAliasingMask[3][4] = normalize * -1;
	m_AntiAliasingMask[4][0] = normalize * -2; m_AntiAliasingMask[4][1] = normalize * -1; m_AntiAliasingMask[4][2] = normalize * 0; m_AntiAliasingMask[4][3] = normalize * -1; m_AntiAliasingMask[4][4] = normalize * -2;

	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}


void CCGWorkView::OnUpdateAntialiasingBox3x3(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_BOX3X3);
}


void CCGWorkView::OnAntialiasingBox3x3()
{
	m_nAntiAliasing = ID_ANTIALIASING_BOX3X3;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_3X3;
	double normalize = 1.0 / 9;
	m_AntiAliasingMask[0][0] = normalize * 1; m_AntiAliasingMask[0][1] = normalize * 1; m_AntiAliasingMask[0][2] = normalize * 1;
	m_AntiAliasingMask[1][0] = normalize * 1; m_AntiAliasingMask[1][1] = normalize * 1; m_AntiAliasingMask[1][2] = normalize * 1;
	m_AntiAliasingMask[2][0] = normalize * 1; m_AntiAliasingMask[2][1] = normalize * 1; m_AntiAliasingMask[2][2] = normalize * 1;

	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}


void CCGWorkView::OnUpdateAntialiasingBox5x5(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_BOX5X5);
}


void CCGWorkView::OnAntialiasingBox5x5()
{
	m_nAntiAliasing = ID_ANTIALIASING_BOX5X5;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_5X5;
	double normalize = 1.0 / 25;
	m_AntiAliasingMask[0][0] = normalize * 1; m_AntiAliasingMask[0][1] = normalize * 1; m_AntiAliasingMask[0][2] = normalize * 1; m_AntiAliasingMask[0][3] = normalize * 1; m_AntiAliasingMask[0][4] = normalize * 1;
	m_AntiAliasingMask[1][0] = normalize * 1; m_AntiAliasingMask[1][1] = normalize * 1; m_AntiAliasingMask[1][2] = normalize * 1; m_AntiAliasingMask[1][3] = normalize * 1; m_AntiAliasingMask[1][4] = normalize * 1;
	m_AntiAliasingMask[2][0] = normalize * 1; m_AntiAliasingMask[2][1] = normalize * 1; m_AntiAliasingMask[2][2] = normalize * 1; m_AntiAliasingMask[2][3] = normalize * 1; m_AntiAliasingMask[2][4] = normalize * 1;
	m_AntiAliasingMask[3][0] = normalize * 1; m_AntiAliasingMask[3][1] = normalize * 1; m_AntiAliasingMask[3][2] = normalize * 1; m_AntiAliasingMask[3][3] = normalize * 1; m_AntiAliasingMask[3][4] = normalize * 1;
	m_AntiAliasingMask[4][0] = normalize * 1; m_AntiAliasingMask[4][1] = normalize * 1; m_AntiAliasingMask[4][2] = normalize * 1; m_AntiAliasingMask[4][3] = normalize * 1; m_AntiAliasingMask[4][4] = normalize * 1;

	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}


void CCGWorkView::OnUpdateAntialiasingTriangle3x3(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_TRIANGLE3X3);
}


void CCGWorkView::OnAntialiasingTriangle3x3()
{
	m_nAntiAliasing = ID_ANTIALIASING_TRIANGLE3X3;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_3X3;
	double normalize = 1.0 / 16;
	m_AntiAliasingMask[0][0] = normalize * 1; m_AntiAliasingMask[0][1] = normalize * 2; m_AntiAliasingMask[0][2] = normalize * 1;
	m_AntiAliasingMask[1][0] = normalize * 2; m_AntiAliasingMask[1][1] = normalize * 4; m_AntiAliasingMask[1][2] = normalize * 2;
	m_AntiAliasingMask[2][0] = normalize * 1; m_AntiAliasingMask[2][1] = normalize * 2; m_AntiAliasingMask[2][2] = normalize * 1;

	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}


void CCGWorkView::OnUpdateAntialiasingTriangle5x5(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_TRIANGLE5X5);
}


void CCGWorkView::OnAntialiasingTriangle5x5()
{
	m_nAntiAliasing = ID_ANTIALIASING_TRIANGLE5X5;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_5X5;
	double normalize = 1.0 / 81;
	m_AntiAliasingMask[0][0] = normalize * 1; m_AntiAliasingMask[0][1] = normalize * 2; m_AntiAliasingMask[0][2] = normalize * 3; m_AntiAliasingMask[0][3] = normalize * 2; m_AntiAliasingMask[0][4] = normalize * 1;
	m_AntiAliasingMask[1][0] = normalize * 2; m_AntiAliasingMask[1][1] = normalize * 4; m_AntiAliasingMask[1][2] = normalize * 6; m_AntiAliasingMask[1][3] = normalize * 4; m_AntiAliasingMask[1][4] = normalize * 2;
	m_AntiAliasingMask[2][0] = normalize * 3; m_AntiAliasingMask[2][1] = normalize * 6; m_AntiAliasingMask[2][2] = normalize * 9; m_AntiAliasingMask[2][3] = normalize * 6; m_AntiAliasingMask[2][4] = normalize * 3;
	m_AntiAliasingMask[3][0] = normalize * 2; m_AntiAliasingMask[3][1] = normalize * 4; m_AntiAliasingMask[3][2] = normalize * 6; m_AntiAliasingMask[3][3] = normalize * 4; m_AntiAliasingMask[3][4] = normalize * 2;
	m_AntiAliasingMask[4][0] = normalize * 1; m_AntiAliasingMask[4][1] = normalize * 2; m_AntiAliasingMask[4][2] = normalize * 3; m_AntiAliasingMask[4][3] = normalize * 2; m_AntiAliasingMask[4][4] = normalize * 3;
	
	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}


void CCGWorkView::OnUpdateAntialiasingGaussian3x3(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_GAUSSIAN3X3);
}


void CCGWorkView::OnAntialiasingGaussian3x3()
{
	m_nAntiAliasing = ID_ANTIALIASING_GAUSSIAN3X3;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_3X3;
	double normalize = 1.0 / 17;
	m_AntiAliasingMask[0][0] = normalize * 1; m_AntiAliasingMask[0][1] = normalize * 2; m_AntiAliasingMask[0][2] = normalize * 1;
	m_AntiAliasingMask[1][0] = normalize * 2; m_AntiAliasingMask[1][1] = normalize * 5; m_AntiAliasingMask[1][2] = normalize * 2;
	m_AntiAliasingMask[2][0] = normalize * 1; m_AntiAliasingMask[2][1] = normalize * 2; m_AntiAliasingMask[2][2] = normalize * 1;

	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}


void CCGWorkView::OnUpdateAntialiasingGaussian5x5(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_nAntiAliasing == ID_ANTIALIASING_GAUSSIAN5X5);
}


void CCGWorkView::OnAntialiasingGaussian5x5()
{
	m_nAntiAliasing = ID_ANTIALIASING_GAUSSIAN5X5;
	m_nAntiAliasingDim = ANTI_ALIASING_DIM_5X5;
	double normalize = 1.0 / 50;
	m_AntiAliasingMask[0][0] = normalize * 1; m_AntiAliasingMask[0][1] = normalize * 1; m_AntiAliasingMask[0][2] = normalize * 1;  m_AntiAliasingMask[0][3] = normalize * 1; m_AntiAliasingMask[0][4] = normalize * 1;
	m_AntiAliasingMask[1][0] = normalize * 1; m_AntiAliasingMask[1][1] = normalize * 2; m_AntiAliasingMask[1][2] = normalize * 4;  m_AntiAliasingMask[1][3] = normalize * 2; m_AntiAliasingMask[1][4] = normalize * 1;
	m_AntiAliasingMask[2][0] = normalize * 1; m_AntiAliasingMask[2][1] = normalize * 4; m_AntiAliasingMask[2][2] = normalize * 10; m_AntiAliasingMask[2][3] = normalize * 4; m_AntiAliasingMask[2][4] = normalize * 1;
	m_AntiAliasingMask[3][0] = normalize * 1; m_AntiAliasingMask[3][1] = normalize * 2; m_AntiAliasingMask[3][2] = normalize * 4;  m_AntiAliasingMask[3][3] = normalize * 2; m_AntiAliasingMask[3][4] = normalize * 1;
	m_AntiAliasingMask[4][0] = normalize * 1; m_AntiAliasingMask[4][1] = normalize * 1; m_AntiAliasingMask[4][2] = normalize * 1;  m_AntiAliasingMask[4][3] = normalize * 1; m_AntiAliasingMask[4][4] = normalize * 1;


	GetWindowRect(rect);

	m_WindowWidth = m_nAntiAliasingDim * (rect->right - rect->left - 4); // 4 is a shift from the edges of the window
	m_WindowHeight = m_nAntiAliasingDim * (rect->bottom - rect->top - 4);

	Invalidate();
}
