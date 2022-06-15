shader_type canvas_item;

void fragment() {
	vec4 c = texture(TEXTURE, UV);//textureLod(TEXTURE, UV, 0.0).rgb;
	COLOR.rgba = c;
}