#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Camera.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/DisplayList.h"
#include "cinder/gl/Material.h"

#include "cinder/params/Params.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static const Vec3f	CAM_POSITION_INIT( 0.0f, 0.0f, -21.0f);
static const Vec3f	LIGHT_POSITION_INIT( 0.0f, 4.0f, 0.0f );
static const int	SHADOW_MAP_RESOLUTION = 1024;

class Base_ThreeD_ProjectApp : public AppBasic 
{
	public:
		Base_ThreeD_ProjectApp();
		virtual ~Base_ThreeD_ProjectApp();
		void prepareSettings( Settings *settings );
		
		void setup();
		void update();
		void draw();
		
		void mouseDown( MouseEvent event );	
		void keyDown( app::KeyEvent event ); 
	
		void drawTestObjects();
    
        void initShaders();
        void initFBOs();
        void createShadowMap();
	
	protected:
	
		//debug
		cinder::params::InterfaceGl mParams;
		bool				mShowParams;
		float				mCurrFramerate;
	
		//objects
		gl::DisplayList		mTorus, mBoard, mBox, mSphere;
	
		//camera
		CameraPersp			*mCam;
		Vec3f				mEye;
		Vec3f				mCenter;
		Vec3f				mUp;
		float				mCameraDistance;
	
		//light
		gl::Light			*mLight;
    
        //FBO that holds depth information for shadow mapping
        gl::Fbo				mDepthFbo;
    
        gl::GlslProg		mTexAndShadowShader;
    
        gl::Texture			mEarthTexture;
	
	public:
};

Base_ThreeD_ProjectApp::Base_ThreeD_ProjectApp()
{}

Base_ThreeD_ProjectApp::~Base_ThreeD_ProjectApp()
{
	delete mCam;
	delete mLight;
}

void Base_ThreeD_ProjectApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 720, 486 );		
	settings->setFrameRate( 60.0f );			//the more the merrier!
	settings->setResizable( false );			//this isn't going to be resizable
	
	//make sure secondary screen isn't blacked out as well when in fullscreen mode ( do wish it could accept keyboard focus though :(
	//settings->enableSecondaryDisplayBlanking( false );
}

void Base_ThreeD_ProjectApp::setup()
{
	glEnable( GL_LIGHTING );
	glEnable( GL_DEPTH_TEST );
	glEnable(GL_RESCALE_NORMAL);
//	glEnable(GL_POLYGON_SMOOTH);
//	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
//	glEnable(GL_COLOR_MATERIAL);
	
	mParams = params::InterfaceGl( "3D_Scene_Base", Vec2i( 225, 250 ) );
	mParams.addParam( "Framerate", &mCurrFramerate, "", true );
	mParams.addParam( "Eye Distance", &mCameraDistance, "min=-100.0 max=-5.0 step=1.0 keyIncr== keyDecr=-");
	mParams.addParam( "Show/Hide Params", &mShowParams, "key=x");
	
	mCurrFramerate = 0.0f;
	mShowParams = true;
	
	//create camera
	mCameraDistance = CAM_POSITION_INIT.z;
	mEye		= Vec3f(CAM_POSITION_INIT.x, CAM_POSITION_INIT.y, CAM_POSITION_INIT.z);
	mCenter		= Vec3f::zero();
	mUp			= Vec3f::yAxis();
	
	mCam = new CameraPersp( getWindowWidth(), getWindowHeight(), 180.0f );
	mCam->lookAt(mEye, mCenter, mUp);
	mCam->setPerspective( 45.0f, getWindowAspectRatio(), 1.0f, 50.0f );
	gl::setMatrices( *mCam );
	
	//create light
	mLight = new gl::Light( gl::Light::DIRECTIONAL, 0 );
	mLight->lookAt( Vec3f(LIGHT_POSITION_INIT.x, LIGHT_POSITION_INIT.y, LIGHT_POSITION_INIT.z), Vec3f( 0, 0, 0 ) );
	mLight->setAmbient( Color( 1.0f, 1.0f, 1.0f ) );
	mLight->setDiffuse( Color( 1.0f, 1.0f, 1.0f ) );
	mLight->setSpecular( Color( 1.0f, 1.0f, 1.0f ) );
	mLight->setShadowParams( 100.0f, 1.0f, 20.0f );
	mLight->update( *mCam );
	mLight->enable();
	
	//DEBUG Test objects
	ci::ColorA pink( CM_RGB, 0.84f, 0.49f, 0.50f, 1.0f );
	ci::ColorA green( CM_RGB, 0.39f, 0.78f, 0.64f, 1.0f );
	ci::ColorA blue( CM_RGB, 0.32f, 0.59f, 0.81f, 1.0f );
	ci::ColorA orange( CM_RGB, 0.77f, 0.35f, 0.35f, 1.0f );
	
	gl::Material torusMaterial;
	torusMaterial.setSpecular( ColorA( 1.0, 1.0, 1.0, 1.0 ) );
	torusMaterial.setDiffuse( pink );
	torusMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	torusMaterial.setShininess( 25.0f );
	
	gl::Material boardMaterial;
	boardMaterial.setSpecular( ColorA( 0.0, 0.0, 0.0, 0.0 ) );
	boardMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	boardMaterial.setDiffuse( green );	
	boardMaterial.setShininess( 0.0f );
	
	gl::Material boxMaterial;
	boxMaterial.setSpecular( ColorA( 0.0, 0.0, 0.0, 0.0 ) );
	boxMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	boxMaterial.setDiffuse( blue );	
	boxMaterial.setShininess( 0.0f );
	
	gl::Material sphereMaterial;
	sphereMaterial.setSpecular( ColorA( 1.0, 1.0, 1.0, 1.0 ) );
	sphereMaterial.setAmbient( ColorA( 0.3, 0.3, 0.3, 1.0 ) );
	sphereMaterial.setDiffuse( orange ) ;	
	sphereMaterial.setShininess( 35.0f );	
	
	mTorus = gl::DisplayList( GL_COMPILE );
	mTorus.newList();
	gl::drawTorus( 1.0f, 0.3f, 32, 64 );
	mTorus.endList();
	mTorus.setMaterial( torusMaterial );
	
	mBoard = gl::DisplayList( GL_COMPILE );
	mBoard.newList();
	gl::drawCube( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 10.0f, 0.1f, 10.0f ) );
	mBoard.endList();
	mBoard.setMaterial( boardMaterial );
	
	mBox = gl::DisplayList( GL_COMPILE );
	mBox.newList();
	gl::drawCube( Vec3f( 0.0f, 0.0f, 0.0f ), Vec3f( 1.0f, 1.0f, 1.0f ) );
	mBox.endList();
	mBox.setMaterial( boxMaterial );
	
	mSphere = gl::DisplayList( GL_COMPILE );
	mSphere.newList();
	gl::drawSphere( Vec3f::zero(), 0.8f, 30 );
	mSphere.endList();
	mSphere.setMaterial( sphereMaterial );
    
    mEarthTexture	= gl::Texture( loadImage( loadResource( EARTH_TEX ) ) );
    
    initShaders();
    initFBOs();
    createShadowMap();
}

void Base_ThreeD_ProjectApp::update()
{
	mCurrFramerate = getAverageFps();
	
    glEnable( GL_LIGHTING );
}

void Base_ThreeD_ProjectApp::draw()
{
	glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClearDepth(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glDisable(GL_LIGHTING);
	glColor3f( 1.0f, 1.0f, 0.1f );
    gl::drawFrustum( mLight->getShadowCamera() );
	glColor3f( 1.0f, 1.0f, 1.0f );
    glEnable(GL_LIGHTING);
	

    mEye = mCam->getEyePoint();
    mEye.normalize();
    mEye = mEye * abs(mCameraDistance);
    mCam->lookAt( mEye, mCenter, mUp );
    gl::setMatrices( *mCam );

	mLight->update( *mCam );
	
	//primary pass to create shadowmap ( draw objects that will cast shadows in here )
    createShadowMap();
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	drawTestObjects();
	glDisable(GL_CULL_FACE);
	
	if (mShowParams)
		params::InterfaceGl::draw();
}

void Base_ThreeD_ProjectApp::mouseDown( MouseEvent event )
{}

//just moving light and camera using keys
void Base_ThreeD_ProjectApp::keyDown( app::KeyEvent event ) 
{
	switch ( event.getCode() ) 
	{
		case 273:
			//up
		{
			Vec3f lightPos = mLight->getPosition();
			lightPos = lightPos + Vec3f(0, 0.0, 0.1);
			mLight->lookAt( lightPos, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;
		case 274:
			//down
		{
			Vec3f lightPos = mLight->getPosition();
			lightPos = lightPos + Vec3f(0, 0.0, -0.1);
			mLight->lookAt( lightPos, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;
		case 276:
			//left
		{
			Vec3f lightPos = mLight->getPosition();
			lightPos = lightPos + Vec3f(0.1, 0, 0);
			mLight->lookAt( lightPos, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;
		case 275:
			//right
		{
			Vec3f lightPos = mLight->getPosition();
			lightPos = lightPos + Vec3f(-0.1, 0, 0);
			mLight->lookAt( lightPos, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;
		case 119:
			//W
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(1, 0, 0), -0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;
		case 97:
			//A
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(0, 1, 0), 0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;	
		case 115:
			//S
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(1, 0, 0), 0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;
		case 100:
			//D
		{
			mEye = mCam->getEyePoint();
			mEye = Quatf( Vec3f(0, 1, 0), -0.03f ) * mEye;
			mCam->lookAt( mEye, Vec3f::zero() );
			mLight->update( *mCam );
		}
			break;
		case 48:
			//0
		{
			//reset everything
			
		}
			break;	
		default:
			break;
	}
}

void Base_ThreeD_ProjectApp::drawTestObjects()
{
    //bind diffuse texture
    mEarthTexture.bind(1);
    
    //bind depth texture
    gl::enableDepthRead();
    mDepthFbo.bindDepthTexture();
    
    mTexAndShadowShader.bind();
    mTexAndShadowShader.uniform("LightPosition", mLight->getPosition() );
    mTexAndShadowShader.uniform( "texture", 1 );
    mTexAndShadowShader.uniform( "shadowTransMatrix", mLight->getShadowTransformationMatrix( *mCam ) );
    
	//glColor3f(1.0, 0.2, 0.2);
	gl::pushMatrices();
	glTranslatef(-2.0f, -1.0f, 0.0f);
	glRotated(90.0f, 1, 0, 0);
		mTorus.draw();
	gl::popMatrices();
	
	//glColor3f(0.4, 1.0, 0.2);
	gl::pushMatrices();
	glTranslatef(0.0f, -1.35f, 0.0f);
		mBoard.draw();
	gl::popMatrices();
	
	//glColor3f(0.8, 0.5, 0.2);
	gl::pushMatrices();
	glTranslatef(0.4f, -0.3f, 0.5f);
	glScalef(2.0f, 2.0f, 2.0f);
		mBox.draw();
	gl::popMatrices();
	
	//glColor3f(0.3, 0.5, 0.9);
	gl::pushMatrices();
	glTranslatef(0.1f, -0.56f, -1.25f);
		mSphere.draw();
	gl::popMatrices();
    
    mTexAndShadowShader.unbind();
    mEarthTexture.unbind();
}

void Base_ThreeD_ProjectApp::initShaders()
{
	mTexAndShadowShader = gl::GlslProg( loadResource( TEXTURE_AND_SHAD_VERT ), loadResource( TEXTURE_AND_SHAD_FRAG ) );
}

void Base_ThreeD_ProjectApp::initFBOs()
{
	//init shadow map
	mDepthFbo = gl::Fbo( SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, false, false, true );
    
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
}

void Base_ThreeD_ProjectApp::createShadowMap()
{
	gl::enableDepthWrite();
	
	//create depth map and bind to a FBO ( from light's perspective )
	mDepthFbo.bindFramebuffer();
    
    glEnable( GL_POLYGON_OFFSET_FILL );
    glClear( GL_DEPTH_BUFFER_BIT );
    
    glPushAttrib( GL_VIEWPORT_BIT );
    glViewport( 0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION );
    
    mLight->setShadowRenderMatrices();
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    drawTestObjects();
    glDisable(GL_CULL_FACE);
    
    glPopAttrib();
    
    glDisable( GL_POLYGON_OFFSET_FILL );
	
	mDepthFbo.unbindFramebuffer();
	
	glEnable( GL_TEXTURE_2D );
	
	gl::setMatrices( *mCam );
	mLight->update( *mCam );
}

CINDER_APP_BASIC( Base_ThreeD_ProjectApp, RendererGl )
