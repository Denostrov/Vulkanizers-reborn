#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants
{
	vec4 data; //viewport width, viewport height, steps, {sphere size, polynomial degree, julia power}
	vec4 cameraPos; //4th argument is focal length
	vec4 cameraHorizontal; //4th argument is scene id
	vec4 cameraVertical; //4th argument is { _ , _, k projection}
	vec4 cameraDirection; //4th argument is iterations
	vec4 juliaC;
} pushConstants;

float qLength2(in vec4 q) { return dot(q, q); }

vec4 qPower(vec4 q, float power)
{
	float allLength = sqrt(qLength2(q));
	float pureLength = sqrt(dot(q.yzw, q.yzw));
	float phi;
	vec3 unit;
	if (q.x >= allLength || allLength <= 0.0001)
	{
		phi = 0.0;
	}
	else
	{
		phi = acos(q.x / allLength);
	}
	if (pureLength <= 0.0001)
	{
		unit = vec3(0.0, 0.0, 0.0);
	}
	else
	{
		unit = q.yzw / pureLength;
	}
	float powLength = pow(allLength, power);
	return vec4(powLength * cos(power * phi), unit * powLength * sin(power * phi));
}

vec4 qSquare(vec4 q)
{
	return vec4(q.x * q.x - q.y * q.y - q.z * q.z - q.w * q.w, 2.0 * q.x * q.yzw);
}

vec4 qCube(vec4 q)
{
	vec4 q2 = q*q;
	return vec4(q.x * (q2.x - 3.0*(q2.y + q2.z + q2.w)),
							q.yzw * (3.0 * q2.x - q2.y - q2.z - q2.w));
}

float DE_spheres(vec3 point)
{
	vec3 p = abs(mod(point - 1.0, 2.0) - 1.0);
	return length(p - vec3(1.0, 1.0, 1.0)) - pushConstants.data.w;
}

float DE_mandelbulb(vec3 point)
{
	int iterations;
	vec3 w = point;
	float dz = 1.0;
	float m = dot(w, w);
	for (iterations = 0; iterations <= int(pushConstants.cameraDirection.w); iterations++)
	{
		dz = pushConstants.data.w * pow(m, (pushConstants.data.w - 1.0) / 2.0)*dz;
		float r = length(w);
		float b = pushConstants.data.w * acos(w.y/r);
		float a = pushConstants.data.w * atan(w.x, w.z);
		w = point + pow(r, pushConstants.data.w) * vec3(sin(b) * sin(a), cos(b), sin(b)*cos(a));
		
		m = dot(w, w);
		if (m > 2048.0) break;
	}
	return 0.25 * log(m) * sqrt(m) / dz;
}

float DE_juliaExact(vec3 point)
{
	int iterations;
	vec4 z = vec4(point, pushConstants.cameraVertical.w);
	float dz2 = 1.0;
	float m2 = 0.0;
	for (iterations = 0; iterations <= int(pushConstants.cameraDirection.w); iterations++)
	{
		dz2 = pushConstants.data.w * pushConstants.data.w * qLength2(qPower(z, pushConstants.data.w - 1.0)) * dz2;
		z = qPower(z, pushConstants.data.w) + pushConstants.juliaC;
		m2 = qLength2(z);
		if (m2 > 512.0) break;
	}
	return 0.25 * log(m2)*sqrt(m2/dz2);
}

float DE_juliaSquare(vec3 point)
{
	int iterations;
	vec4 z = vec4(point, pushConstants.cameraVertical.w);
	float dz2 = 1.0;
	float m2 = 0.0;
	for (iterations = 0; iterations <= int(pushConstants.cameraDirection.w); iterations++)
	{
		dz2 = 4.0 * qLength2(z) * dz2;
		z = qSquare(z) + pushConstants.juliaC;
		m2 = qLength2(z);
		if (m2 > 512.0) break;
	}
	return 0.25 * log(m2)*sqrt(m2/dz2);
}

float DE_juliaCube(vec3 point)
{
	int iterations;
	vec4 z = vec4(point, pushConstants.cameraVertical.w);
	float dz2 = 1.0;
	float m2 = 0.0;
	for (iterations = 0; iterations <= int(pushConstants.cameraDirection.w); iterations++)
	{
		dz2 = 9.0 * qLength2(qSquare(z)) * dz2;
		z = qCube(z) + pushConstants.juliaC;
		m2 = qLength2(z);
		if (m2 > 512.0) break;
	}
	return 0.25 * log(m2)*sqrt(m2/dz2);
}

float trace(vec3 from, vec3 direction)
{
	float totalDistance = 0.0;
	int steps;
	float distance;
	for (steps = 0; steps < int(pushConstants.data.z); steps++)
	{
		vec3 p = from + totalDistance * direction;
		switch(int(pushConstants.cameraHorizontal.w))
		{
			case 0:	distance = DE_spheres(p);
							break;
			case 1:	distance = DE_mandelbulb(p);
							break;
			case 2:	distance = DE_juliaExact(p);
							break;
			case 3:	distance = DE_juliaSquare(p);
							break;
			case 4:	distance = DE_juliaCube(p);
							break;
			default:	distance = 1000.0;
							break;
		}
		totalDistance += distance;
		if (distance < 0.0001) break;
		if (distance > 512.0) discard;
	}
	return 1.0-float(steps)/float(int(pushConstants.data.z));
}

void main() 
{
	vec3 horizontal = pushConstants.cameraHorizontal.xyz * pushConstants.data.x / pushConstants.data.y;
	vec3 vertical = pushConstants.cameraVertical.xyz;
	vec3 topLeftCorner = pushConstants.cameraPos.xyz - horizontal/2.0 + vertical/2.0 + pushConstants.cameraDirection.xyz * pushConstants.cameraPos.w;
	float pixelColor = trace(pushConstants.cameraPos.xyz, normalize(topLeftCorner + gl_FragCoord.x/pushConstants.data.x * horizontal - gl_FragCoord.y/pushConstants.data.y * vertical - pushConstants.cameraPos.xyz));
	outColor = vec4(pixelColor, pixelColor, pixelColor, 1.0);
}