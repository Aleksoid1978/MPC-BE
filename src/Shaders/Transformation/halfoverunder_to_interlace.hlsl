// convert Half OverUnder to Interlace

#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))

sampler s0 : register(s0);
float4 p0 : register(c0);

#define height  (p0[0])
#define dtop    (p0[2])
#define dbottom (p0[3])

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
