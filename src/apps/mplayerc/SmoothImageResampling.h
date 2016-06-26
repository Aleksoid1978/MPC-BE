//* High Quality ARGB image resizer
//* original code from http://www.geisswerks.com/ryan/FAQS/resize.html

static void Resize_HQ_4ch(const BYTE* src, int srcWidth, int srcHeight,
					  BYTE* dest, int destWidth, int destHeight)
{
	// Both buffers must be in ARGB format, and a scanline should be width * 4 bytes.
	ASSERT(src);
	ASSERT(dest);
	ASSERT(srcWidth >= 1);
	ASSERT(srcHeight >= 1);
	ASSERT(destWidth >= 1);
	ASSERT(destHeight >= 1);

	static std::vector<int> g_px1a(4096);
	static std::vector<int> g_px1ab(4096);

	// arbitrary resize.
	unsigned int *dsrc  = (unsigned int *)src;
	unsigned int *ddest = (unsigned int *)dest;

	const bool bUpsampleX = (srcWidth < destWidth);
	const bool bUpsampleY = (srcHeight < destHeight);

	// If too many input pixels map to one output pixel, our 32-bit accumulation values
	// could overflow - so, if we have huge mappings like that, cut down the weights:
	//	256 max color value
	//   *256 weight_x
	//   *256 weight_y
	//   *256 (16*16) maximum # of input pixels (x,y) - unless we cut the weights down...
	int weight_shift = 0;
	const float source_texels_per_out_pixel = ((srcWidth / (float)destWidth + 1)
											 * (srcHeight / (float)destHeight + 1));
	const float weight_per_pixel = source_texels_per_out_pixel * 256 * 256;  // weight_x * weight_y
	const float accum_per_pixel = weight_per_pixel * 256; // color value is 0 - 255
	const float weight_div = accum_per_pixel / 4294967000.0f;
	if (weight_div > 1) {
		weight_shift = (int)ceilf(logf((float)weight_div) / logf(2.0f));
	}
	weight_shift = min(15, weight_shift);  // this could go to 15 and still be ok.

	const float fh = 256 * srcHeight / (float)destHeight;
	const float fw = 256 * srcWidth / (float)destWidth;

	if (bUpsampleX && bUpsampleY) {
		// faster to just do 2x2 bilinear interp here
		if (g_px1a.size() < size_t(destWidth * 2)) {
			g_px1a.resize(destWidth * 2);
		}

		for (int x2 = 0; x2 < destWidth; x2++) {
			// find the x-range of input pixels that will contribute:
			int x1a = (int)(destWidth * fw);
			x1a = min(x1a, 256 * (srcWidth - 1) - 1);
			g_px1a[x2] = x1a;
		}

		// FOR EVERY OUTPUT PIXEL
		for (int y2 = 0; y2 < destHeight; y2++) {   
			// find the y-range of input pixels that will contribute:
			int y1a = (int)(y2 * fh);
			y1a = min(y1a, 256 * (srcHeight - 1) - 1);
			const int y1c = y1a >> 8;

			unsigned int *ddest = &((unsigned int *)dest)[y2 * destWidth];

			for (int x2 = 0; x2 < destWidth; x2++) {
				// find the x-range of input pixels that will contribute:
				const int x1a = g_px1a[x2]; // (int)(x2 * fw); 
				const int x1c = x1a >> 8;

				const unsigned int *dsrc2 = &dsrc[y1c * srcWidth + x1c];

				// PERFORM BILINEAR INTERPOLATION on 2x2 pixels
				unsigned int r = 0, g = 0, b = 0, a = 0;
				unsigned int weight_x = 256 - (x1a & 0xFF);
				unsigned int weight_y = 256 - (y1a & 0xFF);
				for (int y = 0; y < 2; y++) {
					for (int x = 0; x < 2; x++) {
						const unsigned int c = dsrc2[x + y * srcWidth];
						const unsigned int r_src = (c    ) & 0xFF;
						const unsigned int g_src = (c>> 8) & 0xFF;
						const unsigned int b_src = (c>>16) & 0xFF;
						const unsigned int w = (weight_x * weight_y) >> weight_shift;
						r += r_src * w;
						g += g_src * w;
						b += b_src * w;
						weight_x = 256 - weight_x;
					}
					weight_y = 256 - weight_y;
				}

				unsigned int c = ((r >> 16)) | ((g >> 8) & 0xFF00) | (b & 0xFF0000);
				*ddest++ = c; // ddest[y2 * w2 + x2] = c;
			}
		}
	} else {
		if (g_px1ab.size() < size_t(destWidth * 2 * 2)) {
			g_px1ab.resize(destWidth * 2 * 2);
		}

		for (int x2 = 0; x2 < destWidth; x2++) {
			// find the x-range of input pixels that will contribute:
			const int x1a = (int)((x2    ) * fw); 
			int x1b       = (int)((x2 + 1) * fw); 
			if (bUpsampleX) { // map to same pixel -> we want to interpolate between two pixels!
				x1b = x1a + 256;
			}
			x1b = min(x1b, 256 * srcWidth - 1);
			g_px1ab[x2 * 2    ] = x1a;
			g_px1ab[x2 * 2 + 1] = x1b;
		}

		// FOR EVERY OUTPUT PIXEL
		for (int y2 = 0; y2 < destHeight; y2++) {   
			// find the y-range of input pixels that will contribute:
			const int y1a = (int)((y2    ) * fh); 
			int y1b       = (int)((y2 + 1) * fh); 
			if (bUpsampleY) { // map to same pixel -> we want to interpolate between two pixels!
				y1b = y1a + 256;
			}
			y1b = min(y1b, 256 * srcHeight - 1);
			const int y1c = y1a >> 8;
			const int y1d = y1b >> 8;

			for (int x2 = 0; x2 < destWidth; x2++) {
				// find the x-range of input pixels that will contribute:
				const int x1a = g_px1ab[x2 * 2    ]; // (computed earlier)
				const int x1b = g_px1ab[x2 * 2 + 1]; // (computed earlier)
				const int x1c = x1a >> 8;
				const int x1d = x1b >> 8;

				// ADD UP ALL INPUT PIXELS CONTRIBUTING TO THIS OUTPUT PIXEL:
				unsigned int r = 0, g = 0, b = 0, a = 0;
				for (int y = y1c; y <= y1d; y++) {
					unsigned int weight_y = 256;
					if (y1c != y1d) {
						if (y == y1c) {
							weight_y = 256 - (y1a & 0xFF);
						} else if (y == y1d) {
							weight_y = (y1b & 0xFF);
						}
					}

					const unsigned int *dsrc2 = &dsrc[y * srcWidth + x1c];
					for (int x = x1c; x <= x1d; x++) {
						unsigned int weight_x = 256;
						if (x1c != x1d) {
							if (x == x1c) {
								weight_x = 256 - (x1a & 0xFF);
							} else if (x == x1d) {
								weight_x = (x1b & 0xFF);
							}
						}

						const unsigned int c = *dsrc2++; // dsrc[y * w1 + x];
						const unsigned int r_src = (c    ) & 0xFF;
						const unsigned int g_src = (c>> 8) & 0xFF;
						const unsigned int b_src = (c>>16) & 0xFF;
						const unsigned int w = (weight_x * weight_y) >> weight_shift;
						r += r_src * w;
						g += g_src * w;
						b += b_src * w;
						a += w;
					}
				}

				// write results
				const unsigned int c = ((r / a)) | ((g / a) << 8) | ((b / a) << 16);
				*ddest++ = c; // ddest[y2 * w2 + x2] = c;
			}
		}
	}
}
