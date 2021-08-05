#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants
{
	vec4 data; //viewport width, viewport height, steps, sphere size
	vec4 cameraPos; //4th argument is focal length
} pushConstants;

float DE(vec3 point)
{
	vec3 p = abs(mod(point - 1.0, 2.0) - 1.0);
	return length(p - vec3(1.0, 1.0, 1.0)) - pushConstants.data.w;
}

float trace(vec3 from, vec3 direction)
{
	float totalDistance = 0.0;
	int steps;
	for (steps = 0; steps < int(pushConstants.data.z); steps++)
	{
		vec3 p = from + totalDistance * direction;
		float distance = DE(p);
		totalDistance += distance;
		if (distance < 0.01) break;
		if (distance > 100) discard;
	}
	return 1.0-float(steps)/float(int(pushConstants.data.z));
}

void main() 
{
	vec3 horizontal = vec3(pushConstants.data.x / pushConstants.data.y, 0.0, 0.0);
	vec3 vertical = vec3(0.0, 1.0, 0.0);
	vec3 lowerLeftCorner = pushConstants.cameraPos.xyz - horizontal/2.0 - vertical/2.0 - vec3(0.0, 0.0, pushConstants.cameraPos.w);
	float pixelColor = trace(pushConstants.cameraPos.xyz, normalize(lowerLeftCorner + gl_FragCoord.x/pushConstants.data.x * horizontal + gl_FragCoord.y/pushConstants.data.y * vertical - pushConstants.cameraPos.xyz));
	outColor = vec4(pixelColor, pixelColor, pixelColor, 1.0);
}