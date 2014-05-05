/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 *
 * simsys_opengl.cc
 * OpenGL backend
 * Copyright (c) 2012 Ters, Markohs
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <SDL.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include <stdio.h>
#include <vector>

#include "macros.h"
#include "simsys_w32_png.h"
#include "simversion.h"
#include "simsys.h"
#include "simevent.h"
#include "display/simgraph.h"
#include "simdebug.h"

static Uint8 hourglass_cursor[] = {
	0x3F, 0xFE, //   *************
	0x30, 0x06, //   **         **
	0x3F, 0xFE, //   *************
	0x10, 0x04, //    *         *
	0x10, 0x04, //    *         *
	0x12, 0xA4, //    *  * * *  *
	0x11, 0x44, //    *  * * *  *
	0x18, 0x8C, //    **   *   **
	0x0C, 0x18, //     **     **
	0x06, 0xB0, //      ** * **
	0x03, 0x60, //       ** **
	0x03, 0x60, //       **H**
	0x06, 0x30, //      ** * **
	0x0C, 0x98, //     **     **
	0x18, 0x0C, //    **   *   **
	0x10, 0x84, //    *    *    *
	0x11, 0x44, //    *   * *   *
	0x12, 0xA4, //    *  * * *  *
	0x15, 0x54, //    * * * * * *
	0x3F, 0xFE, //   *************
	0x30, 0x06, //   **         **
	0x3F, 0xFE  //   *************
};

static Uint8 hourglass_cursor_mask[] = {
	0x3F, 0xFE, //   *************
	0x3F, 0xFE, //   *************
	0x3F, 0xFE, //   *************
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x0F, 0xF8, //     *********
	0x07, 0xF0, //      *******
	0x03, 0xE0, //       *****
	0x03, 0xE0, //       **H**
	0x07, 0xF0, //      *******
	0x0F, 0xF8, //     *********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x1F, 0xFC, //    ***********
	0x3F, 0xFE, //   *************
	0x3F, 0xFE, //   *************
	0x3F, 0xFE  //   *************
};

#define RMASK 0xf800
#define GMASK 0x7e0
#define BMASK 0x1f
#define AMASK 0

static SDL_Surface *screen;
static SDL_Surface *texture;
static int width = 16;
static int height = 16;

// switch on is a little faster (<3%)
static int async_blit = 0;

static SDL_Cursor* arrow;
static SDL_Cursor* hourglass;

static int pbo_index = 0;
static GLuint pbo_ids[2];
static GLuint pbo_textures[2];
static GLuint gl_texture = 0;
static bool npot_able,pbo_able;
static int tex_max_size;
static int tex_w,tex_h;
static int n_tex_w,n_tex_h;
static float x_max_coord;
static float y_max_coord;
static void* pixels;

static bool tiling_in_use=false;
struct tiled_texture{
	GLuint gl_texture;
	float left;
	float right;
	float top;
	float bottom;
};
static std::vector<std::vector<tiled_texture> > tiled_textures;

#ifdef DEBUG
static inline void opengl_error(){
	int error = glGetError();

	switch (error){
	case GL_NO_ERROR:
		break;
	case GL_INVALID_ENUM:
		DBG_MESSAGE("opengl_error()","GL_INVALID_ENUM");
		break;
	case GL_INVALID_VALUE:
		DBG_MESSAGE("opengl_error()","GL_INVALID_VALUE");
		break;
	case GL_INVALID_OPERATION:
		DBG_MESSAGE("opengl_error()","GL_INVALID_OPERATION");
		break;
	case GL_STACK_OVERFLOW:
		DBG_MESSAGE("opengl_error()","GL_STACK_OVERFLOW");
		break;
	case GL_STACK_UNDERFLOW:
		DBG_MESSAGE("opengl_error()","GL_STACK_UNDERFLOW");
		break;
	case GL_OUT_OF_MEMORY:
		DBG_MESSAGE("opengl_error()","GL_OUT_OF_MEMORY");
		break;
	case GL_TABLE_TOO_LARGE:
		DBG_MESSAGE("opengl_error()","GL_TABLE_TOO_LARGE");
		break;

	}
}
#else
static inline void opengl_error(){
}
#endif

/**
 * Returns the lowest pot (power of two) number higher to the parameter passed
 * @param number the number to approx to
 * @return lowest pot number higher to the parameter passed
 */
static int closest_pot(const int number){

	int result = 16;

	while (result<number)
		result=result<<1;

	return result;
}


/**
 * resizes texture size to the viewport one
 */
static void update_tex_dims(){

	if (npot_able){
		tex_w = screen->w;
		tex_h = screen->h;
		x_max_coord=1.0f;
		y_max_coord=1.0f;
	}
	else{
		tex_w =	closest_pot(screen->w);
		tex_h = closest_pot(screen->h);
		x_max_coord= (float)screen->w/(float)tex_w;
		y_max_coord= (float)screen->h/(float)tex_h;
	}

	// check if we exceded maximum texture size

	if (max(tex_w,tex_h)>tex_max_size){

		tiling_in_use=true;

		n_tex_w = (screen->w + (tex_max_size-1))/tex_max_size;
		n_tex_h = (screen->h + (tex_max_size-1))/tex_max_size;

		x_max_coord = (float)(screen->w%tex_max_size)/(float)tex_max_size;
		y_max_coord = (float)(screen->w%tex_max_size)/(float)tex_max_size;

		// Disable Pixel Buffer Objects

		pbo_able = false;

	}
	else
		tiling_in_use=false;
}


/**
 * Checks for the extensions this backends can use, GL_ARB_pixel_buffer_object and
 * GL_ARB_texture_non_power_of_two, and sets the global variables npot_able and pbo_able
 */
static void check_for_extensions(){

	// Initialize GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err){
		return;
	}

	npot_able=GLEW_ARB_texture_non_power_of_two;

	if (npot_able){
		fprintf(stderr, "Renderer is NPOT able.\n");
		DBG_MESSAGE("check_for_extensions(OpenGL)", "Renderer does support NPOT extension");
	}
	else{
		fprintf(stderr, "Renderer is not NPOT able.\n");
		DBG_MESSAGE("check_for_extensions(OpenGL)", "Renderer does NOT support NPOT extension");
	}

	// Now, GL_ARB_pixel_buffer_object

	pbo_able = GLEW_ARB_pixel_buffer_object;

	if (pbo_able){
		fprintf(stderr, "Renderer is PBO able.\n");
		DBG_MESSAGE("check_for_extensions(OpenGL)", "Renderer does support PBO extension");
	}
	else{
		fprintf(stderr, "Renderer is not PBO able.\n");
		DBG_MESSAGE("check_for_extensions(OpenGL)", "Renderer does NOT support PBO extension");
	}
}


/**
 * Detects the biggest texture the system supports, and sets tex_max_size
 */
static void check_max_texture_size(){

	GLint width,curr_width;

	curr_width = 32;

	do{
		curr_width=curr_width<<1;

		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGB, curr_width, curr_width, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	}
	while (width!=0 && width==curr_width);

	curr_width=curr_width>>1;
	tex_max_size=curr_width;

	fprintf(stderr, "Renderer supports textures up to %dx%d.\n",curr_width,curr_width);
	DBG_MESSAGE("check_max_texture_size(OpenGL)", "Renderer supports textures up to %dx%d",curr_width,curr_width);
}


/**
 * Detects if we have hardware acceleration available or not
 */
static bool check_hardware_accelerated(){

	int result;
	const GLubyte* vendor = glGetString(GL_VENDOR);

	SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL,&result);

	if (result==1){
		fprintf(stderr, "Hardware acceleration available, vendor: %s.\n",vendor);
		DBG_MESSAGE("check_hardware_accelerated(OpenGL)", "Hardware acceleration available, vendor: %s",vendor);
	}
	else{
		fprintf(stderr, "Hardware acceleration NOT available, vendor: %s.\n",vendor);
		DBG_MESSAGE("check_hardware_accelerated(OpenGL)", "Hardware acceleration NOT available, vendor: %s",vendor);
	}
	return result==1;
}

/**
 * Queue a texture update with the PBO from the old frame, and unlock the new one.
 */
void inline pbo_map(){

	// Update the texture with the old PBO

	GLuint pbo_oldindex=pbo_index;

	pbo_index = (pbo_index + 1) % 2;

	glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_w);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	//glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	//glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	glBindTexture(GL_TEXTURE_2D, pbo_textures[pbo_oldindex]);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_ids[pbo_oldindex]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);

	// Unlock the new PBO for writing

	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_ids[pbo_index]);
	pixels = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,GL_READ_WRITE_ARB);

	reset_textur(pixels);

	opengl_error();
}

/**
 * Releases the lock on the PBO
 */
void inline pbo_unmap(){
	glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	opengl_error();

	pixels=NULL;
	reset_textur(pixels);
}


/*
 * Hier sind die Basisfunktionen zur Initialisierung der
 * Schnittstelle untergebracht
 * -> init,open,close
 */
bool dr_os_init(const int* parameter)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) != 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		return false;
	}

	async_blit = parameter[0];

	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	atexit(SDL_Quit); // clean up on exit
	return true;
}


resolution dr_query_screen_resolution()
{
	resolution res;
#if SDL_VERSION_ATLEAST(1, 2, 10)
	SDL_VideoInfo const& vi = *SDL_GetVideoInfo();
	res.w = vi.current_w;
	res.h = vi.current_h;
#elif defined _WIN32
	res.w = GetSystemMetrics(SM_CXSCREEN);
	res.h = GetSystemMetrics(SM_CYSCREEN);
#else
	SDL_Rect** const modi = SDL_ListModes(0, SDL_FULLSCREEN);
	if (modi && modi != (SDL_Rect**)-1) {
		// return first
		res.w = modi[0]->w;
		res.h = modi[0]->h;
	} else {
		res.w = 704;
		res.h = 560;
	}
#endif
	return res;
}


/**
 * Opens the SDL window with the requested dimensions, calls display_set_actual_width with the
 * requested with, but returns the screen pitch, simgraphxx.cc will handle this
 * @param w Desired width
 * @param h Desired height
 * @param fullscreen true if full screen is requested.
 */
int dr_os_open(int w, int const h, int const fullscreen)
{
	Uint32 flags = async_blit ? SDL_ASYNCBLIT : 0;

	// some cards need those alignments
	// especially 64bit want a border of 8bytes
	w = (w + 15) & 0x7FF0;
	if(  w<=0  ) {
		w = 16;
	}

	width = w;
	height = h;

	flags |= (fullscreen ? SDL_FULLSCREEN : SDL_RESIZABLE);
	flags |= SDL_OPENGL;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// open the window now
	screen = SDL_SetVideoMode(w, h, COLOUR_DEPTH, flags);
	if (screen == NULL) {
		fprintf(stderr, "Couldn't open the window: %s\n", SDL_GetError());
		return 0;
	}
	else {
		fprintf(stderr, "Screen Flags: requested=%x, actual=%x\n", flags, screen->flags);
	}
	DBG_MESSAGE("dr_os_open(OpenGL)", "SDL realized screen size width=%d, height=%d (requested w=%d, h=%d)", screen->w, screen->h, w, h);

	SDL_EnableUNICODE(true);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	// set the caption for the window
	SDL_WM_SetCaption(SIM_TITLE, 0);
	SDL_ShowCursor(0);
	arrow = SDL_GetCursor();
	hourglass = SDL_CreateCursor(hourglass_cursor, hourglass_cursor_mask, 16, 22, 8, 11);

	glEnable(GL_TEXTURE_2D);

	check_for_extensions();
	check_max_texture_size();
	if (!check_hardware_accelerated()){
		DBG_MESSAGE("dr_os_open(OpenGL)", "No hardware renderer available, exiting...");
		fprintf(stderr, "No hardware renderer available, exiting...");
		return 0;
	}

	update_tex_dims();

	display_set_actual_width( w );
	return tex_w;
}


// shut down SDL
void dr_os_close()
{
	SDL_FreeCursor(hourglass);
	// Hajo: SDL doc says, screen is free'd by SDL_Quit and should not be
	// free'd by the user
	// SDL_FreeSurface(screen);
}


/**
 * Creates a OpenGL texture, sized as tex_w x tex_h, using 16-bit depth.
 * On tiling we'll create enough textures to represent the screen,
 * on non-pbo, we'll create a single texture, and on pbo, we'll create two
 */
static void create_gl_texture()
{
	if (!tiling_in_use){

		if(!pbo_able){
			opengl_error();
			glGenTextures(1, &gl_texture);
			opengl_error();

			glBindTexture(GL_TEXTURE_2D, gl_texture);
			opengl_error();
			// No mipmapping
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			opengl_error();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			opengl_error();
			glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_w);
			opengl_error();
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			opengl_error();

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
		}
		else{

			glGenTextures(2, &pbo_textures[0]);

			glBindTexture(GL_TEXTURE_2D, pbo_textures[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_w);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);

			glBindTexture(GL_TEXTURE_2D, pbo_textures[1]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_w);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);

			const bool was_mapped = pixels;

			if(was_mapped)
				pbo_unmap();

			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo_ids[pbo_index]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);

			if(was_mapped)
				pbo_map();
		}
		opengl_error();
	}
	else{
		if(  tiled_textures.empty()  ){

			// Initial call

			tiled_textures.resize(n_tex_w);

			for (int x=0;x<n_tex_w;x++){

				tiled_textures[x].resize(n_tex_h);

				for (int y=0;y<n_tex_h;y++){
					glGenTextures(1, &tiled_textures[x][y].gl_texture);

					glBindTexture(GL_TEXTURE_2D, tiled_textures[x][y].gl_texture);

					// No mipmapping
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_max_size);
					glPixelStorei(GL_UNPACK_ALIGNMENT, 2);

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_max_size, tex_max_size, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);


					// set the cordinates of this texture in the -1 .. +1 space
					// +1 is top, so we need to flip the vertical coordinate

					tiled_textures[x][y].left=(float)(x*tex_max_size*2)/(float)(screen->w)-1;
					tiled_textures[x][y].right=(float)((x+1)*tex_max_size*2)/(float)(screen->w)-1;
					tiled_textures[x][y].top=-((float)(y*tex_max_size*2)/(float)(screen->h)-1);
					tiled_textures[x][y].bottom=-((float)((y+1)*tex_max_size*2)/(float)(screen->h)-1);
				}
			}
		}
	}
}

/**
 * Deletes the textures
 */
void delete_textures(){
	if (!tiling_in_use){
		if (pbo_able){
			glDeleteTextures(2, &pbo_textures[0]);
		}
		else if (gl_texture != 0){
			glDeleteTextures(1, &gl_texture);
		}
	}
	else{
		for (size_t x=0;x<tiled_textures.size();x++){
			for (size_t y=0;y<tiled_textures[x].size();y++){
				glDeleteTextures(1, &tiled_textures[x][y].gl_texture);
			}
			tiled_textures[x].resize(0);
		}
		tiled_textures.resize(0);
	}
}


/**
 * Resizes the current buffers to new dimensions
 */
unsigned short *dr_textur_reset()
{
	if(!pbo_able){
		SDL_FreeSurface(texture);
		texture = SDL_CreateRGBSurface(SDL_SWSURFACE, tex_w, tex_h, COLOUR_DEPTH, RMASK, GMASK, BMASK,  AMASK);
		pixels=texture->pixels;
	}
	else{

		if (pixels)
			pbo_unmap();

		glDeleteBuffersARB(2, &pbo_ids[0]);
		glGenBuffersARB(2,&pbo_ids[0]);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pbo_ids[0]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,tex_w*tex_h*2,0,GL_DYNAMIC_DRAW_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pbo_ids[1]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,tex_w*tex_h*2,0,GL_DYNAMIC_DRAW_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	}

	opengl_error();

	return (unsigned short*)pixels;
}

/**
 * Re-inits our surfaces and textures.
 * @param textur The new allocated buffer address will be set to that direction.
 * @param w Desired screen width
 * @param h Desired screen height
 */
int dr_textur_resize(unsigned short** const textur, int w, int const h)
{
	int flags = screen->flags;

	display_set_actual_width( w );
	// some cards need those alignments
	// especially 64bit want a border of 8bytes
	w = (w + 15) & 0x7FF0;
	if(  w<=0  ) {
		w = 16;
	}

	if(  w!=screen->w  ||  h!=screen->h  ) {

		width = w;
		height = h;

		screen = SDL_SetVideoMode(w, h, COLOUR_DEPTH, flags);

		delete_textures();
		update_tex_dims();
		dr_textur_reset();

		printf("textur_resize()::screen=%p\n", screen);
		if (screen && texture) {
			DBG_MESSAGE("dr_textur_resize(SDL)", "SDL realized screen size width=%d, height=%d (requested w=%d, h=%d)", screen->w, screen->h, w, h);
		}
		else {
			if (dbg) {
				dbg->warning("dr_textur_resize(SDL)", "screen or texture is NULL. Good luck!");
			}
		}
		fflush(NULL);

		glEnable(GL_TEXTURE_2D);

		create_gl_texture();
		if (pbo_able)
			pbo_map();
	}
	*textur = (unsigned short*)pixels;

	return tex_w;
}


/**
 * Allocates and creates the initial buffers (RGBSurface/PBOs and OpenGL texture)
 * @return Pointer to the newly allocated buffer
 */
unsigned short *dr_textur_init()
{
	// discard last error
	glGetError();

	create_gl_texture();

	if(!pbo_able){
		texture = SDL_CreateRGBSurface(SDL_SWSURFACE, tex_w, tex_h, COLOUR_DEPTH, RMASK, GMASK, BMASK,  AMASK);
		pixels=texture->pixels;
	}
	else{
		glGenBuffersARB(2,&pbo_ids[0]);

		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pbo_ids[0]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,tex_w*tex_h*2,0,GL_DYNAMIC_DRAW_ARB);

		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pbo_ids[1]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,tex_w*tex_h*2,0,GL_DYNAMIC_DRAW_ARB);

		//clear the binding
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

		pbo_map();
	}

	opengl_error();

	return (unsigned short*)pixels;
}


/**
 * Transform a 24 bit RGB color into 16-bit R5G6B5.
 * @return converted color value
 */
unsigned int get_system_color(unsigned int r, unsigned int g, unsigned int b)
{
	return ((Uint8)r>>3)<<11 | ((Uint8)g>>2)<<5 | (Uint8)b>>3;
}

/**
 * Unlocks the PBO.
 */
void dr_prepare_flush()
{
	if (pbo_able && !pixels)
		pbo_map();
	return;
}

/**
 * Clears screen and queues a new render.
 */
void dr_flush()
{
	if (pbo_able)
		pbo_unmap();
	else
		display_flush_buffer();

	glViewport(0, 0, screen->w, screen->h);
	glClear(GL_COLOR_BUFFER_BIT);

	if (!tiling_in_use){

		if (pbo_able)
			glBindTexture(GL_TEXTURE_2D, pbo_textures[pbo_index]);
		else
			glBindTexture(GL_TEXTURE_2D, gl_texture);

		glColor3f(1.0f, 1.0f, 1.0f);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, 1.0f);
		glTexCoord2f(x_max_coord, 0.0f);
		glVertex2f(1.0f, 1.0f);
		glTexCoord2f(x_max_coord, y_max_coord);
		glVertex2f(1.0f, -1.0f);
		glTexCoord2f(0.0f, y_max_coord);
		glVertex2f(-1.0f, -1.0f);
		glEnd();
	}
	else{
		glColor3f(1.0f, 1.0f, 1.0f);
		for (int x=0;x<n_tex_w;x++){
			for (int y=0;y<n_tex_h;y++){

				glBindTexture(GL_TEXTURE_2D, tiled_textures[x][y].gl_texture);

				const float left=tiled_textures[x][y].left;
				const float right=tiled_textures[x][y].right;
				const float top=tiled_textures[x][y].top;
				const float bottom=tiled_textures[x][y].bottom;

				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f);
				glVertex2f(left, top);
				glTexCoord2f(1.0f, 0.0f);
				glVertex2f(right, top);
				glTexCoord2f(1.0f, 1.0f);
				glVertex2f(right, bottom);
				glTexCoord2f(0.0f, 1.0f);
				glVertex2f(left, bottom);
				glEnd();
			}
		}
	}
	SDL_GL_SwapBuffers();
}

/**
 * Copies from the buffer to the texture the desired zone. Not called in PBO-mode.
 */
void dr_textur(int xp, int yp, int w, int h)
{
	// make sure the given rectangle is completely on screen
	if (xp + w > screen->w) {
		w = screen->w - xp;
	}
	if (yp + h > screen->h) {
		h = screen->h - yp;
	}
#ifdef DEBUG
	// make sure both are positive numbers
	if(  w*h>0  )
#endif
	{
		if (!tiling_in_use){
			glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_w);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, xp);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, yp);
			glTexSubImage2D(GL_TEXTURE_2D, 0, xp, yp, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels );
		}
		else{

			// divide this into smaller dr_textur, on tex_max_size x tex_max_size chunks

			int x_tile_start=xp/tex_max_size;
			int x_tile_end=(xp+w-1)/tex_max_size;

			int y_tile_start=yp/tex_max_size;
			int y_tile_end=(yp+h-1)/tex_max_size;


			if ((x_tile_start==x_tile_end) && (y_tile_start == y_tile_end)){
				// base case, just one texture involved, we just update it

				glBindTexture(GL_TEXTURE_2D, tiled_textures[x_tile_start][y_tile_start].gl_texture);

				glPixelStorei(GL_UNPACK_ROW_LENGTH, tex_w);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
				glPixelStorei(GL_UNPACK_SKIP_PIXELS, xp);
				glPixelStorei(GL_UNPACK_SKIP_ROWS, yp);

				glTexSubImage2D(GL_TEXTURE_2D, 0, xp%tex_max_size, yp%tex_max_size, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);

			}
			else{

				//int n_x_tiles = x_tile_end - x_tile_start;
				int n_y_tiles = y_tile_end - y_tile_start;

				if (n_y_tiles==0){
					// horizontal strip

					const int first_tile_w = tex_max_size - xp%tex_max_size;
					const int last_tile_w = (xp+w)%tex_max_size;

					dr_textur(xp,yp,first_tile_w,h);

					for (int i=(x_tile_start+1);i<(x_tile_end);i++){
						dr_textur(i*tex_max_size,yp,tex_max_size,h);
					}

					dr_textur(x_tile_end*tex_max_size,yp,last_tile_w,h);

				}
				else{
					// spans y, split it in horizontal strips

					const int first_tile_h = tex_max_size - yp%tex_max_size;
					const int last_tile_h = (yp+h)%tex_max_size;

					dr_textur(xp,yp,w,first_tile_h);

					for (int i=(y_tile_start+1);i<(y_tile_end);i++){
						dr_textur(xp,i*tex_max_size,w,tex_max_size);
					}

					dr_textur(xp,y_tile_end*tex_max_size,w,last_tile_h);
				}
			}
		}
	}
}


// move cursor to the specified location
void move_pointer(int x, int y)
{
	SDL_WarpMouse((Uint16)x, (Uint16)y);
}


// set the mouse cursor (pointer/load)
void set_pointer(int loading)
{
	SDL_SetCursor(loading ? hourglass : arrow);
}


/**
 * Some wrappers can save screenshots.
 * @return 1 on success, 0 if not implemented for a particular wrapper and -1
 *         in case of error.
 * @author Hj. Malthaner
 */
int dr_screenshot(const char *filename, int x, int y, int w, int h)
{
#ifdef WIN32
	if(  dr_screenshot_png(filename, w, h, width, ((unsigned short *)(screen->pixels))+x+y*width, screen->format->BitsPerPixel )  ) {
		return 1;
	}
#endif
	return SDL_SaveBMP(SDL_GetVideoSurface(), filename) == 0 ? 1 : -1;
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */


static inline unsigned int ModifierKeys()
{
	SDLMod mod = SDL_GetModState();

	return
		(mod & KMOD_SHIFT ? 1 : 0) |
		(mod & KMOD_CTRL  ? 2 : 0);
}


static int conv_mouse_buttons(Uint8 const state)
{
	return
		(state & SDL_BUTTON_LMASK ? MOUSE_LEFTBUTTON  : 0) |
		(state & SDL_BUTTON_MMASK ? MOUSE_MIDBUTTON   : 0) |
		(state & SDL_BUTTON_RMASK ? MOUSE_RIGHTBUTTON : 0);
}


static void internal_GetEvents(bool const wait)
{
	SDL_Event event;
	event.type = 1;

	if (wait) {
		int n;

		do {
			SDL_WaitEvent(&event);
			n = SDL_PollEvent(NULL);
		} while (n != 0 && event.type == SDL_MOUSEMOTION);
	}
	else {
		int n;
		bool got_one = false;

		do {
			n = SDL_PollEvent(&event);

			if (n != 0) {
				got_one = true;

				if (event.type == SDL_MOUSEMOTION) {
					sys_event.type = SIM_MOUSE_MOVE;
					sys_event.code = SIM_MOUSE_MOVED;
					sys_event.mx   = event.motion.x;
					sys_event.my   = event.motion.y;
					sys_event.mb   = conv_mouse_buttons(event.motion.state);
				}
			}
		} while (n != 0 && event.type == SDL_MOUSEMOTION);

		if (!got_one) return;
	}

	switch (event.type) {
		case SDL_VIDEORESIZE:
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SYSTEM_RESIZE;
			sys_event.mx   = event.resize.w;
			sys_event.my   = event.resize.h;
			printf("expose: x=%i, y=%i\n", sys_event.mx, sys_event.my);
			break;

		case SDL_MOUSEBUTTONDOWN:
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.key_mod = ModifierKeys();
			sys_event.mx      = event.button.x;
			sys_event.my      = event.button.y;
			sys_event.mb      = conv_mouse_buttons(SDL_GetMouseState(0, 0));
			switch (event.button.button) {
				case 1: sys_event.code = SIM_MOUSE_LEFTBUTTON;  break;
				case 2: sys_event.code = SIM_MOUSE_MIDBUTTON;   break;
				case 3: sys_event.code = SIM_MOUSE_RIGHTBUTTON; break;
				case 4: sys_event.code = SIM_MOUSE_WHEELUP;     break;
				case 5: sys_event.code = SIM_MOUSE_WHEELDOWN;   break;
			}
			break;

		case SDL_MOUSEBUTTONUP:
			sys_event.type    = SIM_MOUSE_BUTTONS;
			sys_event.key_mod = ModifierKeys();
			sys_event.mx      = event.button.x;
			sys_event.my      = event.button.y;
			sys_event.mb      = conv_mouse_buttons(SDL_GetMouseState(0, 0));
			switch (event.button.button) {
				case 1: sys_event.code = SIM_MOUSE_LEFTUP;  break;
				case 2: sys_event.code = SIM_MOUSE_MIDUP;   break;
				case 3: sys_event.code = SIM_MOUSE_RIGHTUP; break;
			}
			break;

		case SDL_KEYDOWN:
		{
			unsigned long code;
			bool   const  numlock = SDL_GetModState() & KMOD_NUM;
			SDLKey const  sym     = event.key.keysym.sym;
			switch (sym) {
				case SDLK_DELETE:   code = 127;                           break;
				case SDLK_DOWN:     code = SIM_KEY_DOWN;                  break;
				case SDLK_END:      code = SIM_KEY_END;                   break;
				case SDLK_HOME:     code = SIM_KEY_HOME;                  break;
				case SDLK_F1:       code = SIM_KEY_F1;                    break;
				case SDLK_F2:       code = SIM_KEY_F2;                    break;
				case SDLK_F3:       code = SIM_KEY_F3;                    break;
				case SDLK_F4:       code = SIM_KEY_F4;                    break;
				case SDLK_F5:       code = SIM_KEY_F5;                    break;
				case SDLK_F6:       code = SIM_KEY_F6;                    break;
				case SDLK_F7:       code = SIM_KEY_F7;                    break;
				case SDLK_F8:       code = SIM_KEY_F8;                    break;
				case SDLK_F9:       code = SIM_KEY_F9;                    break;
				case SDLK_F10:      code = SIM_KEY_F10;                   break;
				case SDLK_F11:      code = SIM_KEY_F11;                   break;
				case SDLK_F12:      code = SIM_KEY_F12;                   break;
				case SDLK_F13:      code = SIM_KEY_F13;                   break;
				case SDLK_F14:      code = SIM_KEY_F14;                   break;
				case SDLK_F15:      code = SIM_KEY_F15;                   break;
				case SDLK_KP0:      code = numlock ? '0' : 0;             break;
				case SDLK_KP1:      code = numlock ? '1' : SIM_KEY_END;   break;
				case SDLK_KP2:      code = numlock ? '2' : SIM_KEY_DOWN;  break;
				case SDLK_KP3:      code = numlock ? '3' : '<';           break;
				case SDLK_KP4:      code = numlock ? '4' : SIM_KEY_LEFT;  break;
				case SDLK_KP5:      code = numlock ? '5' : 0;             break;
				case SDLK_KP6:      code = numlock ? '6' : SIM_KEY_RIGHT; break;
				case SDLK_KP7:      code = numlock ? '7' : SIM_KEY_HOME;  break;
				case SDLK_KP8:      code = numlock ? '8' : SIM_KEY_UP;    break;
				case SDLK_KP9:      code = numlock ? '9' : '>';           break;
				case SDLK_LEFT:     code = SIM_KEY_LEFT;                  break;
				case SDLK_PAGEDOWN: code = '<';                           break;
				case SDLK_PAGEUP:   code = '>';                           break;
				case SDLK_RIGHT:    code = SIM_KEY_RIGHT;                 break;
				case SDLK_UP:       code = SIM_KEY_UP;                    break;
				case SDLK_PAUSE:    code = 16;                            break;

				default:
					if (event.key.keysym.unicode != 0) {
						code = event.key.keysym.unicode;
						if (event.key.keysym.unicode == 22 /* ^V */) {
#if 0	// disabled as internal buffer is used; code is retained for possible future implementation of dr_paste()
							// X11 magic ... not tested yet!
							SDL_SysWMinfo si;
							if (SDL_GetWMInfo(&si) && si.subsystem == SDL_SYSWM_X11) {
								// clipboard under X11
								XEvent         evt;
								Atom           sseln   = XA_CLIPBOARD(si.x11.display);
								Atom           target  = XA_STRING;
								unsigned char* sel_buf = 0;
								unsigned long  sel_len = 0;	/* length of sel_buf */
								unsigned int   context = XCLIB_XCOUT_NONE;
								xcout(si.x11.display, si.x11.window, evt, sseln, target, &sel_buf, &sel_len, &context);
								/* fallback is needed. set XA_STRING to target and restart the loop. */
								if (context != XCLIB_XCOUT_FALLBACK) {
									// something in clipboard
									SDL_Event new_event;
									new_event.type           = SDL_KEYDOWN;
									new_event.key.keysym.sym = SDLK_UNKNOWN;
									for (unsigned char const* chr = sel_buf; sel_len-- >= 0; ++chr) {
										new_event.key.keysym.sym = *chr == '\n' ? '\r' : *chr;
										SDL_PushEvent(&new_event);
									}
									free(sel_buf);
								}
							}
#endif
						}
					} else if (0 < sym && sym < 127) {
						code = event.key.keysym.sym; // try with the ASCII code
					} else {
						code = 0;
					}
			}
			sys_event.type    = SIM_KEYBOARD;
			sys_event.code    = code;
			sys_event.key_mod = ModifierKeys();
			break;
		}

		case SDL_MOUSEMOTION:
			sys_event.type = SIM_MOUSE_MOVE;
			sys_event.code = SIM_MOUSE_MOVED;
			sys_event.mx   = event.motion.x;
			sys_event.my   = event.motion.y;
			sys_event.mb   = conv_mouse_buttons(event.motion.state);
			sys_event.key_mod = ModifierKeys();
			break;

		case SDL_ACTIVEEVENT:
		case SDL_KEYUP:
			sys_event.type = SIM_KEYBOARD;
			sys_event.code = 0;
			break;

		case SDL_QUIT:
			sys_event.type = SIM_SYSTEM;
			sys_event.code = SYSTEM_QUIT;
			break;

		default:
			sys_event.type = SIM_IGNORE_EVENT;
			sys_event.code = 0;
			break;
	}
}


void GetEvents()
{
	internal_GetEvents(true);
}


void GetEventsNoWait()
{
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	internal_GetEvents(false);
}


void show_pointer(int yesno)
{
	SDL_ShowCursor(yesno != 0);
}


void ex_ord_update_mx_my()
{
	SDL_PumpEvents();
}


uint32 dr_time()
{
	return SDL_GetTicks();
}


void dr_sleep(uint32 usec)
{
	SDL_Delay(usec);
}


#ifdef _WIN32
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(int argc, char **argv)
#endif
{
#ifdef _WIN32
	int    const argc = __argc;
	char** const argv = __argv;
#endif
	return sysmain(argc, argv);
}
