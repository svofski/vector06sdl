#include <gdnative_api_struct.gen.h>

#include <iostream>
#include <fstream>
#include <iterator>

#include "memory.h"
#include "vio.h"
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

#include "debug.h"

#if 0
#include "scriptnik.h"
#endif

#include "v06x_class.h"

extern const godot_gdnative_core_api_struct* api;

Memory memory;
FD1793 fdc;
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
Debug debug(&board, &memory);
#if 0
Scriptnik scriptnik;
#endif

struct LoadKind {
	enum {
		ROM = 0,
		COM = 1,
		FDD = 2,
		EDD = 3,
		WAV = 4
	};
} LOADKIND;

void install_ruslat_handler(v06x_user_data * v)
{
	io.onruslat = [v](bool ruslat_new) {
		if (v->autostart_armed) {
			v->autostart_seq = (v->autostart_seq << 1) | (ruslat_new ? 1 : 0);
			if ((v->autostart_seq & 15) == 6) {
				board.reset(Board::ResetMode::BLKSBR);
				v->autostart_armed = false;
			}
		}
		v->ruslat = ruslat_new;
	};

//    if (Options.autostart) {
//        int seq = 0;
//        io.onruslat = [&seq](bool ruslat) {
//            seq = (seq << 1) | (ruslat ? 1 : 0);
//            if ((seq & 15) == 6) {
//                board.reset(Board::ResetMode::BLKSBR);
//                io.onruslat = nullptr;
//            }
//        };
//    }
}

//extern "C" JNIEXPORT jint JNICALL
//Java_com_svofski_v06x_cpp_Emulator_Init(JNIEnv *env, jobject /* this */)
godot_variant V06X_Init(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
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


	auto v = static_cast<v06x_user_data *>(p_user_data);
	keyboard.onreset = [v](bool blkvvod) {
		board.reset(blkvvod ?
				Board::ResetMode::BLKVVOD : Board::ResetMode::BLKSBR);
		v->autostart_armed = blkvvod && Options.autostart;
	};

	install_ruslat_handler(v);

	board.reset(Board::ResetMode::BLKVVOD);
	v->autostart_armed = Options.autostart;

//    bootstrap_scriptnik();

	godot_variant hello;
	api->godot_variant_new_uint(&hello, 0xdeadbeef);
	return hello;
}

void load_rom(const uint8_t * bytes, size_t size, int org)
{
	std::vector<uint8_t> bin(bytes, bytes + size);
	memory.init_from_vector(bin, org);
}

void load_fdd(const uint8_t * bytes, size_t size, int drive)
{
	std::vector<uint8_t> fdd_image(bytes, bytes + size);
	fdc.disk(drive).attach(fdd_image);
}

void load_edd(const uint8_t * bytes, size_t size, int slot)
{
	std::vector<uint8_t> edd(bytes, bytes+size);
	memory.init_from_vector(edd, 0x10000 + slot * 0x40000);
}

void load_wav(const uint8_t * bytes, size_t size)
{
	std::vector<uint8_t> v(bytes, bytes+size);
	wav.set_bytes(v);
}

//Java_com_svofski_v06x_cpp_Emulator_ExecuteFrame(JNIEnv *env, jobject self,  jbyteArray pixels, jfloatArray samples)
godot_variant V06X_ExecuteFrame(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	lator.execute_frame();

	// obtain instance user data
	v06x_user_data * v = static_cast<v06x_user_data *>(p_user_data);
	// initialize the arrays if this is the first frame
	if (!v->initialized) {
		api->godot_pool_byte_array_new(&v->bitmap);
		api->godot_pool_byte_array_resize(&v->bitmap, lator.pixel_bytes_size());

		api->godot_pool_vector2_array_new(&v->sound); // but set size...

		api->godot_pool_byte_array_new(&v->state); // for serializing

		api->godot_pool_byte_array_new(&v->memory); // for dump / debug
		api->godot_pool_byte_array_resize(&v->memory, memory.buffer_size());

		api->godot_pool_byte_array_new(&v->heatmap);
		api->godot_pool_byte_array_resize(&v->heatmap, memory.get_heatmap().size());

		v->initialized = true;
	}

	// obtain the pointer to bytes
	godot_pool_byte_array_write_access * wraccess = 
		api->godot_pool_byte_array_write(&v->bitmap);
	uint8_t * wrptr = api->godot_pool_byte_array_write_access_ptr(wraccess);

	lator.export_pixel_bytes(wrptr);

	memory.cool_off_heatmap();

	// destroy write_access
	api->godot_pool_byte_array_write_access_destroy(wraccess);

	godot_variant ret;
	api->godot_variant_new_pool_byte_array(&ret, &v->bitmap);

	return ret;
}

// arg0: int sample size in funny units
// returns Vector2 array
godot_variant V06X_GetSound(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_int nsamples = api->godot_variant_as_int(p_args[0]);

	// obtain instance user data
	v06x_user_data * v = static_cast<v06x_user_data *>(p_user_data);

	godot_int current_size = api->godot_pool_vector2_array_size(&v->sound);
	if (current_size != nsamples) {
		api->godot_pool_vector2_array_resize(&v->sound, nsamples);
	}

	// get the write ptr
	auto wraccess = api->godot_pool_vector2_array_write(&v->sound);
	godot_vector2 * wrptr = api->godot_pool_vector2_array_write_access_ptr(wraccess);

	lator.export_audio_frame(reinterpret_cast<float *>(wrptr), (size_t) nsamples * 2);

	api->godot_pool_vector2_array_write_access_destroy(wraccess);

	godot_variant ret;
	api->godot_variant_new_pool_vector2_array(&ret, &v->sound);
	return ret;
}

/** 
 * Args: ()
 * Returns: bool, true if Rus/Lat LED is on
 */
godot_variant V06X_GetRusLat(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_variant ret;
	v06x_user_data * v = static_cast<v06x_user_data *>(p_user_data);
	api->godot_variant_new_bool(&ret, v->ruslat);
	return ret;
}


//extern "C" JNIEXPORT void JNICALL
//Java_com_svofski_v06x_cpp_Emulator_KeyDown(JNIEnv *env, jobject self, jint scancode)
godot_variant V06X_KeyDown(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_int scancode = api->godot_variant_as_int(p_args[0]);
	lator.keydown((int)scancode);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

//extern "C" JNIEXPORT void JNICALL
//Java_com_svofski_v06x_cpp_Emulator_KeyUp(JNIEnv *env, jobject self, jint scancode)
godot_variant V06X_KeyUp(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_int scancode = api->godot_variant_as_int(p_args[0]);
	lator.keyup((int)scancode);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

godot_variant V06X_SetJoysticks(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_int joy_0e = api->godot_variant_as_int(p_args[0]);
	godot_int joy_0f = api->godot_variant_as_int(p_args[1]);
	lator.set_joysticks((int)joy_0e, (int)joy_0f);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

//extern "C" JNIEXPORT void JNICALL
//Java_com_svofski_v06x_cpp_Emulator_LoadAsset(JNIEnv *env, jobject self, jbyteArray asset, jint kind,
//    jint org)
godot_variant V06X_LoadAsset(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_pool_byte_array bytes = api->godot_variant_as_pool_byte_array(p_args[0]);
	godot_int kind = api->godot_variant_as_int(p_args[1]);
	godot_int org  = api->godot_variant_as_int(p_args[2]);

	godot_int size = api->godot_pool_byte_array_size(&bytes);
	auto ra = api->godot_pool_byte_array_read(&bytes);
	const uint8_t * ptr = api->godot_pool_byte_array_read_access_ptr(ra);

	switch (kind) {
		case LOADKIND.COM:
		case LOADKIND.ROM:
			load_rom(ptr, size, org);
			break;
		case LOADKIND.FDD:
			load_fdd(ptr, size, org);
			break;
		case LOADKIND.EDD:
			load_edd(ptr, size, org);
			break;
		case LOADKIND.WAV:
			load_wav(ptr, size);
	}

	api->godot_pool_byte_array_read_access_destroy(ra);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

// dir or fdd
godot_variant V06X_Mount(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_int device = api->godot_variant_as_int(p_args[0]);
	godot_string wpath = api->godot_variant_as_string(p_args[1]);
	godot_char_string cpath = api->godot_string_ascii(&wpath);
	
	const char * path = api->godot_char_string_get_data(&cpath);

	try
	{
		fdc.disk(device).attach(path);
	}
	catch(...)
	{
		printf("Mount: dev=%d path=%s failed\n", device, path);
	}

	api->godot_char_string_destroy(&cpath);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

//extern "C" JNIEXPORT void JNICALL
//Java_com_svofski_v06x_cpp_Emulator_reset(JNIEnv *env, jobject /* this */, jboolean blkvvod)
godot_variant V06X_Reset(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_bool blkvvod = api->godot_variant_as_bool(p_args[0]);
	board.reset(blkvvod ?
				Board::ResetMode::BLKVVOD : Board::ResetMode::BLKSBR);

	auto v = static_cast<v06x_user_data *>(p_user_data);
	v->autostart_armed = blkvvod && Options.autostart;

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

//extern "C" JNIEXPORT jbyteArray JNICALL
//Java_com_svofski_v06x_cpp_Emulator_ExportState(JNIEnv * env, jobject) {
godot_variant V06X_ExportState(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	std::vector<uint8_t> state;
	lator.save_state(state);

	auto v = static_cast<v06x_user_data *>(p_user_data);
	api->godot_pool_byte_array_resize(&v->state, state.size());

	auto wraccess = api->godot_pool_byte_array_write(&v->state);
	uint8_t * wrptr = api->godot_pool_byte_array_write_access_ptr(wraccess);

	std::copy(state.begin(), state.end(), wrptr);

	api->godot_pool_byte_array_write_access_destroy(wraccess);

	godot_variant ret;
	api->godot_variant_new_pool_byte_array(&ret, &v->state);
	return ret;
}

//extern "C" JNIEXPORT jboolean JNICALL
//Java_com_svofski_v06x_cpp_Emulator_RestoreState(JNIEnv * env, jobject, jbyteArray in_state) {
godot_variant V06X_RestoreState(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_pool_byte_array bytes = api->godot_variant_as_pool_byte_array(p_args[0]);
	godot_int size = api->godot_pool_byte_array_size(&bytes);
	auto ra = api->godot_pool_byte_array_read(&bytes);
	const uint8_t * ptr = api->godot_pool_byte_array_read_access_ptr(ra);

	std::vector<uint8_t> state((uint8_t *)ptr, (uint8_t *)ptr + size);
	bool result = lator.restore_state(state);

	api->godot_pool_byte_array_read_access_destroy(ra);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, result);
	return ret;
}

godot_variant V06X_SetVolumes(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	double vol8253 = api->godot_variant_as_real(p_args[0]);
	double volBeep = api->godot_variant_as_real(p_args[1]);
	double volAY   = api->godot_variant_as_real(p_args[2]);
	double volCovox= api->godot_variant_as_real(p_args[3]);
	double volMaster= api->godot_variant_as_real(p_args[4]);

	bool tch0 = api->godot_variant_as_bool(p_args[5]);
	bool tch1 = api->godot_variant_as_bool(p_args[6]);
	bool tch2 = api->godot_variant_as_bool(p_args[7]);

	bool aych0 = api->godot_variant_as_bool(p_args[8]);
	bool aych1 = api->godot_variant_as_bool(p_args[9]);
	bool aych2 = api->godot_variant_as_bool(p_args[10]);

	lator.set_volumes(vol8253, volBeep, volAY, volCovox, volMaster);
	lator.enable_timer_channels(tch0, tch1, tch2);
	lator.enable_ay_channels(aych0, aych1, aych2);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

// GetMem(int addr, int length) -> PoolArrayBytes
godot_variant V06X_GetMem(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_variant ret;
	// obtain instance user data
	v06x_user_data * v = static_cast<v06x_user_data *>(p_user_data);

	if (!v->initialized) {
		api->godot_variant_new_bool(&ret, 0);
		return ret;
	}

	godot_int addr = api->godot_variant_as_int(p_args[0]);
	godot_int size = api->godot_variant_as_int(p_args[1]);

	// obtain the pointer to bytes
	godot_pool_byte_array_write_access * wraccess = 
		api->godot_pool_byte_array_write(&v->memory);
	uint8_t * wrptr = api->godot_pool_byte_array_write_access_ptr(wraccess);

	memory.export_bytes(wrptr, addr, size);

	// destroy write_access
	api->godot_pool_byte_array_write_access_destroy(wraccess);

	api->godot_variant_new_pool_byte_array(&ret, &v->memory);
	
	return ret;
}

// GetHeatmap(addr, length) -> PoolIntArray
godot_variant V06X_GetHeatmap(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	godot_variant ret;
	// obtain instance user data
	v06x_user_data * v = static_cast<v06x_user_data *>(p_user_data);

	if (!v->initialized) {
		api->godot_variant_new_bool(&ret, 0);
		return ret;
	}

	godot_int addr = api->godot_variant_as_int(p_args[0]);
	godot_int size = api->godot_variant_as_int(p_args[1]);

	// obtain the pointer to ints
	godot_pool_byte_array_write_access * wraccess = 
		api->godot_pool_byte_array_write(&v->heatmap);
	uint8_t * wrptr = api->godot_pool_byte_array_write_access_ptr(wraccess);

	std::copy(memory.get_heatmap().begin(), memory.get_heatmap().end(),
			wrptr);


	// destroy write_access
	api->godot_pool_byte_array_write_access_destroy(wraccess);

	api->godot_variant_new_pool_byte_array(&ret, &v->heatmap);
	return ret;
}

godot_variant debug_break(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	v06x_user_data* user_data_p = static_cast<v06x_user_data*>(p_user_data);
	if (!user_data_p->initialized) {
		godot_variant ret;
		api->godot_variant_new_bool(&ret, 0);
		return ret;
	}

	board.debugger_attached();

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

godot_variant debug_continue(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	v06x_user_data* user_data_p = static_cast<v06x_user_data*>(p_user_data);
	if (!user_data_p->initialized) {
		godot_variant ret;
		api->godot_variant_new_bool(&ret, 0);
		return ret;
	}

	board.debugger_continue();

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}

godot_variant debug_read_registers(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	v06x_user_data* user_data_p = static_cast<v06x_user_data*>(p_user_data);
	if (!user_data_p->initialized) {
		godot_variant ret;
		api->godot_variant_new_bool(&ret, 0);
		return ret;
	}
	auto regsV = board.read_registers_b();

    godot_pool_int_array int_array_gd;
    api->godot_pool_int_array_new(&int_array_gd);
	api->godot_pool_int_array_resize(&int_array_gd, regsV.size());

	godot_pool_int_array_write_access * int_array_gd_wa = 
		api->godot_pool_int_array_write(&int_array_gd);
	godot_int* int_array_gd_waptr = api->godot_pool_int_array_write_access_ptr(int_array_gd_wa);

	std::copy(regsV.begin(), regsV.end(), int_array_gd_waptr);


	api->godot_pool_int_array_write_access_destroy(int_array_gd_wa);
	godot_variant ret;
	api->godot_variant_new_pool_int_array(&ret, &int_array_gd);

	return ret;
}

godot_variant debug_step_into(godot_object* p_instance, void* p_method_data, 
		void* p_user_data, int p_num_args, godot_variant** p_args)
{
	v06x_user_data* user_data_p = static_cast<v06x_user_data*>(p_user_data);
	if (!user_data_p->initialized) {
		godot_variant ret;
		api->godot_variant_new_bool(&ret, 0);
		return ret;
	}

	board.single_step(true);

	godot_variant ret;
	api->godot_variant_new_bool(&ret, 1);
	return ret;
}