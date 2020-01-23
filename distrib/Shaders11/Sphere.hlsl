// $MinimumShaderProfile: ps_4_0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 pxy;
	float2 wh;
	uint   counter;
	float  clock;
};

#define PI acos(-1)

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	// - this is a very simple raytracer, one sphere only
	// - no reflection or refraction, yet (my ati 9800 has a 64 + 32 instruction limit...)

	float3 pl = float3(3, -3, -4); // light pos
	float4 cl = 0.4; // light color

	float3 pc = float3(0, 0, -1);  // cam pos
	float3 ps = float3(0, 0, 0.5); // sphere pos
	float r = 0.65; // sphere radius

	float3 pd = normalize(float3(coord.x - 0.5, coord.y - 0.5, 0) - pc);

	float A = 1;
	float B = 2 * dot(pd, pc - ps);
	float C = dot(pc - ps, pc - ps) - r * r;
	float D = B * B - 4 * A * C;

	float4 c0 = 0;

	if (D >= 0) {
		// t2 is the smaller, obviously...
		// float t1 = (-B + sqrt(D)) / (2 * A);
		// float t2 = (-B - sqrt(D)) / (2 * A);
		// float t = min(t1, t2);

		float t = (-B - sqrt(D)) / (2 * A);

		// intersection data
		float3 p = pc + pd * t;
		float3 n = normalize(p  - ps);
		float3 l = normalize(pl - p);

		// mapping the image onto the sphere
		coord = acos(-n) / PI;

		// rotate it
		coord.x = frac(coord.x + frac(clock / 10));

		// diffuse + specular
		c0 = tex.Sample(samp, coord) * dot(n, l) + cl * pow(max(dot(l, reflect(pd, n)), 0), 50);
	}

	return c0;
}
