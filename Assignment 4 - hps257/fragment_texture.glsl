uniform sampler2D screenFramebuffer;
varying vec2 texCoordVar;
void main()
{
gl_FragColor = vec4(1.0-texture2D( screenFramebuffer, texCoordVar).xyz, 1.0);
}