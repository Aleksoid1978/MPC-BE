// downscaling "Simple averaging"

#if Ml
#define tex2D(s, t) tex2Dlod(s, float4(t, 0., 0.))
#define MAXSTEPS 10
#else
#define MAXSTEPS 3
#endif

sampler s0 : register(s0);
float2 dxdy : register(c0);
float2 r : register(c1);

static const int kx = clamp(int(r.x+0.5), 2, MAXSTEPS);
static const int ky = clamp(int(r.y+0.5), 2, MAXSTEPS);
static const int kxy = kx*ky;

float4 main(float2 tex : TEXCOORD0) : COLOR0
{
	float2 t = frac(tex);
	float2 pos = tex-t+0.5;

	float4 result = 0;
	int startx = kx/2 - kx;
	int starty = ky/2 - ky;
	[unroll] for (int ix = 0; ix < kx; ix++) {
		[unroll] for (int iy = 0; iy < ky; iy++) {
			result = result + tex2D(s0, (pos + float2(startx+ix, startx+iy))*dxdy) / kxy;
		}
	}

	return result;
}
