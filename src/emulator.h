#include "SDL.h"

#include <boost/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/concurrent_queues/sync_priority_queue.hpp>

#include "board.h"

class Emulator {
private:
    enum event_type {
        /* ui to emulator thread */
        EXECUTE_FRAME,
        KEYDOWN,
        KEYUP,
        QUIT,
        /* emulator to ui */
        RENDER,
    };

    struct threadevent {
        event_type type;
        int data;
        int frame_no;
        SDL_KeyboardEvent key;
        threadevent() {}
        threadevent(event_type t, int d) : type(t), data(d) {}
        threadevent(event_type t, int d, int frameno) : type(t), data(d),
            frame_no(frameno) {}
        threadevent(event_type t, SDL_KeyboardEvent k) : 
            type(t), data(0), key(k) {}

        bool operator <(const threadevent& other) const
        {
            return false;
        }
    };

    boost::thread thread;
    Board & board;

    boost::sync_queue<threadevent> ui_to_engine_queue;
    boost::sync_priority_queue<threadevent> engine_to_ui_queue;

private:
    void threadfunc();
    void handle_threadevent(threadevent & ev);
    void handle_render(threadevent & event, bool & stopping);
    void join_emulator_thread();
    bool handle_keyboard_event(SDL_KeyboardEvent & event);
    int wait_event(SDL_Event * event, threadevent & ev, int timeout);

public:
    Emulator(Board & borat);
    void run_event_loop();
    void start_emulator_thread();
};
