shader_type canvas_item;

//uniform float filter_gain;

const float filter_gain = 1.6;

const float PI = 3.14159265358;
const float FSC = 4433618.75;
const float LINETIME = 64.0e-6; // 64 us total
const float VISIBLE = 52.0e-6; // 52 us visible part
const float FLINE = (1.0/LINETIME); // =15625 for 64ms, but = 19230 accounting for visible part only
const float VISIBLELINES = 288.0; //312

const mat3 RGB_to_YIQ = mat3( vec3(0.299 , 0.595716 , 0.211456) ,  vec3(0.587    , -0.274453 , -0.522591) ,     vec3( 0.114    , -0.321263 , 0.311135) );
const mat3 YIQ_to_RGB = mat3( vec3(1.0   , 1.0      , 1.0     ) ,  vec3(0.9563   , -0.2721   , -1.1070  ) ,     vec3( 0.6210   , -0.6474   , 1.7046  ) );

const mat3 RGB_to_YUV = mat3( vec3( 0.299 , -0.14713 , 0.615  )  , vec3(  0.587    , -0.28886  , -0.514991) ,   vec3(   0.114    , 0.436     , -0.10001 ));
const mat3 YUV_to_RGB = mat3( vec3( 1.0   , 1.0      , 1.0    )  , vec3(  0.0      , -0.39465  , 2.03211  ) ,   vec3(   1.13983  , -0.58060  , 0.0      ));

vec4 fetch(sampler2D tex, int ofs, vec2 center, float invx)
{
  return texture(tex, vec2(float(ofs) * (invx) + center.x, center.y));
}

const int FIRTAPS = 20;
const float FIR[20] = float[20] (0.032510684078502015,0.037230050756041834,0.041843026634912281,0.046224816069039297,0.050252793446409441,0.053811167664048955,0.056795464455619823,0.059116625356802809,0.060704537603613343,0.061510833935010188,0.061510833935010188,0.060704537603613343,0.059116625356802809,0.056795464455619823,0.053811167664048955,0.050252793446409441,0.046224816069039297,0.041843026634912281,0.037230050756041834,0.032510684078502015);

float luma_only(sampler2D tex, vec2 xy, float invx)
{
    vec3 rgb = fetch(tex, 0, xy, invx).xyz;
    vec3 yuv = RGB_to_YUV * rgb;
    return yuv.x;
}

vec2 modem_uv(sampler2D tex, vec2 xy, int ofs, float invx, float altv, float width_ratio, vec2 texsize) {
    float t = (xy.x + float(ofs) * invx) * texsize.x;
    float wt = t * 2. * PI / width_ratio;

    float sinwt = sin(wt);
    float coswt = cos(wt + altv);

    vec3 rgb = fetch(tex, ofs, xy, invx).xyz;
    vec3 yuv = RGB_to_YUV * rgb;
    float signal = yuv.x + yuv.y * sinwt + yuv.z * coswt;
    return vec2(signal * sinwt, signal * coswt);
}

// Scanline divider depends on how the screen is scanned in the machine
// Machines that simply display 2 equal fields, like Atari 8-bit
// need this value to be 1.0 for correct scanlines, because they have
// 2 TV lines per horizontal pixel.
// Unfortunately, this has no chance of looking decent on medium
// resolution LCD with less than 1000 lines to display.
// Decent looking fake effect can be achieved by using non-integer values.
const float DIV_ATARI = 2.0;
const float DIV_MSX = 4.0;
const float SCANLINE_DIV = 3.;

//const float VFREQ = PI*(screen_texture_sz.y)/SCANLINE_DIV;
const float VFREQ = PI*(576.0)/SCANLINE_DIV;

// scanline offset relative to pixel boundary
const float VPHASEDEG = 90.;
const float VPHASE = VPHASEDEG * PI / (180.0 * VFREQ);
// difference between scanline max and min intensities
const float PROMINENCE = 0.2;
// 1.0 makes lines with maximal luma fuse together
const float FLATNESS = 1.0;

float scanline(float y, float luma) {
    // scanlines
    float w = (y + VPHASE) * VFREQ;
    
    float flatness = 2.0 - luma * 2.0 * FLATNESS;  // more luminance = more flatness
    float sinw = pow(abs(sin(w)), flatness);
    sinw = (1.0 - PROMINENCE) + sinw * PROMINENCE;    

    return sinw;
}


void fragment() {
    vec2 xy = UV;

    float width_ratio = TEXTURE_PIXEL_SIZE.x / (FSC / FLINE);
    float height_ratio = TEXTURE_PIXEL_SIZE.y / VISIBLELINES;

    float altv = mod(floor(xy.y * VISIBLELINES + 0.5), 2.0) * PI;
    float invx = 0.25 / (FSC/FLINE); // equals 4 samples per Fsc period

    // lowpass U/V at baseband
    vec2 filtered = vec2(0.0, 0.0);
    for (int i = 0; i < FIRTAPS; i++) {
        vec2 uv = modem_uv(TEXTURE, xy, i - FIRTAPS/2, invx, altv, width_ratio, TEXTURE_PIXEL_SIZE);
        filtered += filter_gain * uv * FIR[i];
    }

    float t = xy.x * TEXTURE_PIXEL_SIZE.x;
    float wt = t * 2. * PI / width_ratio;

    float sinwt = sin(wt);
    float coswt = cos(wt + altv);

    float luma = luma_only(TEXTURE, xy, invx);

    COLOR = vec4(1.-luma, 1.-luma, 1.-luma, 1.0);
}
