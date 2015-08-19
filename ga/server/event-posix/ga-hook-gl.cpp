/*
 * Copyright (c) 2013 Chun-Ying Huang
 *
 * This file is part of GamingAnywhere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#ifndef WIN32
#include <dlfcn.h>
#endif

#ifdef __linux__
#include <X11/extensions/XTest.h>
#endif

#include "ga-common.h"
#include "ga-conf.h"
#include "vsource.h"
#include "pipeline.h"
#include "controller.h"
#include "ga-hook-common.h"
#include "ga-hook-gl.h"
#include "ga-hook-lib.h"
#include "ctrl-sdl.h"

#include "sdl12-event.h"
#include "sdl12-video.h"
#include "sdl12-mouse.h"

#include <map>
using namespace std;

#ifndef WIN32
#ifdef __cplusplus
extern "C" {
#endif
void glFlush();
#ifdef __linux__
void glXSwapBuffers( Display *dpy, GLXDrawable drawable );
#endif
#ifdef __cplusplus
}
#endif
#endif

// Configuration
const unsigned MINIMUM_FRAME_RATE = 30; // fps

// For duplicate frame generation (since OpenGL does not deliver updates if nothing changes)
static struct timeval previous_frame_tv = { 0 };
static vsource_frame_t previous_frame = { 0 };
pthread_cond_t new_frame_captured_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t new_frame_captured_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pipe_access_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct timeval initialTv = { 0 };
static int frame_interval;

// for hooking
t_glFlush		old_glFlush = NULL;
#ifdef __linux__
t_glXSwapBuffers	old_glXSwapBuffers = NULL;

Display *display = NULL;
Window window = 0;
#endif

// Thanks to https://gist.github.com/BinaryPrison/1112092 for this code.
static inline uint32_t __iter_div_u64_rem(uint64_t dividend, uint32_t divisor, uint64_t *remainder)
{
  uint32_t ret = 0;

  while (dividend >= divisor) {
    /* The following asm() prevents the compiler from
       optimising this loop into a modulo operation.  */
    asm("" : "+rm"(dividend));

    dividend -= divisor;
    ret++;
  }

  *remainder = dividend;

  return ret;
}

#define NSEC_PER_SEC  1000000000L
static inline void timespec_add_ns(struct timespec *a, uint64_t ns)
{
  a->tv_sec += __iter_div_u64_rem(a->tv_nsec + ns, NSEC_PER_SEC, &ns);
  a->tv_nsec = ns;
}

static void *
frame_injection_threadproc(void *arg)
{
	ga_error("Frame injection thread entered.\n");

	while (1)
	{
		// Wait until a frame is captured
		struct timeval tv;
		struct timespec to;
		gettimeofday(&tv, NULL);
		to.tv_sec = tv.tv_sec;
		to.tv_nsec = tv.tv_usec * 1000;
		timespec_add_ns(&to, NSEC_PER_SEC / MINIMUM_FRAME_RATE);

		int wait_result = pthread_cond_timedwait(&new_frame_captured_cond, &new_frame_captured_mutex, &to);

		struct timeval now;
		gettimeofday(&now, NULL);

		if (wait_result == ETIMEDOUT &&
			previous_frame_tv.tv_sec > 0 &&
			tvdiff_us(&now, &previous_frame_tv) > 1000000 / MINIMUM_FRAME_RATE &&
			previous_frame.realsize > 0)
		{
			pthread_mutex_lock(&pipe_access_mutex);
			// Inject previous frame again
			pooldata_t *data = g_pipe[0]->allocate_data();
			vsource_frame_t *frame = (vsource_frame_t *) data->ptr;
			vsource_dup_frame(&previous_frame, frame);

			// Generate presentation time stamp
			struct timeval repeat_tv;
			gettimeofday(&repeat_tv, NULL);
			frame->imgpts = tvdiff_us(&repeat_tv, &initialTv)/frame_interval;

			// duplicate from channel 0 to other channels
			ga_hook_capture_dupframe(frame);
			g_pipe[0]->store_data(data);
			g_pipe[0]->notify_all();
			pthread_mutex_unlock(&pipe_access_mutex);
			// ga_error("Frame injected.\n");
		}
	}
}

void
x11_replay_callback(void *msg, int msglen) {
	sdlmsg_t *smsg = (sdlmsg_t*) msg;
#ifdef __linux__
	if (display == NULL || window == 0) {
		ga_error("Unable to replay event due to missing display or window.\n");
		return;
	}

   	/*XEvent event;
	memset(&event, 0x00, sizeof(event));
	bool success = XQueryPointer(
		display,
		window,
		&event.xbutton.root,
		&event.xbutton.window,
		&event.xbutton.x_root,
		&event.xbutton.y_root,
		&event.xbutton.x,
		&event.xbutton.y,
		&event.xbutton.state);
		
	//ga_error("Mouse readings x_root=%u, y_root=%u, x=%u, y=%u, state=%u\n", event.xbutton.x_root, event.xbutton.y_root, event.xbutton.x, event.xbutton.y, event.xbutton.state);
		
	// ga_error((success) ? "YES" : "NO");
	
	if(!success) {
		ga_error("Cannot query pointer attributes which are required for the relaying of events.\n");
		return;
	}
	*/

	sdlmsg_mouse_t *msgm = (sdlmsg_mouse_t*) msg;
	XTestGrabControl(display, True);
	switch(smsg->msgtype) {
		case SDL_EVENT_MSGTYPE_MOUSEMOTION:
			ga_error("Mouse movement, x: %u y: %u\n", ntohs(msgm->mousex), ntohs(msgm->mousey));
			{
				int new_root_x, new_root_y;

				Window child;
				/*bool success = */XTranslateCoordinates(display, window, DefaultRootWindow(display), ntohs(msgm->mousex), ntohs(msgm->mousey), &new_root_x, &new_root_y, &child);
				//ga_error("new_x=%d, new_y=%d\n", new_root_x, new_root_y);
				XTestFakeMotionEvent (display, 0, new_root_x, new_root_y, CurrentTime);

/*				Display* xdisplay1 = XOpenDisplay(NULL);
				Window root = DefaultRootWindow(display);
			XWarpPointer(xdisplay1, None, root, 0, 0, 0, 0, msgm->mousex), ntohs(msgm->mousey))*/
			}
			break;

		case SDL_EVENT_MSGTYPE_MOUSEKEY:
			ga_error("Mouse button event btn=%u pressed=%d\n", msgm->mousebutton, msgm->is_pressed);
			/*event.type = (msgm->is_pressed == 1) ? ButtonPress : ButtonRelease;
			if (event.type == ButtonRelease)
				event.xbutton.state = 0x100;
			event.xbutton.button = msgm->mousebutton;
			event.xbutton.same_screen = True;
			if(XSendEvent(display, window, True, 0xfff, &event) == 0) ga_error("Error\n");*/
			XTestFakeButtonEvent (display, 1, (msgm->is_pressed == 1), CurrentTime);
//			XFlush(display);

			break;
	}
	XSync(display, True);
	XTestGrabControl(display, False);

	/*if(smsg->msgtype == SDL_EVENT_MSGTYPE_MOUSEMOTION) {
		sdlmsg_mouse_t *msgm = (sdlmsg_mouse_t*) msg;
		ga_error("Mouse movement, x: %u y: %u, %s\n", ntohs(msgm->mousex), ntohs(msgm->mousey), (msgm->is_pressed != 0) ? "pressed" : "");
		
	}*/
	// sdlmsg_ntoh(smsg);
	// sdl12_hook_replay(smsg);
	//	ga_error("Hello World");
    #endif
	return;
}

static void
gl_global_init() {
	static int initialized = 0;
	pthread_t ga_server_thread;
	pthread_t frame_injection_thread;
	if(initialized != 0)
		return;
	//

	// override controller
	// sdl12_mapinit();
	// sdlmsg_replay_init(NULL);
	ctrl_server_setreplay(x11_replay_callback);
	no_default_controller = 1;

	if(pthread_create(&ga_server_thread, NULL, ga_server, NULL) != 0) {
		ga_error("ga_hook: create thread failed.\n");
		exit(-1);
	}

	if(pthread_create(&frame_injection_thread, NULL, frame_injection_threadproc, NULL) != 0) {
		ga_error("ga_hook: create thread for frame injection failed.\n");
	}

	initialized = 1;

	return;
}

static void
gl_hook_symbols() {
#ifndef WIN32
	void *handle = NULL;
	char *ptr, soname[2048];
	if((ptr = getenv("LIBVIDEO")) == NULL) {
		strncpy(soname, "libGL.so.1", sizeof(soname));
	} else {
		strncpy(soname, ptr, sizeof(soname));
	}
	if((handle = dlopen(soname, RTLD_LAZY)) == NULL) {
		ga_error("dlopen '%s' failed: %s\n", soname, strerror(errno));
		exit(-1);
	}
	// for hooking
	old_glFlush = (t_glFlush)
				ga_hook_lookup_or_quit(handle, "glFlush");
	ga_error("hook-gl: hooked glFlush.\n");

#ifdef __linux__
	old_glXSwapBuffers = (t_glXSwapBuffers)ga_hook_lookup_or_quit(handle, "glXSwapBuffers");
	ga_error("hook_gl: hooked glXSwapBuffers\n");
#endif

	// indirect hook
	if((ptr = getenv("HOOKVIDEO")) == NULL)
		goto quit;
	strncpy(soname, ptr, sizeof(soname));
	if((handle = dlopen(soname, RTLD_LAZY)) != NULL) {
		hook_lib_generic(soname, handle, "glFlush", (void*) hook_glFlush);

#ifdef __linux__
		hook_lib_generic(soname, handle, "glXSwapBuffers", (void*) hook_glXSwapBuffers);
#endif
	}
	ga_error("hook-gl: hooked into %s.\n", soname);

quit:
#endif
	return;
}

void
copyFrame() {
#ifdef __linux__
	static struct timeval captureTv;
	static int frameLinesize;
	static unsigned char *frameBuf;
	static int sb_initialized = 0;
	static int global_initialized = 0;
	//
	GLint vp[4];
	int vp_x, vp_y, vp_width, vp_height;
	int i;
	//
	pooldata_t *data;
	vsource_frame_t *frame;
	//
	if(global_initialized == 0) {
		gl_global_init();
		global_initialized = 1;
	}

		// capture the screen
	glGetIntegerv(GL_VIEWPORT, vp);
	vp_x = vp[0];
	vp_y = vp[1];
	vp_width = vp[2];
	vp_height = vp[3];
	//
	if(ga_hook_capture_prepared(vp_width, vp_height, 1) < 0)
		return;
	//
	if(sb_initialized == 0) {
		frame_interval = 1000000/video_fps; // in the unif of us
		frame_interval++;
		gettimeofday(&initialTv, NULL);
		frameBuf = (unsigned char*) malloc(encoder_width * encoder_height * 4);
		if(frameBuf == NULL) {
			ga_error("allocate frame failed.\n");
			return;
		}
		frameLinesize = game_width * 4;
		sb_initialized = 1;
	} else {
		gettimeofday(&captureTv, NULL);
	}
	//
	if (enable_server_rate_control && ga_hook_video_rate_control() < 0) {
		return;
	}
	//
	pthread_mutex_lock(&pipe_access_mutex);

	do {
		unsigned char *src, *dst;
		//
		frameLinesize = vp_width<<2;
		//
		data = g_pipe[0]->allocate_data();
		frame = (vsource_frame_t*) data->ptr;
		frame->pixelformat = PIX_FMT_RGBA;
		frame->realwidth = vp_width;
		frame->realheight = vp_height;
		frame->realstride = frameLinesize;
		frame->realsize = vp_height/*vp_width*/ * frameLinesize;
		frame->linesize[0] = frameLinesize;/*frame->stride*/;
		// read a block of pixels from the framebuffer (backbuffer)
		glReadBuffer(GL_BACK);
		glReadPixels(vp_x, vp_y, vp_width, vp_height, GL_RGBA, GL_UNSIGNED_BYTE, frameBuf);
		// image is upside down!
		src = frameBuf + frameLinesize * (vp_height - 1);
		dst = frame->imgbuf;
		for(i = 0; i < frame->realheight; i++) {
			bcopy(src, dst, frameLinesize);
			dst += frameLinesize/*frame->stride*/;
			src -= frameLinesize;
		}
		frame->imgpts = tvdiff_us(&captureTv, &initialTv)/frame_interval;
	} while(0);

	// duplicate from channel 0 to other channels
	ga_hook_capture_dupframe(frame);
	g_pipe[0]->store_data(data);
	g_pipe[0]->notify_all();

	previous_frame_tv = captureTv;
	if (frame->imgbufsize > previous_frame.imgbufsize) {
		free(previous_frame.imgbuf);
		previous_frame.imgbuf = (unsigned char*)malloc(frame->imgbufsize);
		previous_frame.imgbufsize = frame->imgbufsize;
	}
	vsource_dup_frame(frame, &previous_frame);

	pthread_mutex_unlock(&pipe_access_mutex);
	pthread_cond_broadcast(&new_frame_captured_cond);
#endif
}

void
hook_glFlush() {
	//
	if(old_glFlush == NULL) {
		gl_hook_symbols();
	}
	old_glFlush();
	//

	copyFrame();
	return;
}

#ifdef __linux__
void
hook_glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
	display = dpy;
	window = drawable;

	//
	if(old_glXSwapBuffers == NULL) {
		gl_hook_symbols();
	}
	old_glXSwapBuffers(dpy, drawable);
	//printf("JACKPOT");fflush(stdout);
	//

	copyFrame();
	return;
}
#endif

#ifndef WIN32	/* POSIX interfaces */
void
glFlush() {
	hook_glFlush();
}

#ifdef __linux__
void
glXSwapBuffers( Display *dpy, GLXDrawable drawable ){
	hook_glXSwapBuffers(dpy, drawable);
}
#endif

__attribute__((constructor))
static void
gl_hook_loaded(void) {
	ga_error("ga-hook-gl loaded!\n");
	if(ga_hook_init() < 0) {
		ga_error("ga_hook: init failed.\n");
		exit(-1);
	}
	return;
}
#endif /* ! WIN32 */


