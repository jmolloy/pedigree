/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* $XFree86: xc/programs/glxgears/glxgears.c,v 1.4 2003/10/24 20:38:11 tsi Exp $ */

/*
 * This is a port of the infamous "gears" demo to straight GLX (i.e. no GLUT)
 * Port by Brian Paul  23 March 2001
 *
 * Command line options:
 *    -info      print GL implementation information
 *
 */

// Simplified, courtesy of Kevin Lange.

#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include <GL/gl.h>
#include <GL/osmesa.h>

#include <graphics/Graphics.h>
#include <Widget.h>

class Gears;

Gears *g_pGears = NULL;

static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;

static unsigned int frames = 0;
static unsigned int start_time = 0;

void fps()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    frames++;
    if (!start_time)
    {
        start_time = now.tv_sec;
    }
    else if ((now.tv_sec - start_time) >= 5)
    {
        GLfloat seconds = now.tv_sec - start_time;
        GLfloat fps = frames / seconds;
        syslog(LOG_INFO, "%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
        start_time = now.tv_sec;
        frames = 0;
    }
}

static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
{
    GLint i;
    GLfloat r0, r1, r2;
    GLfloat angle, da;
    GLfloat u, v, len;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0;
    r2 = outer_radius + tooth_depth / 2.0;

    da = 2.0 * M_PI / teeth / 4.0;

    glShadeModel(GL_FLAT);

    glNormal3f(0.0, 0.0, 1.0);

    /* draw front face */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;
        glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        if (i < teeth) {
            glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
            glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
            width * 0.5);
        }
    }
    glEnd();

    /* draw front sides of teeth */
    glBegin(GL_QUADS);
    da = 2.0 * M_PI / teeth / 4.0;
    for (i = 0; i < teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;

        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
            width * 0.5);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
            width * 0.5);
    }
    glEnd();

    glNormal3f(0.0, 0.0, -1.0);

    /* draw back face */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        if (i < teeth) {
            glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
                -width * 0.5);
            glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        }
    }
    glEnd();

    /* draw back sides of teeth */
    glBegin(GL_QUADS);
    da = 2.0 * M_PI / teeth / 4.0;
    for (i = 0; i < teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;

        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
        -width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
        -width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
    }
    glEnd();

    /* draw outward faces of teeth */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i < teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;

        glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
        glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
        u = r2 * cos(angle + da) - r1 * cos(angle);
        v = r2 * sin(angle + da) - r1 * sin(angle);
        len = sqrt(u * u + v * v);
        u /= len;
        v /= len;
        glNormal3f(v, -u, 0.0);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
        glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
        glNormal3f(cos(angle), sin(angle), 0.0);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
            width * 0.5);
        glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
            -width * 0.5);
        u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
        v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
        glNormal3f(v, -u, 0.0);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
            width * 0.5);
        glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
            -width * 0.5);
        glNormal3f(cos(angle), sin(angle), 0.0);
    }

    glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
    glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

    glEnd();

    glShadeModel(GL_SMOOTH);

    /* draw inside radius cylinder */
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / teeth;
        glNormal3f(-cos(angle), -sin(angle), 0.0);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
        glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
    }
    glEnd();
}


static void
draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotatef(view_rotx, 1.0, 0.0, 0.0);
    glRotatef(view_roty, 0.0, 1.0, 0.0);
    glRotatef(view_rotz, 0.0, 0.0, 1.0);

    glPushMatrix();
    glTranslatef(-3.0, -2.0, 0.0);
    glRotatef(angle, 0.0, 0.0, 1.0);
    glCallList(gear1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(3.1, -2.0, 0.0);
    glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
    glCallList(gear2);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-3.1, 4.2, 0.0);
    glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
    glCallList(gear3);
    glPopMatrix();

    glPopMatrix();
}



/* new window size or exposure */
static void
reshape(int width, int height)
{
    GLfloat h = (GLfloat) height / (GLfloat) width;

    glViewport(0, 0, (GLint) width, (GLint) height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -40.0);
}



static void
init(void)
{
    static GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };
    static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
    static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
    static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    /* make the gears */
    gear1 = glGenLists(1);
    glNewList(gear1, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
    gear(1.0, 4.0, 1.0, 20, 0.7);
    glEndList();

    gear2 = glGenLists(1);
    glNewList(gear2, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
    gear(0.5, 2.0, 2.0, 10, 0.7);
    glEndList();

    gear3 = glGenLists(1);
    glNewList(gear3, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
    gear(1.3, 2.0, 0.5, 10, 0.7);
    glEndList();

    glEnable(GL_NORMALIZE);
}


class Gears : public Widget
{
    public:
        Gears() : Widget(), fb(NULL), m_bGlInit(false), m_bContextValid(false), m_nWidth(0), m_nHeight(0)
        {};
        virtual ~Gears()
        {}

        bool initOpenGL()
        {
            gl_ctx = OSMesaCreateContext(OSMESA_BGRA, NULL);

            m_bGlInit = true;

            if(m_nWidth)
            {
                glResize(m_nWidth, m_nHeight);
            }

            return true;
        }

        void deinitOpenGL()
        {
            OSMesaDestroyContext(gl_ctx);
        }

        void reposition(PedigreeGraphics::Rect newrt)
        {
            m_nWidth = newrt.getW();
            m_nHeight = newrt.getH();
            glResize(newrt.getW(), newrt.getH());
        }

        virtual bool render(PedigreeGraphics::Rect &rt, PedigreeGraphics::Rect &dirty)
        {
            if(!(m_bGlInit && m_bContextValid && fb))
            {
                return false;
            }

            // Render frame.
            angle += 0.2;
            draw();

            dirty.update(0, 0, m_nWidth, m_nHeight);

            return true;
        }

    private:

        bool glResize(size_t w, size_t h)
        {
            if(!m_bGlInit)
            {
                return false;
            }

            fb = (uint8_t*) getRawFramebuffer();
            if(!fb)
            {
                m_bContextValid = false;
                fprintf(stderr, "Couldn't get a framebuffer to use.\n");
                return false;
            }

            if(!OSMesaMakeCurrent(gl_ctx, fb, GL_UNSIGNED_BYTE, w, h))
            {
                m_bContextValid = false;
                fprintf(stderr, "OSMesaMakeCurrent failed.\n");
                return false;
            }

            // Don't render upside down.
            OSMesaPixelStore(OSMESA_Y_UP, 0);

            reshape(w, h);

            m_bContextValid = true;

            return true;
        }

        uint8_t *fb;

        bool m_bGlInit;
        bool m_bContextValid;

        OSMesaContext gl_ctx;

        size_t m_nWidth, m_nHeight;
};

bool callback(WidgetMessages message, size_t msgSize, void *msgData)
{
    switch(message)
    {
        case RepaintNeeded:
            {
                PedigreeGraphics::Rect rt, dirty;
                if(g_pGears->render(rt, dirty))
                {
                    g_pGears->redraw(dirty);
                }
            }
            break;
        case Reposition:
            {
                PedigreeGraphics::Rect dirty;
                PedigreeGraphics::Rect *rt = reinterpret_cast<PedigreeGraphics::Rect*>(msgData);
                g_pGears->reposition(*rt);
            }
            break;
        case KeyUp:
            {
                uint64_t *key = reinterpret_cast<uint64_t*>(msgData);
                syslog(LOG_INFO, "gears: keypress %d '%c'", (uint32_t) *key, (char) (*key & 0xFF));

                // What do we have?
                /// \todo don't do this this is silly.
                *key &= 0xFF;
                if(*key == 'a')
                {
                    view_roty += 5.0;
                }
                else if(*key == 'd')
                {
                    view_roty -= 5.0;
                }
                else if(*key == 'w')
                {
                    view_rotx += 5.0;
                }
                else if(*key == 's')
                {
                    view_rotx -= 5.0;
                }
            }
            break;
        default:
            syslog(LOG_INFO, "gears: unhandled callback");
    }
    return true;
}

int main (int argc, char ** argv) {
    PedigreeGraphics::Rect rt(30, 30, 500, 500);

    char endpoint[256];
    sprintf(endpoint, "gears.%d", getpid());

    g_pGears = new Gears();
    if(!g_pGears->construct(endpoint, "OSMesa 3D Gears", callback, rt)) {
        syslog(LOG_ERR, "gears: not able to construct widget");
        delete g_pGears;
        return 1;
    }
    syslog(LOG_INFO, "gears: widget constructed");

    g_pGears->initOpenGL();

    syslog(LOG_INFO, "gears: reshaping");

    init();

    syslog(LOG_INFO, "gears: entering main loop");

    time_t t1, t2;

    while (1) {
        Widget::checkForEvents(true);

        // Cheat a bit, render every frame.
        callback(RepaintNeeded, 0, 0);

        // Display our FPS - nice way to see how vbe performs...
        fps();
    }

    g_pGears->deinitOpenGL();

    delete g_pGears;

    return 0;
}

