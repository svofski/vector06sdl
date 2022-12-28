shader_type canvas_item;

uniform sampler2D wav;
uniform int frame_index;  // 0..3
uniform float midpoint;
uniform float scale;
uniform vec2 gobshite;

const float sample_step = 1.0/960.0;

const float framesize_x = 960.0 - 960.0/4.0; // 960 - trigger delay

float sdRoundedBox( in vec2 p, in vec2 b, in vec4 r )
{
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    vec2 q = abs(p)-b+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}

// h/t iq
float sdSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

float get_interp_value(vec2 uv, int frame)
{
	float perfect_x = uv.x * framesize_x;

	int left_index = int(floor(perfect_x));
	int right_index = int(ceil(perfect_x));

	float left_samp = texelFetch(wav, ivec2(left_index, frame), 0).r;
        float right_samp = texelFetch(wav, ivec2(right_index, frame), 0).r; 
	return mix(left_samp, right_samp, fract(perfect_x));
}

float get_power_at(vec2 uv, float x_pixel_size, int frame, float thicc)
{
	float prev_samp_value = get_interp_value(vec2(uv.x - x_pixel_size, uv.y), frame);
	float samp_value = get_interp_value(uv, frame);

	prev_samp_value = (prev_samp_value - midpoint) * scale;
	samp_value = (samp_value - midpoint) * scale;

	float dist = sdSegment(uv, vec2(uv.x - x_pixel_size, -prev_samp_value + 0.5), vec2(uv.x, -samp_value + 0.5));
	return 1.0 - smoothstep(0.0, thicc * 0.01, dist);
}

void fragment() {
	// the texture is unwrapped from Vector2(Left, Right)

	float xprev = UV.x - 1.0/gobshite.x;
	float xprev2 = UV.x - 1.0/gobshite.x * 2.0;

	float p = get_power_at(UV, 1.0/gobshite.x, frame_index, 1.);
	float p3 = get_power_at(UV, 1.0/gobshite.x, (frame_index + 1) % 4, 10.);
	float p2 = get_power_at(UV, 1.0/gobshite.x, (frame_index + 2) % 4, 7.5);
	float p1 = get_power_at(UV, 1.0/gobshite.x, (frame_index + 3) % 4, 5.0);

	vec4 c3 = vec4(p3, p3, p3, 1.0);
	vec4 c2 = vec4(p2, p2, p2, 1.0);
	vec4 c1 = vec4(p1, p1, p1, 1.0);
	vec4 c0 = vec4(p, p, p, 1.0);

	vec4 c = mix(c2, c3, 0.25);
	c = mix(c1, c, 0.25);
	c = mix(c0, c, 0.25);

	//float r = 8.0 * gobshite.x;
	//float dist = sdRoundedBox(UV - vec2(0.5, 0.5), vec2(0.45, 0.45), vec4(0.1, 0.1, 0.1, 0.1));
	//p = pow(1.0 - dist, 200.0);
	//p = 0.0;
	//if (dist < 0.01) p = 1.0;
	//p = 1.0 - smoothstep(0.0, 0.001, dist);
	//c = vec4(p, p, p, 1.0);

	c = mix(c, vec4(0.0, 0.5, 0.0, 1.0), 0.1);
	float edgeAlpha = 0.9 * smoothstep(0.0, 15.0/gobshite.x, UV.x) *
			  0.9 * smoothstep(1.0, 1.0 - 15.0/gobshite.x, UV.x) *
			  0.9 * smoothstep(0.0, 0.05, UV.y) *
			  0.9 * smoothstep(1.0, 1.0 - 0.05, UV.y);
	
	COLOR.rgba = vec4(c.r, c.g, c.b, edgeAlpha);
}

	
// primitive caveman version 
	//float bottom = min(cur, prev) - SCREEN_PIXEL_SIZE.y;
	//float top = max(cur, prev) + SCREEN_PIXEL_SIZE.y;
	//float p = (UV.y >= bottom) && (UV.y <= top) ? 1.0 : 0.0;
