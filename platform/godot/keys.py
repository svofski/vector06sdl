#!/usr/bin/env python3

"""
Takes api.json and outputs C++ enums based on KEY_ and JOY_ values.

Invoke like ./keys.py >src/godot_scancodes.h
"""

import json

godot2sdl = {
        'EQUAL':        'EQUALS',
        'ENTER':        'RETURN',
        'BRACKETLEFT':  'LEFTBRACKET',
        'BRACKETRIGHT': 'RIGHTBRACKET',
        'ALT':          'LALT',
        'SHIFT':        'LSHIFT',
        'CONTROL':      'LCTRL',
        'META':         'LGUI',
        'QUOTELEFT':    'GRAVE',
        }

lopsided={'LALT': 'RALT', 'LSHIFT': 'RSHIFT', 'LCTRL' : 'RCTRL'}


with open('godot_headers/api.json', 'r') as fapi:
    j = json.load(fapi)
    constants = j[0]['constants']

    print("    enum GodotScancodes {");
    for k,v in constants.items():
        if k.startswith("KEY_"):
            godot_name = k[4:]
            sdl_name = godot2sdl.get(godot_name, godot_name)
            constname = "SDL_SCANCODE_" + sdl_name
            print(f"        {constname} = {v},")
            if sdl_name in lopsided:
                fakeval = -v
                print(f"        SDL_SCANCODE_{lopsided[sdl_name]} = {fakeval},")

    print("    };")

    print("    enum GodotJoystick {");
    for k,v in constants.items():
        if k.startswith("JOY_"):
            print(f"        SDL_{k} = {v},")

    print("    };")
