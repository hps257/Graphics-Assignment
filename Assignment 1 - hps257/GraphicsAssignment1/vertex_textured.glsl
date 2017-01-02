attribute vec4 position;
attribute vec2 texCoord;
varying vec2 texCoordVar;
uniform vec2 location;

void main()
{
    texCoordVar = texCoord;
	gl_Position = vec4(location.x, location.y, 0.0, 0.0) + position;
}