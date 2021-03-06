/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPipeUtils.cpp"

#include "zita-at1/png2img.cc"
#include "zita-at1/button.cc"
#include "zita-at1/guiclass.cc"
#include "zita-at1/mainwin.cc"
#include "zita-at1/rotary.cc"
#include "zita-at1/styles.cc"
#include "zita-at1/tmeter.cc"

using namespace AT1;

static Mainwin* mainwin = nullptr;

// --------------------------------------------------------------------------------------------

class ZitaPipeClient : public CarlaPipeClient,
                       public Mainwin::ValueChangedCallback
{
public:
    ZitaPipeClient() noexcept
        : CarlaPipeClient(),
          fQuitReceived(false) {}

    ~ZitaPipeClient() noexcept override
    {
        if (fQuitReceived || ! isPipeRunning())
            return;

        const CarlaMutexLocker cml(getPipeLock());

        writeMessage("exiting\n");
        flushMessages();
    }

    bool quitRequested() const noexcept
    {
        return fQuitReceived;
    }

protected:
    bool msgReceived(const char* const msg) noexcept override
    {
        if (std::strcmp(msg, "zita-data") == 0)
        {
            float error;
            int noteset, midiset;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(error), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(noteset), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsInt(midiset), true);

            mainwin->setdata_ui(error, noteset, midiset);
            return true;
        }

        if (std::strcmp(msg, "zita-mask") == 0)
        {
            uint mask;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(mask), true);

            mainwin->setmask_ui(mask);
            return true;
        }

        if (std::strcmp(msg, "control") == 0)
        {
            uint index;
            float value;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(index), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            if (index < Mainwin::NROTARY)
                mainwin->_rotary[index]->set_value(value);
            else if (index == Mainwin::NROTARY) // == kParameterM_CHANNEL
                mainwin->setchan_ui(value);

            return true;
        }

        if (std::strcmp(msg, "show") == 0)
        {
            mainwin->x_map();
            return true;
        }

        if (std::strcmp(msg, "hide") == 0)
        {
            mainwin->x_unmap();
            return true;
        }

        if (std::strcmp(msg, "focus") == 0)
        {
            mainwin->x_mapraised();
            return true;
        }

        if (std::strcmp(msg, "uiTitle") == 0)
        {
            const char* uiTitle;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(uiTitle), true);

            mainwin->x_set_title(uiTitle);
            return true;
        }

        if (std::strcmp(msg, "quit") == 0)
        {
            fQuitReceived = true;
            mainwin->stop();
            return true;
        }

        carla_stderr("ZitaPipeClient::msgReceived : %s", msg);
        return false;
    }

    void noteMaskChangedCallback(int mask)
    {
        if (! isPipeRunning())
            return;

        char tmpBuf[0xff+1];
        tmpBuf[0xff] = '\0';
        std::snprintf(tmpBuf, 0xff, "%i\n", mask);

        const CarlaMutexLocker cml(getPipeLock());

        writeMessage("zita-mask\n", 10);
        writeMessage(tmpBuf);
        flushMessages();
    }

    void valueChangedCallback(uint index, float value) override
    {
        if (! isPipeRunning())
            return;

        writeControlMessage(index, value);
    }

private:
    bool fQuitReceived;
};

// --------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    X_resman   xresman;
    X_display *display;
    X_handler *handler;
    X_rootwin *rootwin;
    int        ev, xp, yp, xs, ys;

    int   fake_argc   = 1;
    char* fake_argv[] = { (char*)"rev1" };
    xresman.init(&fake_argc, fake_argv, (char*)"rev1", nullptr, 0);

    display = new X_display(nullptr);
    if (display->dpy () == 0)
    {
        carla_stderr("Can't open display.");
        delete display;
        return 1;
    }

    ZitaPipeClient pipe;
    const char* uiTitle = "Test UI";

    if (argc > 1)
    {
        uiTitle = argv[2];

        if (! pipe.initPipeClient(argv))
            return 1;
    }

    xp = yp = 100;
    xs = Mainwin::XSIZE + 4;
    ys = Mainwin::YSIZE + 30;
    xresman.geometry(".geometry", display->xsize(), display->ysize(), 1, xp, yp, xs, ys);

    styles_init(display, &xresman);
    rootwin = new X_rootwin(display);
    mainwin = new Mainwin(rootwin, &xresman, xp, yp, &pipe);
    mainwin->x_set_title(uiTitle);
    rootwin->handle_event();
    handler = new X_handler(display, mainwin, EV_X11);
    handler->next_event();
    XFlush(display->dpy());

    if (argc == 1)
        mainwin->x_map();

    do
    {
        ev = mainwin->process();
        if (ev == EV_X11)
        {
            rootwin->handle_event();
            handler->next_event();
        }
        else if (ev == Esync::EV_TIME)
        {
            handler->next_event();

            if (pipe.isPipeRunning())
                pipe.idlePipe();
        }
    }
    while (ev != EV_EXIT && ! pipe.quitRequested());

    styles_fini(display);
    delete handler;
    delete rootwin;
    delete display;

    return 0;
}

// --------------------------------------------------------------------------------------------
