#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"
#include "cinder/ip/Resize.h"

#include "Kinect.h"

#define MESH_SIZE_X 640
#define MESH_SIZE_Y 480

using namespace ci;
using namespace ci::app;
using namespace ci::ip;
using namespace std;

class kinectBasicApp : public AppBasic {
	
  public:
	void prepareSettings( Settings* settings );
	void setup();
	void drawMesh();
	void drawRect(float x1, float y1, float x2, float y2);
	void calibrateBlock(float xCoord, float yCoord);
	void chooseBlockLocation(float xCoord, float yCoord);
	void findBlock();
	void mouseDown( MouseEvent event);
	void mouseDrag( MouseEvent event);
	void mouseUp( MouseEvent event );
	void keyDown( KeyEvent event );
	void keyUp( KeyEvent event );
	void update();
	void draw();
	
	// PARAMS
	params::InterfaceGl mParams;
	
	// CROP PARAMS
	int x1croptemp, y1croptemp, x2crop, y2crop;
	int x1crop, y1crop ;
	
	// BLOCK PARAMS
	int block_val;
	int block_x, block_y;
	
	// RENDER PARAMS
	int x_offset, y_offset;
	
	// RGB THRESHOLDS
	float r_thresh_upper;
	float r_thresh_lower;
	
	float g_thresh_upper;
	float g_thresh_lower;

	float b_thresh_upper;
	float b_thresh_lower;
	
	// KEY LISTENERS
	bool r_down;
	bool g_down;
	bool b_down;
	
	Kinect			mKinect;
	gl::Texture		mColorTexture, mDepthTexture, mColorDepthTexture; 
	Surface			mColorDepth;
};

void kinectBasicApp::prepareSettings( Settings* settings )
{
	settings->setWindowSize( 1280, 480 );
	mColorDepth = Surface(640,480,true);
	
	r_thresh_upper = 90;
	r_thresh_lower = 84;
	
	g_thresh_upper = 88;
	g_thresh_lower = 82;
	
	b_thresh_upper = 82;
	b_thresh_lower = 80;
	
	r_down = false;
	g_down = false;
	b_down = false;
	
	x1croptemp = 0;
	y1croptemp = 0;
	x1crop = 0;
	y1crop = 0;
	x2crop = 640,
	y2crop = 480;
	
	//TODO
	block_val = 83;
	block_x = 50;
	block_y = 50;
	
	x_offset = 0;
	y_offset = 0;
}

void kinectBasicApp::setup()
{
	mParams = params::InterfaceGl( "Sandscape", Vec2i( 200, 180 ) );
	//increment top and bottom boundaries by negative amounts to put origin at bottom left
	mParams.addParam( "Red", &r_thresh_upper, "min=0.0 max=200.0 step=10.0 keyIncr=s keyDecr=w");
	mParams.addParam( "top boundary", &y1crop, "min=0.0 max=480.0 step=-5.0 keyIncr=s keyDecr=w");
	mParams.addParam( "left boundary", &x1crop, "min=0.0 max=640.0 step=5.0 keyIncr=s keyDecr=w");
	mParams.addParam( "bottom boundary", &y2crop, "min=0.0 max=480.0 step=-5.0 keyIncr=s keyDecr=w");
	mParams.addParam( "right boundary", &x2crop, "min=0.0 max=640.0 step=5.0 keyIncr=s keyDecr=w");
	mParams.addParam( "block x", &block_x, "min=0.0 max=640.0 step=5.0 keyIncr=s keyDecr=w");
	mParams.addParam( "block y", &block_y, "min=0.0 max=640.0 step=5.0 keyIncr=s keyDecr=w");
	
	
	console() << "There are " << Kinect::getNumDevices() << " Kinects connected." << std::endl;

	mKinect = Kinect( Kinect::Device() ); // the default Device implies the first Kinect connected


}

void kinectBasicApp::mouseDown( MouseEvent event )
{
	x1croptemp = event.getX();
	y1croptemp = event.getY();
	

}

void kinectBasicApp::mouseDrag( MouseEvent event) 
{

	x1crop = x1croptemp;
	y1crop = y1croptemp;
	x2crop = event.getX();
	y2crop = event.getY();
	
	x2crop = (x2crop < 0 ? 0 : x2crop);
	x2crop = (x2crop > 640 ? 640 : x2crop);
	y2crop = (y2crop < 0 ? 0 : y2crop);
	y2crop = (y2crop > 480 ? 480 : y2crop);


	

}

void kinectBasicApp::mouseUp( MouseEvent event )
{
	//writeImage( getHomeDirectory() + "kinect_video.png", mKinect.getVideoImage() );
	//writeImage( getHomeDirectory() + "kinect_depth.png", mKinect.getDepthImage() );
	
	// set tilt to random angle
//	mKinect.setTilt( Rand::randFloat() * 62 - 31 );

	// toggle infrared video
	//mKinect.setVideoInfrared( ! mKinect.isVideoInfrared() );

	
	x1crop = x1croptemp;
	y1crop = y1croptemp;
	x2crop = event.getX();
	y2crop = event.getY();
	

	// preserve the invariant that x1crop < x2crop, for UI purposes
	//TODO: if dragged inside gui, is not accurate
	x1crop = (x1croptemp < event.getX() ? x1croptemp : event.getX());
	y1crop = (y1croptemp < event.getY() ? y1croptemp : event.getY());
	x2crop = (x1croptemp < event.getX() ? event.getX() : x1croptemp);
	y2crop = (y1croptemp < event.getY() ? event.getY() : y1croptemp);
		

	
}

void kinectBasicApp::update()
{	
	if( mKinect.checkNewDepthFrame() ) {
		mDepthTexture = mKinect.getDepthImage();
		
		//Surface croppedColorDepth = Surface(x2crop-x1crop, y2crop-y1crop, true);
		//Area area = (0,0,640,480);
		mColorDepth = mKinect.getDepthImage();

		Area area( 0, 0, 640, 480 );
		Area cropArea( x1crop, y1crop, x2crop, y2crop );
		// draw crop area
		drawRect(x1crop, y1crop, x2crop, y2crop);

		
		// map colors
		Surface::Iter iter = mColorDepth.getIter( area );
		while( iter.line() ) {
			while( iter.pixel() ) {
				float val = iter.r();				
				iter.r() = 0;
				iter.b() = 0;
				iter.g() = 0;
				
				// increase from 0
				if (val >= r_thresh_lower && val <= r_thresh_upper ) {
					float r = (val - r_thresh_lower) / (r_thresh_upper - r_thresh_lower);
					iter.r() = r * 255;
				}
				// triangle function
				if (val >= g_thresh_lower && val <= g_thresh_upper) {
					//float g = (val - g_thresh_lower) / (g_thresh_upper - g_thresh_lower);
					//iter.g() = g * 255;
					
				    int g_thresh_mid = (g_thresh_upper + g_thresh_lower) / 2;
					// increase from 0, lower half of green spectrum
					if (val <= g_thresh_mid) {
						float g = (val - g_thresh_mid) / (g_thresh_mid - g_thresh_lower);
						iter.g() = g * 255;
					}
					// decrease to 0, upper half of green spectrum
					if (val >= g_thresh_mid) {
						float g = (val - g_thresh_mid) / (g_thresh_upper - g_thresh_mid);
						iter.g() = abs(255 - g * 255);
					}
				}
				// decrease to 0
				if (val >= b_thresh_lower && val <= b_thresh_upper) {
					float b = (val - b_thresh_lower) / (b_thresh_upper - b_thresh_lower);
					iter.b() = abs(255 - b * 255);
				}
			}
		}
		
		
		float height_block_area = (float)*mColorDepth.getDataRed  (Vec2i(block_x,block_y)) / 255.0f;
				
		gl::color(255,0,0);
		gl::drawStrokedCircle(Vec2f(block_x,block_y), 10);
		
		
		if (block_val == height_block_area) {
			gl::color(255,0,0);
			gl::drawStrokedCircle(Vec2f(block_x + 640,block_y), 10);
		}
		
		float cropAspectRatio = (float)cropArea.getWidth() / (float)cropArea.getHeight();
		
		
		int dim_x = 640;
		int dim_y = 480;
		
		float resize_ratio;
		
		// if wider aspect ratio, constrained by x
		if (cropAspectRatio > (640.0/480.0)) {
			resize_ratio = 640.0/cropArea.getWidth();
			dim_y = (int) cropArea.getHeight() * resize_ratio;
		} 
		// if taller aspect ratio, constrained by y
		else {
			resize_ratio = 480.0/cropArea.getHeight();
			dim_x = (int) cropArea.getWidth() * resize_ratio;
		}
		
		// center the image
		x_offset = (640 - dim_x)/2;
		y_offset = (480 - dim_y)/2;
		
		Surface mColorDepthCopy(cropArea.getWidth(), cropArea.getHeight(), true) ;
		mColorDepthCopy.copyFrom(mColorDepth, cropArea, Vec2i(-cropArea.getX1(), -cropArea.getY1()));
		
		Surface mColorDepthResize(dim_x,dim_y,true);
		ip::resize(mColorDepthCopy, &mColorDepthResize);
		
		mColorDepthTexture = gl::Texture( mColorDepthResize );
	}
	if( mKinect.checkNewVideoFrame() )
		mColorTexture = mKinect.getVideoImage();
	
}

void kinectBasicApp::drawMesh() {
	glPushMatrix();
	
	//glRotated(-1, 1, 0, 0);
	
	glBegin( GL_QUADS );  
	
	Area area( 0, 0, 640, 480 );
	
	for (int x = 0; x < MESH_SIZE_X - 1 ; x ++ ) {
		for (int y = 0; y < MESH_SIZE_Y -1; y ++ )	{
	
			float height = (float)*mColorDepth.getDataRed  (Vec2i(x,y)) / 255.0f;
			
			glColor4f(height, height, height, 1.0f);
			glVertex3i(x, y, height);
	
			glVertex3i(x, y+1, height);
	
			glVertex3i(x+1, y+1, height);
	
			glVertex3i(x+1, y, height);
		}
	}

	glEnd();
	glPopMatrix();
}

void kinectBasicApp::drawRect(float x1, float y1, float x2, float y2) {
	gl::color(255,255,255);
	
	//draw four lines
	gl::drawLine(Vec2f(x1, y1), Vec2f(x1,y2));
	gl::drawLine(Vec2f(x1, y1), Vec2f(x2,y1));
	gl::drawLine(Vec2f(x1, y2), Vec2f(x2,y2));
	gl::drawLine(Vec2f(x2, y1), Vec2f(x2,y2));
}

// calibrates block for choosing mode
void kinectBasicApp::calibrateBlock(float xCoord, float yCoord) {
	block_val = 83;
}

// choose block locations
void kinectBasicApp::chooseBlockLocation(float xCoord, float yCoord) {
	block_x = xCoord;
	block_y = yCoord;
}


// draws a circle where the block currently is, in the second frame
void kinectBasicApp:: findBlock() {

}


void kinectBasicApp::keyDown( KeyEvent event ) {
	if (event.getChar() == 'r')
		r_down = true;
	if (event.getChar() == 'g')
		g_down = true;
	if (event.getChar() == 'b')
		b_down = true;
	
	//TODO: change for uniformity, unless in "manual" mode...
	//adjust "spacing", "height", and "blend"
	
	// adjust range
	if (event.getCode() == KeyEvent::KEY_LEFT ) {
		if (r_down == true) {
			if (r_thresh_upper - r_thresh_lower > 1) {
				r_thresh_upper --;
				r_thresh_lower ++;
			}
		}
		if (g_down == true) {
			if (g_thresh_upper - g_thresh_lower > 1) {
				g_thresh_upper --;
				g_thresh_lower ++;
			}
		}
		if (b_down == true) {
			if (b_thresh_upper - b_thresh_lower > 1) {
				b_thresh_upper --;
				b_thresh_lower ++;
			}
		}
	}
	if (event.getCode() == KeyEvent::KEY_RIGHT ) {
		if (r_down == true) {
			r_thresh_upper ++;
			r_thresh_lower --;
		}
		if (g_down == true) {
			g_thresh_upper ++;
			g_thresh_lower --;
		}
		if (b_down == true) {
			b_thresh_upper ++;
			b_thresh_lower --;
		}
	}
	
	// adjust level
	if (event.getCode() == KeyEvent::KEY_UP ) {
		if (r_down == true) {
			r_thresh_upper ++;
			r_thresh_lower ++;
		}
		if (g_down == true) {
			g_thresh_upper ++;
			g_thresh_lower ++;
		}
		if (b_down == true) {
			b_thresh_upper ++;
			b_thresh_lower ++;
		}
	}
	if (event.getCode() == KeyEvent::KEY_DOWN ) {
		if (r_down == true) {
			r_thresh_upper --;
			r_thresh_lower --;
		}
		if (g_down == true) {
			g_thresh_upper --;
			g_thresh_lower --;
		}
		if (b_down == true) {
			b_thresh_upper --;
			b_thresh_lower --;
		}
	}	
	
	
}

void kinectBasicApp::keyUp( KeyEvent event ) {
	if (event.getChar() == 'r')
		r_down = false;
	if (event.getChar() == 'g')
		g_down = false;
	if (event.getChar() == 'b')
		b_down = false;
}

void kinectBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	//if( mDepthTexture ) {
	//	gl::draw( mDepthTexture );
	//}
	if (mColorDepthTexture) {
		gl::draw( mColorDepthTexture, Vec2i( 640 + x_offset, 0 + y_offset ) );
	}
	
	drawMesh();
	drawRect(x1crop, y1crop, x2crop, y2crop);


	params::InterfaceGl::draw();
}


CINDER_APP_BASIC( kinectBasicApp, RendererGl )
