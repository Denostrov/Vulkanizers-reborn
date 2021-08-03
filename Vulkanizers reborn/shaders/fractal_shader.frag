#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

float DE(vec3 point)
{
	vec3 p = abs(mod(point - 1.0, 2.0) - 1.0);
	return length(p - vec3(1.0, 1.0, 1.0)) - 0.5;
}

float trace(vec3 from, vec3 direction)
{
	float totalDistance = 0.0;
	int steps;
	for (steps = 0; steps < 100; steps++)
	{
		vec3 p = from + totalDistance * direction;
		float distance = DE(p);
		totalDistance += distance;
		if (distance < 0.1) break;
		if (distance > 100) discard;
	}
	return 1.0-float(steps)/float(100);
}

void main() 
{
	vec3 horizontal = vec3(1.0, 0.0, 0.0);
	vec3 vertical = vec3(0.0, 1.0, 0.0);
	vec3 lowerLeftCorner = -horizontal/2 -vertical/2 - vec3(0.0, 0.0, 1.0);
	float pixelColor = trace(vec3(0.0, 0.0, 0.0), normalize(lowerLeftCorner + gl_FragCoord.x/1000.0 * horizontal + gl_FragCoord.y/1000.0 * vertical));
	outColor = vec4(pixelColor, pixelColor, pixelColor, 1.0);
}