#include <gdnative_api_struct.gen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "v06x_class.h"

const godot_gdnative_core_api_struct* api = NULL;
const godot_gdnative_ext_nativescript_api_struct* nativescript_api = NULL;

GDCALLINGCONV void* simple_constructor(godot_object* p_instance,
  void* p_method_data);

GDCALLINGCONV void simple_destructor(godot_object* p_instance,
  void* p_method_data,
  void* p_user_data);


#define V06X_DECL(name) \
    godot_variant name(godot_object* p_instance, void* p_method_data, \
        void* p_user_data, int p_num_args, godot_variant** p_args)

V06X_DECL(V06X_godot_init);
V06X_DECL(V06X_Init);
V06X_DECL(V06X_ExecuteFrame);
V06X_DECL(V06X_GetSound);
V06X_DECL(V06X_KeyDown);
V06X_DECL(V06X_KeyUp);
V06X_DECL(V06X_LoadAsset);
V06X_DECL(V06X_Mount);
V06X_DECL(V06X_Reset);
V06X_DECL(V06X_ExportState);
V06X_DECL(V06X_RestoreState);
V06X_DECL(V06X_GetRusLat);
V06X_DECL(V06X_SetJoysticks);
V06X_DECL(V06X_SetVolumes);
V06X_DECL(V06X_GetMem);
V06X_DECL(V06X_GetHeatmap);
V06X_DECL(V06X_InsertBootROM);
V06X_DECL(V06X_SetScriptText);
V06X_DECL(V06X_AddScriptFile);
V06X_DECL(V06X_AppendScriptArg);
V06X_DECL(V06X_ExecuteScript);
V06X_DECL(debug_break);
V06X_DECL(debug_continue);
V06X_DECL(debug_read_registers);
V06X_DECL(debug_read_stack);
V06X_DECL(debug_step_into);
V06X_DECL(debug_get_disasm);
V06X_DECL(debug_add_breakpoint);
V06X_DECL(debug_del_breakpoint);
V06X_DECL(debug_add_watchpoint);
V06X_DECL(debug_del_watchpoint);
V06X_DECL(debug_is_break);
V06X_DECL(debug_read_executed_memory);
V06X_DECL(debug_read_hw_info);
V06X_DECL(debug_set_debugging);
V06X_DECL(debug_get_global_addr);
V06X_DECL(debug_get_trace_log);
V06X_DECL(debug_set_labels);

#undef V06X_DECL

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options* p_options)
{
    api = p_options->api_struct;

    // now find our extensions
    for (int i = 0; i < api->num_extensions; i++) {
        switch (api->extensions[i]->type) {
            case GDNATIVE_EXT_NATIVESCRIPT: {
                nativescript_api = (godot_gdnative_ext_nativescript_api_struct*)
                                     api->extensions[i];
            }; break;
            default:
                break;
        };
    };
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(
  godot_gdnative_terminate_options* p_options)
{
    api = NULL;
    nativescript_api = NULL;
}

extern "C" void GDN_EXPORT godot_nativescript_init(void* p_handle)
{
    godot_instance_create_func create = { NULL, NULL, NULL };
    create.create_func = &simple_constructor;

    godot_instance_destroy_func destroy = { NULL, NULL, NULL };
    destroy.destroy_func = &simple_destructor;

    nativescript_api->godot_nativescript_register_class(
      p_handle, "V06X", "Reference", create, destroy);

    godot_method_attributes attributes = { GODOT_METHOD_RPC_MODE_DISABLED };

#define V06X_METHOD(name,fnuc) { \
    godot_instance_method descr = { NULL, NULL, NULL };\
    descr.method = &fnuc;\
    nativescript_api->godot_nativescript_register_method(p_handle, "V06X",\
            name, attributes, descr);\
    }

    V06X_METHOD("Init", V06X_Init);
    V06X_METHOD("ExecuteFrame", V06X_ExecuteFrame);
    V06X_METHOD("GetSound", V06X_GetSound);
    V06X_METHOD("KeyDown", V06X_KeyDown);
    V06X_METHOD("KeyUp", V06X_KeyUp);
    V06X_METHOD("LoadAsset", V06X_LoadAsset);
    V06X_METHOD("Mount", V06X_Mount);
    V06X_METHOD("Reset", V06X_Reset);
    V06X_METHOD("ExportState", V06X_ExportState);
    V06X_METHOD("RestoreState", V06X_RestoreState);
    V06X_METHOD("GetRusLat", V06X_GetRusLat);
    V06X_METHOD("SetJoysticks", V06X_SetJoysticks);
    V06X_METHOD("SetVolumes", V06X_SetVolumes);
    V06X_METHOD("GetMem", V06X_GetMem);
    V06X_METHOD("GetHeatmap", V06X_GetHeatmap);
    V06X_METHOD("InsertBootROM", V06X_InsertBootROM);
    V06X_METHOD("SetScriptText", V06X_SetScriptText);
    V06X_METHOD("AddScriptFile", V06X_AddScriptFile);
    V06X_METHOD("ExecuteScript", V06X_ExecuteScript);
    V06X_METHOD("AppendScriptArg", V06X_AppendScriptArg);
    V06X_METHOD("debug_break", debug_break);
    V06X_METHOD("debug_continue", debug_continue);
    V06X_METHOD("debug_read_registers", debug_read_registers);
    V06X_METHOD("debug_read_stack", debug_read_stack);
    V06X_METHOD("debug_step_into", debug_step_into);
    V06X_METHOD("debug_get_disasm", debug_get_disasm);
    V06X_METHOD("debug_add_breakpoint", debug_add_breakpoint);
    V06X_METHOD("debug_del_breakpoint", debug_del_breakpoint);
    V06X_METHOD("debug_add_watchpoint", debug_add_watchpoint);
    V06X_METHOD("debug_del_watchpoint", debug_del_watchpoint);
    V06X_METHOD("debug_is_break", debug_is_break);
    V06X_METHOD("debug_read_executed_memory", debug_read_executed_memory);
    V06X_METHOD("debug_read_hw_info", debug_read_hw_info);
    V06X_METHOD("debug_set_debugging", debug_set_debugging);
    V06X_METHOD("debug_get_global_addr", debug_get_global_addr);
    V06X_METHOD("debug_get_trace_log", debug_get_trace_log);
    V06X_METHOD("debug_set_labels", debug_set_labels);
#undef V06X_METHOD
}

GDCALLINGCONV void* simple_constructor(godot_object* p_instance,
  void* p_method_data)
{
    v06x_user_data* v = static_cast<v06x_user_data *>(api->godot_alloc(sizeof(v06x_user_data)));
    memset(v, 0, sizeof *v);
    v->initialized = false;

    return v;
}

GDCALLINGCONV void simple_destructor(godot_object* p_instance,
  void* p_method_data, void* p_user_data)
{
    v06x_user_data* v = static_cast<v06x_user_data *>(p_user_data);

    if (v->initialized) {
        api->godot_pool_byte_array_destroy(&v->bitmap);
        api->godot_pool_vector2_array_destroy(&v->sound);
        api->godot_pool_byte_array_destroy(&v->state);
        api->godot_pool_byte_array_destroy(&v->memory);
        api->godot_pool_byte_array_destroy(&v->heatmap);
    }
 
    api->godot_free(p_user_data);
}

