// $MinimumShaderProfile: ps_2_0
// https://forum.doom9.org/showpost.php?p=1677542&postcount=5

// (C) 2014 Jan-Willem Krans (janwillem32 <at> hotmail.com)
// This file is part of Video pixel shader pack.
// This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// prototype basic fullscreen top-and-bottom to red-cyan anaglyph 3D stereoscopic view
// This shader should be run as a screen space pixel shader.
// This shader requires compiling with ps_2_0, but higher is better, see http://en.wikipedia.org/wiki/Pixel_shader to look up what PS version your video card supports.
// For this shader to work correctly, the video height in pixels on screen has to be an even amount.

sampler s0 : register(s0);

float2 c0;
float2 c1;

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	float2 texr = tex;

	float2 vr = float2(0., c0.y);
	float2 CurPixel = tex*c0-.5;
	float oy = CurPixel.y-vr.x;// video-relative y position
	float PixelOffset = floor(-.5*oy);// the two images are half-height, compensate for that, round half-pixel offsets down

	// adjust the y coordinate of the source position to the correct side of the video image using the pixel offset we just calculated
	tex.y += PixelOffset*c1.y;
	texr.y += (PixelOffset+.5*(vr.y-vr.x))*c1.y;// adjust the pixel offset to the video frame on the bottom side of the video data

	return float4(tex2D(s0, tex).r, tex2D(s0, texr).gbb);// sample from the left and right images and only pass-trough the red channel for the left, and the green and blue channels for the right view
}
