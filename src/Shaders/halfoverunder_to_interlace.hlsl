// convert Half OverUnder to Interlace

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#endif

sampler s0 : register(s0);
float2 p0 : register(c0);
float4 p1 : register(c1);

#define width  (p0[0])
#define height (p0[1])

// frame rect
#define dleft   (p1[0])
#define dtop    (p1[1])
#define dright  (p1[2])
#define dbottom (p1[3])

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	if (tex.y >= dbottom) {
		return float4(0.0, 0.0, 0.0, 1.0);
	}

	if (fmod((tex.y - dtop) * height, 2) < 1.0) {
		// even
		tex.y = (dtop + tex.y) / 2;
	} else {
		// odd
		tex.y = (dbottom + tex.y) / 2;
	}

	return tex2D(s0, tex);
}
