shader_type canvas_item;

uniform sampler2D mem; // hint_fuck
uniform sampler2D heatmap; // heat per byte
uniform vec2 mem_sz;    // input texture pixel size (16, 5120)
uniform vec2 gobshite;  // output texture pixel size (512, 5120)


// rearrange in columns


vec4 gruu(vec2 uv)
{
    ivec2 screen_xy = ivec2(floor(uv * gobshite)); // 0,0+512x5120

    // baseofs == memory start at current page
    // 2 256x256 screens in every y row
    int baseofs = screen_xy.y / 256;
    baseofs = baseofs * 2 * 8192;

    int yfb = 255 - screen_xy.y % 256;
    int fb = (screen_xy.x / 8) * 256 + yfb;
    fb = fb + baseofs;
    int texel_x = fb % int(floor(mem_sz.x));
    int texel_y = fb / int(floor(mem_sz.x));

    // 4 lines of 8 pixels
    int b = ivec4(floor(255.0 * texelFetch(mem, ivec2(texel_x, texel_y), 0))).r;
    int shitf = screen_xy.x % 8;

    float bit = float( (((255 & b) << shitf) & 128) == 128 );

    float heat = texelFetch(heatmap, ivec2(texel_x, texel_y), 0).r;

    // tiles for visual cues
    int tilex = (screen_xy.x / 256) % 2;
    int tiley = (screen_xy.y / 256) % 2;
    float tile = float(tilex ^ tiley);

    vec4 tilecolor = vec4(0.0, ((1.0 - tile) * 0.1), 0.1 + (tile * 0.1), 1.0);
    vec4 color = vec4(bit, bit, bit, 1.0);

    color = color * bit + tilecolor * (1.0 - bit);
    //return color;


    vec4 heater = vec4(heat, 0.0, 0.0, 1.0);


    return mix(color, heater, 0.5);
}

void fragment() {
    vec4 c = gruu(UV);
    COLOR.rgba = c;
}
