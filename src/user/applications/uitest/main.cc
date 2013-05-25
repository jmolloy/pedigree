/*
 * Copyright (c) 2011 Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <types.h>
#include <graphics/Graphics.h>
#include <Widget.h>

#include <iostream>

using namespace std;
using namespace PedigreeGraphics;

class TestWidget : public Widget
{
    public:
        TestWidget() : Widget()
        {};
        virtual ~TestWidget()
        {}

        virtual bool render(Rect &rt, Rect &dirty)
        {
            cout << "uitest: rendering widget." << endl;

            Framebuffer *pFramebuffer = getFramebuffer();
            pFramebuffer->rect(rt.getX(), rt.getY(), rt.getW(), rt.getH(), createRgb(0xFF, 0, 0), Bits24_Rgb);

            return true;
        }
};

volatile bool bRun = true;

bool callback(WidgetMessages message, size_t msgSize, void *msgData)
{
    cout << "uitest: callback for '" << static_cast<int>(message) << "'." << endl;

    return true;
}

int main(int argc, char *argv[])
{
    cout << "uitest: starting up" << endl;

    Rect rt(20, 20, 100, 100);

    Widget *pWidget = new TestWidget();
    if(!pWidget->construct("uitest", "UI Test", callback, rt))
    {
        cerr << "uitest: widget construction failed" << endl;
        delete pWidget;
        return 1;
    }

    cout << "uitest: widget created (handle is " << pWidget->getHandle() << ")." << endl;

    pWidget->visibility(true);

    cout << "Widget visible!" << endl;

    Rect dirty;
    pWidget->render(rt, dirty);
    pWidget->redraw(dirty);

    // Main loop
    while(bRun)
    {
        Widget::checkForEvents();
    }

    return 0;
}
