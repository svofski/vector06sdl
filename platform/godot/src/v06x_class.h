#pragma once

typedef struct v06x_user_data
{
    bool initialized;
    bool ruslat;
    int autostart_seq;
    bool autostart_armed;

    godot_pool_byte_array bitmap;
    godot_pool_vector2_array sound;

    godot_pool_byte_array state;

    godot_pool_byte_array memory;

    godot_pool_byte_array heatmap;
} v06x_user_data;
