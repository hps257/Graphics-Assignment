uniform sampler2D texture;
varying vec2 texCoordVar;
void main()
{
vec4 texColor = texture2D( texture, texCoordVar);
float brightness = (texColor.x+texColor.y+texColor.z)/3.0;
gl_FragColor = vec4(brightness, brightness, brightness, 1.0);
}