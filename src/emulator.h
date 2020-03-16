#ifndef __ANDROID__
#include "SDL.h"

#include <boost/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/concurrent_queues/sync_priority_queue.hpp>
#else
#include <pthread.h>
#endif

#include "board.h"

class Emulator {
    static const int N_SCANCODES = 4;

private:
    enum event_type {
        /* ui to emulator thread */
        EXECUTE_FRAME,
        KEYDOWN,
        KEYUP,
        QUIT,
        /* emulator to ui */
        RENDER,
        /* DUMMY */
        VACANT
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

    Board & board;

#ifndef __ANDROID__
    boost::thread thread;
    boost::sync_queue<threadevent> ui_to_engine_queue;
    boost::sync_priority_queue<threadevent> engine_to_ui_queue;
#else
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    threadevent ui_to_engine_event;
    threadevent engine_to_ui_event;
    int keydowns[N_SCANCODES];
    int keyups[N_SCANCODES];
#endif

private:
#ifndef __ANDROID__
    void threadfunc();
    void handle_threadevent(threadevent & ev);
    void handle_render(threadevent & event, bool & stopping);
    void join_emulator_thread();
    bool handle_keyboard_event(SDL_KeyboardEvent & event);
    int wait_event(SDL_Event * event, threadevent & ev, int timeout);
#else
public:
    void execute_frame(); // execute frame in current thread, no mt stuff
    void keydown(int scancode);
    void keyup(int scancode);
    void export_pixel_bytes(uint8_t * dst);
    void export_audio_frame(float * dst, size_t count);
    size_t pixel_bytes_size();
#endif

public:
    Emulator(Board & borat);
    void run_event_loop();
    void start_emulator_thread();

    void save_state(vector<uint8_t> & to);
    bool restore_state(vector<uint8_t> & to);
};
