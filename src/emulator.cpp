#include "emulator.h"

static void kick_timer()
{
    extern uint32_t timer_callback(uint32_t interval, void * param);
    timer_callback(0, 0);
    DBG_QUEUE(putchar('K'); fflush(stdout););
}

Emulator::Emulator(Board & borat) : board(borat)
{
    board.onframetimer = [=]() {
        ui_to_engine_queue.push(threadevent(EXECUTE_FRAME, 0));
    };
}

bool Emulator::handle_keyboard_event(SDL_KeyboardEvent & event)
{
    switch (event.keysym.scancode) 
    {
        case SDL_SCANCODE_RETURN:
#if __WIN32__
            if (event.keysym.mod & KMOD_ALT) {	
#else 
                if (event.keysym.mod & KMOD_GUI) {
#endif
                    board.toggle_fullscreen();
                    return true;
                }
            break;
        default:
            break;
    }
    return false;
}

/* This part is copied from SDL_events.c SDL_WaitEventTimeout().
 * It is not acceptable to wait 10ms as written in the original code.
 * Because this loop is also used to receive render requests from the engine,
 * 10ms may create frame dropouts and it looks awful.
 *
 * The place of SDL_Delay(10) is now taken by sync_priority_queue<>::pull_for() 
 * with a timeout. I'm not sure why boost only has timed-out pull for priority
 * queues and not for regular ones.
 *
 * When the engine is done executing a frame, it posts a threadevent(RENDER)
 * to engine_to_ui_queue and it's supposed to instantly wake up the main
 * thread. 
 */
int Emulator::wait_event(SDL_Event * event, threadevent & ev, int timeout)
{
    Uint32 expiration = 0;

    if (timeout > 0)
        expiration = SDL_GetTicks() + timeout;

    for (;;) {
        SDL_PumpEvents();
        switch (SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, 
                    SDL_LASTEVENT)) 
        {
        case -1:
            return 0;
        case 0:
            if (timeout == 0) {
                /* Polling and no events, just return */
                return 0;
            }
            if (timeout > 0 && SDL_TICKS_PASSED(SDL_GetTicks(), expiration)) {
                /* Timeout expired and no events */
                return 0;
            }
            //SDL_Delay(10); pizdec
            {
                auto timeout = boost::chrono::milliseconds(10);
                static auto constexpr ok = boost::queue_op_status::success;
                if (engine_to_ui_queue.pull_for(timeout, ev) == ok) {
                    /* render request: fill in a dummy SDL event */
                    event->type = SDL_USEREVENT;
                    event->user.code = 0x80 | ev.data;

                    /* purge extra requests if any, they can accumulate if
                     * window is being dragged by its titlebar on windows */
                    int purge = 0;
                    while(engine_to_ui_queue.nonblocking_pull(ev) == ok) 
                        ++purge;
                    purge && Options.log.video && 
                        fprintf(stderr, "purged render(%d)\n", purge);
                    return 2;
                }
            }
            break;
        default:
            /* Has events */
            return 1;
        }
    }
}

void Emulator::handle_render(threadevent & event, bool & stopping)
{
    if (event.data) {
        int frame_no = event.frame_no;
        bool executed = event.data;
        DBG_QUEUE(putchar('r'); putchar('0' + executed););
        board.render_frame(frame_no, executed);
        if ((Options.nosound && Options.novideo) || 
                (Options.vsync && Options.vsync_enable)) {
            /* tests: kick-spin the event loop */
            kick_timer();
        }
        DBG_QUEUE(putchar('R'); fflush(stdout););
        if (Options.max_frame >= 0 && frame_no >= Options.max_frame) {
            ui_to_engine_queue.push(threadevent(QUIT, 0));
            stopping = true;
        }
    }
}

/* handle sdl events in the main thread */
void Emulator::run_event_loop()
{
    SDL_Event event;
    threadevent threadev;
    bool end = false;
    bool kickstart = true;
    /* tests: kick-spin the event loop */
    if (Options.nosound && Options.novideo) {
        kick_timer();
    }
    while(!end) {
        if (this->wait_event(&event, threadev, -1)) {
            switch(event.type) {
                case SDL_USEREVENT:
                    handle_render(threadev, end);
                    break;
                case SDL_KEYDOWN:
                    if (!this->handle_keyboard_event(event.key)) {
                        ui_to_engine_queue.push(threadevent(KEYDOWN, event.key));
                    }
                    break;
                case SDL_KEYUP:
                    ui_to_engine_queue.push(threadevent(KEYUP, event.key));
                    break;
                // this is to be handled in the ui thread
                case SDL_WINDOWEVENT:
                    //printf("windowevent: %x\n", event.window.event);
                    if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                        if (kickstart) {
                            kick_timer();
                            kickstart = false;
                        }
                    } 
                    board.handle_window_event(event);
                    break;
                case SDL_QUIT:
                    ui_to_engine_queue.push(threadevent(QUIT, 0));
                    end = true;
                    break;
                default:
                    break;
            }
        }
    }
    join_emulator_thread();
}


void Emulator::handle_threadevent(threadevent & event)
{
    //printf("handle_event: event.type=%d\n", event.type);
    switch(event.type) {
        case KEYDOWN:
            board.handle_keydown(event.key);
            break;
        case KEYUP:
            board.handle_keyup(event.key);
            break;
        case EXECUTE_FRAME:
            {
                int executed;
                if (Options.vsync && Options.vsync_enable) {
                    DBG_QUEUE(putchar('E'); fflush(stdout););
                    executed = board.execute_frame_with_cadence(true, true);
                } 
                else {
                    DBG_QUEUE(putchar('e'); fflush(stdout););
                    executed = board.execute_frame_with_cadence(true, false);
                }
                engine_to_ui_queue.push(threadevent(RENDER, executed, 
                            board.get_frame_no()));
            }
            break;
        case QUIT:
            board.handle_quit();
            break;
        default:
            break;
    }
}


/* emulator thread body */
void Emulator::threadfunc()
{
    for(int i = 0; !this->board.terminating(); ++i) {
        threadevent ev;
        if (ui_to_engine_queue.wait_pull(ev) == boost::queue_op_status::closed)
            break;
        handle_threadevent(ev);
        if (i == 0) {
            board.pause_sound(0);
        }
    }
}

void Emulator::start_emulator_thread()
{
    thread = boost::thread(&Emulator::threadfunc, this);
}

void Emulator::join_emulator_thread()
{
    thread.join();
}

