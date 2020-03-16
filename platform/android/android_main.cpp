#include <jni.h>
#include <android/log.h>

#include <iostream>
#include <fstream>
#include <iterator>

#include "memory.h"
#include "io.h"
#include "tv.h"
#include "board.h"
#include "emulator.h"
#include "options.h"
#include "keyboard.h"
#include "8253.h"
#include "sound.h"
#include "ay.h"
#include "wav.h"
#include "util.h"


Memory memory;
FD1793_Real fdc;
FD1793 fdc_dummy;
Wav wav;
WavPlayer tape_player(wav);
Keyboard keyboard;
I8253 timer;
TimerWrapper tw(timer);
AY ay;
AYWrapper aw(ay);
Soundnik soundnik(tw, aw);
IO io(memory, keyboard, timer, fdc, ay, tape_player);//Options.nofdc ? fdc_dummy : fdc);
TV tv;
PixelFiller filler(memory, io, tv);
Board board(memory, io, filler, soundnik, tv, tape_player);
Emulator lator(board);

struct LoadKind {
    int ROM;
    int COM;
    int FDD;
    int EDD;

    static int getStaticInt(JNIEnv * env, jclass cls, const char * name) {
        jfieldID id = env->GetStaticFieldID(cls, name, "I");
        jint value = env->GetStaticIntField(cls, id);
        return reinterpret_cast<int>(value);
    }

    void init(JNIEnv * env) {
        jclass romitem = env->FindClass("com/svofski/v06x/cpp/LoadKind");
        ROM = getStaticInt(env, romitem, "ROM");
        COM = getStaticInt(env, romitem, "COM");
        FDD = getStaticInt(env, romitem, "FDD");
        EDD = getStaticInt(env, romitem, "EDD");
    }
} LOADKIND;

extern "C" JNIEXPORT jint JNICALL
Java_com_svofski_v06x_cpp_Emulator_Init(JNIEnv *env, jobject /* this */)
{
    WavRecorder rec;
    WavRecorder * prec = 0;

    if (Options.audio_rec_path.length()) {
        rec.init(Options.audio_rec_path);
        prec = &rec;
    }

    filler.init();
    soundnik.init(prec);    // this may switch the audio output off
    tv.init();
    board.init();
    fdc.init();
    if (Options.bootpalette) {
        io.yellowblue();
    }

    keyboard.onreset = [](bool blkvvod) {
        board.reset(blkvvod ?
                Board::ResetMode::BLKVVOD : Board::ResetMode::BLKSBR);
    };

    if (Options.autostart) {
        int seq = 0;
        io.onruslat = [&seq](bool ruslat) {
            seq = (seq << 1) | (ruslat ? 1 : 0);
            if ((seq & 15) == 6) {
                board.reset(Board::ResetMode::BLKSBR);
                io.onruslat = nullptr;
            }
        };
    }

    board.reset(Board::ResetMode::BLKVVOD);

//    if (Options.wavfile.length() != 0) {
//        load_wav(wav, Options.wavfile);
//    }
//
//    load_disks(fdc);


//    bootstrap_scriptnik();

    LOADKIND.init(env);

    return (signed)0xdeadbeef;
}

void load_rom(uint8_t * bytes, size_t size, int org)
{
    std::vector<uint8_t> bin(bytes, bytes + size);
    memory.init_from_vector(bin, org);
}

void load_fdd(uint8_t * bytes, size_t size, int drive)
{
    std::vector<uint8_t> fdd(bytes, bytes + size);
    fdc.loadDsk(drive, "", fdd);
}

void load_edd(uint8_t * bytes, size_t size, int slot)
{
    std::vector<uint8_t> edd(bytes, bytes+size);
    memory.init_from_vector(edd, 0x10000 + slot * 0x40000);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_svofski_v06x_cpp_Emulator_ExecuteFrame(JNIEnv *env, jobject self,  jbyteArray pixels,
        jfloatArray samples)
{
    lator.execute_frame();

    // TODO: try GetPrimitiveArrayCritical
    jboolean isCopy = 0;
    jbyte * jbytes = env->GetByteArrayElements(pixels, &isCopy);
    lator.export_pixel_bytes((uint8_t *)jbytes);
    env->ReleaseByteArrayElements(pixels, jbytes, isCopy ? JNI_COMMIT : JNI_ABORT);
    // Also note that the JNI_COMMIT flag does not release the array,
    // and you will need to call Release again with a different flag eventually.


    jfloat * jfloats = env->GetFloatArrayElements(samples, &isCopy);
    jsize framesize = env->GetArrayLength(samples);
    lator.export_audio_frame(jfloats, (size_t) framesize);
    env->ReleaseFloatArrayElements(samples, jfloats, isCopy ? JNI_COMMIT : JNI_ABORT);

    return 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_svofski_v06x_cpp_Emulator_KeyDown(JNIEnv *env, jobject self, jint scancode)
{
    lator.keydown((int)scancode);
}

extern "C" JNIEXPORT void JNICALL
Java_com_svofski_v06x_cpp_Emulator_KeyUp(JNIEnv *env, jobject self, jint scancode)
{
    lator.keyup((int)scancode);
}



extern "C" JNIEXPORT void JNICALL
Java_com_svofski_v06x_cpp_Emulator_LoadAsset(JNIEnv *env, jobject self, jbyteArray asset, jint kind,
    jint org)
{
    jsize size = env->GetArrayLength(asset);
    jbyte * jbytes = env->GetByteArrayElements(asset, NULL);
    if (kind == LOADKIND.COM || kind == LOADKIND.ROM) {
        load_rom((uint8_t *)jbytes, size, org);
    }
    else if (kind == LOADKIND.FDD) {
        load_fdd((uint8_t *)jbytes, size, org);
    }
    else if (kind == LOADKIND.EDD) {
        load_edd((uint8_t *)jbytes, size, org);
    }

    env->ReleaseByteArrayElements(asset, jbytes, JNI_ABORT);
}

extern "C" JNIEXPORT void JNICALL
Java_com_svofski_v06x_cpp_Emulator_reset(JNIEnv *env, jobject /* this */, jboolean blkvvod)
{
    board.reset(blkvvod ?
                Board::ResetMode::BLKVVOD : Board::ResetMode::BLKSBR);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_svofski_v06x_cpp_Emulator_ExportState(JNIEnv * env, jobject) {
    std::vector<uint8_t> state;
    lator.save_state(state);
    jbyteArray out_state = env->NewByteArray(state.size());

    jbyte * jbytes = env->GetByteArrayElements(out_state, NULL);
    std::copy(state.begin(), state.end(), jbytes);

    env->ReleaseByteArrayElements(out_state, jbytes, JNI_COMMIT);

    return out_state;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_svofski_v06x_cpp_Emulator_RestoreState(JNIEnv * env, jobject, jbyteArray in_state) {
    jsize size = env->GetArrayLength(in_state);
    jbyte * jbytes = env->GetByteArrayElements(in_state, NULL);

    std::vector<uint8_t> state((uint8_t *)jbytes, (uint8_t *)jbytes + size);
    jboolean result = lator.restore_state(state);

    env->ReleaseByteArrayElements(in_state, jbytes, JNI_ABORT);

    return result;
}

